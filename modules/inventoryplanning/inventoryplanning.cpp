/***************************************************************************
 *                                                                         *
 * Copyright (C) 2015 by frePPLe bvba                                      *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "inventoryplanning.h"

namespace module_inventoryplanning
{

const Keyword InventoryPlanningSolver::tag_fixed_order_cost("fixed_order_cost");
const Keyword InventoryPlanningSolver::tag_holding_cost("holding_cost");
const Keyword InventoryPlanningSolver::tag_horizon_start("horizon_start");
const Keyword InventoryPlanningSolver::tag_horizon_end("horizon_end");

const MetaClass *InventoryPlanningSolver::metadata;
Calendar *InventoryPlanningSolver::cal = NULL;
int InventoryPlanningSolver::horizon_start = 0;
int InventoryPlanningSolver::horizon_end = 365;
double InventoryPlanningSolver::fixed_order_cost = 20;
double InventoryPlanningSolver::holding_cost = 0.05;


int InventoryPlanningSolver::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<InventoryPlanningSolver>(
    "solver", "solver_inventoryplanning", Object::create<InventoryPlanningSolver>
    );
  registerFields<InventoryPlanningSolver>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  PythonType& x = FreppleClass<InventoryPlanningSolver, Solver>::getPythonType();
  x.setName("solver_inventoryplanning");
  x.setDoc("frePPLe solver_inventoryplanning");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("solve", Solver::solve, METH_NOARGS, "run the solver");
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}



PyObject* InventoryPlanningSolver::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Create the solver
    InventoryPlanningSolver *s = new InventoryPlanningSolver();

    // Iterate over extra keywords, and set attributes.   @todo move this responsibility to the readers...
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwds, &pos, &key, &value))
    {
      PythonData field(value);
      PyObject* key_utf8 = PyUnicode_AsUTF8String(key);
      DataKeyword attr(PyBytes_AsString(key_utf8));
      Py_DECREF(key_utf8);
      const MetaFieldBase* fmeta = metadata->findField(attr.getHash());
      if (!fmeta)
        fmeta = Solver::metadata->findField(attr.getHash());
      if (fmeta)
        // Update the attribute
        fmeta->setField(s, field);
      else
        PyErr_Format(PyExc_AttributeError,
            "attribute '%S' on '%s' can't be updated",
            key, Py_TYPE(s)->tp_name);
    };

    // Return the object. The reference count doesn't need to be increased
    // as we do with other objects, because we want this object to be available
    // for the garbage collector of Python.
    return static_cast<PyObject*>(s);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


void InventoryPlanningSolver::solve(void* v)
{
  if (getLogLevel() > 0)
    logger << "Start inventory planning solver" << endl;

  // Validate the solver is initialised correctly
  if (!cal)
    throw DataException("Inventory planning solver requires a calendar to be specified");

  // Call the solve method for each buffer
  for (Buffer::iterator i = Buffer::begin(); i != Buffer::end(); ++i)
    if (i->getType() != *BufferInfinite::metadata)
      solve(&*i);

  if (getLogLevel() > 0)
    logger << "End inventory planning solver" << endl;
}


void InventoryPlanningSolver::solve(const Buffer* b, void* v)
{
  short loglevel = getLogLevel();
  if (loglevel > 1)
    logger << "Inventory planning solver on buffer " << b << endl;

  // Inventory planning parameters of this buffer
  double leadtime_deviation = 0;
  double demand_deviation = 0;
  bool nostock = false;
  double roq_min_qty = 0.0;
  double roq_max_qty = DBL_MAX;
  Duration roq_min_poc;
  Duration roq_max_poc(10 * 365 * 86400);
  double roq_multiple = 1.0;
  string distribution = "normal";
  double service_level = 0.0;
  double ss_min_qty = 0.0;
  double ss_max_qty = DBL_MAX;
  double ss_multiple = 1.0;
  Duration ss_min_poc;
  Duration ss_max_poc(10 * 365 * 86400);
  double price = 0.0;
  if (b->getItem())
    price = b->getItem()->getPrice();

  // Get the lead time from the operation replenishing this buffer
  Duration leadtime;
  Operation *oper = b->getProducingOperation();
  if (!oper)
  {
    if (loglevel > 1)
      logger << "   No replenishing operation defined" << endl;
    return;
  }
  else if (oper->getType() != *OperationFixedTime::metadata)
    logger << "   Replenishing operation should be of type fixed_time" << endl; // TODO Make more generic
  else
    leadtime = static_cast<OperationFixedTime*>(oper)->getDuration();

  // Report parameter settings
  if (loglevel > 1)
  {
    logger << "   lead time: " << leadtime
      << ", lead time std deviation: " << leadtime_deviation
      << ", demand std deviation: " << demand_deviation
      << ", price: " << price << endl;
    logger << "   ROQ: min quantity: " << roq_min_qty
      << ", max quantity: " << roq_max_qty
      << ", min cover: " << roq_min_poc
      << ", max cover: " << roq_max_poc
      << ", multiple: " << roq_multiple << endl;
    logger << "   SS: min quantity: " << ss_min_qty
      << ", max quantity: " << ss_max_qty
      << ", min cover: " << ss_min_poc
      << ", max cover: " << ss_max_poc
      << ", multiple: " << ss_multiple << endl;
    logger << "   don't stock: " << nostock
      << ", distribution: " << distribution
      << ", service level: " << service_level << endl;
  }

  // Prepare the calendars to retrieve the results
  Calendar *roq_calendar = oper->getSizeMinimumCalendar();
  Calendar *ss_calendar = b->getMinimumCalendar();
  if (roq_calendar)
  {
    // Erase existing calendar buckets
    while (true)
    {
      CalendarBucket::iterator i = roq_calendar->getBuckets();
      if (i == roq_calendar->getBuckets().end())
        break;
      roq_calendar->removeBucket(&*i);
    }
  }
  else
  {
    // Create a brand new calendar
    roq_calendar = new Calendar();
    roq_calendar->setName("Reorder quantities for " + b->getName());
    roq_calendar->setSource("Inventory planning");
  }
  if (ss_calendar)
  {
    // Erase existing calendar buckets
    while (true)
    {
      CalendarBucket::iterator i = ss_calendar->getBuckets();
      if (i == ss_calendar->getBuckets().end())
        break;
      ss_calendar->removeBucket(&*i);
    }
  }
  else
  {
    // Create a brand new calendar
    ss_calendar = new Calendar();
    ss_calendar->setName("Safety stock for " + b->getName());
    ss_calendar->setSource("Inventory planning");
  }
  CalendarBucket *roq_calendar_bucket = NULL;
  CalendarBucket *ss_calendar_bucket = NULL;

  // Loop over all buckets in the horizon
  Date startdate = Plan::instance().getCurrent() - Duration(horizon_start * 86400L);
  Date enddate = Plan::instance().getCurrent() + Duration(horizon_end * 86400L);
  Date bucketstart;
  for (Calendar::EventIterator bucket(cal, startdate, true);
    bucket.getDate() < enddate; ++bucket)
  {
    if (!bucketstart)
    {
      bucketstart = bucket.getDate();
      continue;
    }
    Date bucketend = bucket.getDate();
    if (loglevel > 2)
      logger << "     Bucket " << bucketstart << " - " << bucketend << ": ";

    // Get the demand per day, measured over the lead time.
    // Demand in buckets which are completely within the lead time is all added.
    // The last bucket will be only partially within the lead time. We take the
    // complete demand in that last bucket and add a proportion of it to the
    // lead time demand.
    double demand = 0.0;
    double lastdemand = 0.0;
    Date fence = bucketstart + Duration(leadtime);
    Date last_bucket_start;
    Date last_bucket_end;
    for (Calendar::EventIterator tmp(cal, startdate, true);
      tmp.getDate() < enddate; ++tmp)
    {
      last_bucket_start = last_bucket_end;
      last_bucket_end = tmp.getDate();
      if (last_bucket_end > fence)
        break;
    }
    for (Buffer::flowplanlist::const_iterator i = b->getFlowPlans().begin();
      i != b->getFlowPlans().end(); ++i)
    {
      if (i->getQuantity() < 0)
      {
        if (i->getDate() > last_bucket_start)
          lastdemand -= i->getQuantity();
        else if (i->getDate() > bucketstart)
          demand -= i->getQuantity();
      }
      if (i->getDate() >= last_bucket_end)
        break;
    }
    if (lastdemand > 0)
      demand += lastdemand * (fence - last_bucket_start) / (last_bucket_end - last_bucket_start);
    demand = demand / leadtime * 86400;

    // Compute the reorder quantity
    // 1. start with the wilson formula for the optimal reorder quantity
    // 2. apply the minimum constraints in quantity and period of cover
    // 3. round up to the next roq_multiple, if specified
    // 4. apply the maximum constraints in quantity and period of cover, and
    //    round down when doing so.
    double roq = 1.0;
    if (price)
      roq = sqrt(2 * 365 * demand * fixed_order_cost / holding_cost / price);
    if (roq < roq_min_qty)
      roq = roq_min_qty;
    if (roq < demand * roq_min_poc)
      roq = demand * roq_min_poc;
    if (roq_multiple > 0)
      roq = roq_multiple * static_cast<int>(roq / roq_multiple + 0.99999999);
    if (roq > roq_max_qty)
    {
      roq = roq_max_qty;
      if (roq_multiple > 0)
        roq = roq_multiple * static_cast<int>(roq / roq_multiple);
    }
    if (roq > demand * roq_max_poc)
    {
      roq = demand * roq_max_poc;
      if (roq_multiple > 0)
        roq = roq_multiple * static_cast<int>(roq / roq_multiple);
    }

    // Compute the safety stock
    // 1. start with the value based on the desired service level
    // 2. apply the minimum constraints in quantity and period of cover
    // 3. round up to the next roq_multiple, if specified
    // 4. apply the maximum constraints in quantity and period of cover, and
    //    round down when doing so.
    double ss = 0.0;
    if (service_level > 0)
    {
      if (demand_deviation && leadtime_deviation)
        ss = 1.0 * sqrt(demand * demand_deviation * demand_deviation + demand * demand * leadtime_deviation * leadtime_deviation);
      else if (demand_deviation)
        ss = 1.0 * sqrt(demand) * demand_deviation;
      else if (leadtime_deviation)
        ss = 1.0 * demand * leadtime_deviation;
    }
    if (ss < ss_min_qty)
      ss = ss_min_qty;
    if (ss < demand * ss_min_poc)
      ss = demand * ss_min_poc;
    if (ss_multiple > 0)
      ss = ss_multiple * static_cast<int>(ss / ss_multiple + 0.99999999);
    if (ss > ss_max_qty)
    {
      ss = ss_max_qty;
      if (ss_multiple > 0)
        ss = ss_multiple * static_cast<int>(ss / ss_multiple);
    }
    if (ss > demand * ss_max_poc)
    {
      ss = demand * ss_max_poc;
      if (ss_multiple > 0)
        ss = ss_multiple * static_cast<int>(ss / ss_multiple);
    }

    // Final result
    if (loglevel > 2)
      logger << "demand: " << demand
        << ", roq: " << roq
        << ", ss: " << ss
        << ", rop: " << (ss + leadtime * demand / 86400) << endl;

    // Store the result on the ROQ and SS calendars
    if (!ss_calendar_bucket || fabs(ss_calendar_bucket->getValue() - ss) > ROUNDING_ERROR)
    {
      if (ss_calendar_bucket)
        ss_calendar_bucket->setEnd(bucketstart);
      ss_calendar_bucket = new CalendarBucket();
      ss_calendar_bucket->setStart(bucketstart);
      ss_calendar_bucket->setCalendar(ss_calendar);
      ss_calendar_bucket->setValue(ss);
    }
    if (!roq_calendar_bucket || fabs(roq_calendar_bucket->getValue() - roq) > ROUNDING_ERROR)
    {
      if (roq_calendar_bucket)
        roq_calendar_bucket->setEnd(bucketstart);
      roq_calendar_bucket = new CalendarBucket();
      roq_calendar_bucket->setStart(bucketstart);
      roq_calendar_bucket->setCalendar(roq_calendar);
      roq_calendar_bucket->setValue(roq);
    }

    // Prepare for the next bucket
    bucketstart = bucket.getDate();
  }
}

}       // end namespace