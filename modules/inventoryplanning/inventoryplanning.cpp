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

#define SOURCE_IP "Inventory planning"

namespace module_inventoryplanning
{

const Keyword InventoryPlanningSolver::tag_fixed_order_cost("fixed_order_cost");
const Keyword InventoryPlanningSolver::tag_holding_cost("holding_cost");
const Keyword InventoryPlanningSolver::tag_horizon_start("horizon_start");
const Keyword InventoryPlanningSolver::tag_horizon_end("horizon_end");
const Keyword InventoryPlanningSolver::tag_service_level_on_average_inventory("service_level_on_average_inventory");

const MetaClass *InventoryPlanningSolver::metadata;
Calendar *InventoryPlanningSolver::cal = NULL;
int InventoryPlanningSolver::horizon_start = 0;
int InventoryPlanningSolver::horizon_end = 365;
double InventoryPlanningSolver::fixed_order_cost = 20;
double InventoryPlanningSolver::holding_cost = 0.05;
Duration InventoryPlanningSolver::bucket_size;
bool InventoryPlanningSolver::service_level_on_average_inventory = true;


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
  x.addMethod(
    "computeStockoutProbability",
    computeStockoutProbability,
    METH_VARARGS,
    "compute the risk of a stockout within the lead time"
    );
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

  // Measure the average bucket size
  // We round to the nearest day, to assure we get stable results over time.
  Date startdate = Plan::instance().getCurrent() - Duration(horizon_start * 86400L);
  Date enddate = Plan::instance().getCurrent() + Duration(horizon_end * 86400L);
  Date start;
  Date end;
  unsigned int cnt = 0;
  for (Calendar::EventIterator bucket(cal, startdate, true);
    bucket.getDate() < enddate; ++bucket)
  {
    if (!start)
      start = bucket.getDate();
    else
    {
      end = bucket.getDate();
      ++cnt;
    }
  }
  if (!cnt)
    throw DataException("Invalid calendar");
  bucket_size = Duration(static_cast<long>(end - start) / cnt / 86400) * 86400;
  if (bucket_size <= 0L)
    throw DataException("Invalid calendar");

  // Step 1: Erase the previous plan (except locked operationplans)
  for (Operation::iterator o = Operation::begin(); o != Operation::end(); ++o)
    o->deleteOperationPlans();

  // Step 2: Create a delivery operationplan for all demands
  for (Demand::iterator d = Demand::begin(); d != Demand::end(); ++d)
  {
    // Determine the quantity to be planned and the date for the planning loop
    double plan_qty = d->getQuantity() - d->getPlannedQuantity();
    if (plan_qty < ROUNDING_ERROR || d->getDue() == Date::infiniteFuture)
      continue;

    // Select delivery operation
    Operation* deliveryoper = d->getDeliveryOperation();
    if (!deliveryoper)
      continue;

    // Respect minimum shipment quantities
    if (plan_qty < d->getMinShipment())
      plan_qty = d->getMinShipment();

    // Create a delivery operationplan for the remaining quantity
    OperationPlan* deli = deliveryoper->createOperationPlan(
      plan_qty, Date::infinitePast, d->getDue(), &*d, NULL, 0, true
      );
    deli->activate();
  }

  // Step 3: Solve buffer by buffer, ordered by level
  SolverMRP prop;
  prop.setConstraints(0);
  prop.setLogLevel(0);
  prop.setPropagate(false);
  for (short lvl = -1; lvl <= HasLevel::getNumberOfLevels(); ++lvl)
  {
    for (Buffer::iterator b = Buffer::begin(); b != Buffer::end(); ++b)
    {
      if (b->getLevel() != lvl)
        // Not your turn yet...
        continue;

      // We know the complete demand on the buffer now.
      // We can calculate the ROQ and safety stock.
      if (b->getType() != *BufferInfinite::metadata)
        solve(&*b);

      // Given the demand, ROQ and safety stock, we resolve the shortage
      // with an unconstrained propagation to the next level.
      prop.getCommands().state->curBuffer = NULL;
      prop.getCommands().state->q_qty = -1.0;
      prop.getCommands().state->q_date = Date::infinitePast;
      prop.getCommands().state->a_cost = 0.0;
      prop.getCommands().state->a_penalty = 0.0;
      prop.getCommands().state->curDemand = NULL;
      prop.getCommands().state->curOwnerOpplan = NULL;
      prop.getCommands().state->a_qty = 0;
      b->solve(prop, &(prop.getCommands()));
      prop.getCommands().CommandManager::commit();
    }
  }

  if (getLogLevel() > 0)
    logger << "End inventory planning solver" << endl;
}

Duration InventoryPlanningSolver::getBufferLeadTime(const Buffer* b)
{
  Duration leadtime;
  short loglevel = getLogLevel();
  Operation *oper = b->getProducingOperation();
  if (!oper)
  {
    if (loglevel > 1)
      logger << "   No replenishing operation defined" << endl;
    // Setting an axtremely long lead time, which results in a huge
    // safety stock that covers the entire horizon.
    leadtime = 999L * 86400L;
  }
  else if (oper->getType() == *OperationAlternate::metadata)
  {
    // Alternate operation: Take the lead time of the preferred operation
    int curPrio = INT_MAX;
    for (Operation::Operationlist::const_iterator
      sub = oper->getSubOperations().begin();
      sub != oper->getSubOperations().end();
      ++sub)
      {
        if ((*sub)->getPriority() < curPrio && (
          (*sub)->getOperation()->getType() == *OperationFixedTime::metadata
          || (*sub)->getOperation()->getType() == *OperationItemDistribution::metadata
          || (*sub)->getOperation()->getType() == *OperationItemSupplier::metadata
          ))
        {
          leadtime = static_cast<OperationFixedTime*>((*sub)->getOperation())->getDuration();
          curPrio = (*sub)->getPriority();
        }
      }
  }
  else if (oper->getType() == *OperationSplit::metadata)
  {
    // Split operation: Take the lead time of the longest operation
    for (Operation::Operationlist::const_iterator
      sub = oper->getSubOperations().begin();
      sub != oper->getSubOperations().end();
      ++sub)
      {
        if ((*sub)->getOperation()->getType() != *OperationFixedTime::metadata
          || (*sub)->getOperation()->getType() != *OperationItemDistribution::metadata
          || (*sub)->getOperation()->getType() != *OperationItemSupplier::metadata
          )
        {
          Duration tmp = static_cast<OperationFixedTime*>((*sub)->getOperation())->getDuration();
          if (tmp > leadtime)
            leadtime = tmp;
        }
      }
  }
  else if (
    oper->getType() != *OperationFixedTime::metadata
    && oper->getType() != *OperationItemDistribution::metadata
    && oper->getType() != *OperationItemSupplier::metadata
    )
    // Using a lead time of 0
    logger << "   Replenishing operation should be of type fixed_time" << endl;
  else
    // After all special cases, the normal case of an operation with a fixed duration
    leadtime = static_cast<OperationFixedTime*>(oper)->getDuration();

  return leadtime;
}

void InventoryPlanningSolver::solve(const Buffer* b, void* v)
{
  short loglevel = getLogLevel();
  if (loglevel > 1)
    logger << "Inventory planning solver on buffer " << b << endl;

  // Inventory planning parameters of this buffer
  double leadtime_deviation = b->getDoubleProperty("leadtime_deviation", 0.0);
  double demand_deviation = b->getDoubleProperty("demand_deviation", 0.0);
  string roq_type = b->getStringProperty("roq_type", "combined");
  string ss_type = b->getStringProperty("ss_type", "combined");
  bool nostock = b->getBoolProperty("nostock", false);
  if (nostock)
  {
    roq_type = "nostock";
    ss_type = "nostock";
  }
  double roq_min_qty = b->getDoubleProperty("roq_min_qty", 1);
  double roq_max_qty = b->getDoubleProperty("roq_max_qty", DBL_MAX);
  Duration roq_min_poc = static_cast<long>(b->getDoubleProperty("roq_min_poc", 0) * 86400);
  Duration roq_max_poc = static_cast<long>(b->getDoubleProperty("roq_max_poc", 10 * 365) * 86400);
  double roq_multiple = b->getDoubleProperty("roq_multiple", 1);
  string distname = b->getStringProperty("distribution", "Automatic");
  distribution dist = matchDistributionName(distname);
  double service_level = b->getDoubleProperty("service_level", 0.0);
  if (service_level < 0)
    service_level = 0;
  if (service_level >= 100)
    service_level = 99.99;
  double ss_min_qty = b->getDoubleProperty("ss_min_qty", 0);
  double ss_max_qty = b->getDoubleProperty("ss_max_qty", DBL_MAX);
  double ss_multiple = b->getDoubleProperty("ss_multiple", 1);
  Duration ss_min_poc = static_cast<long>(b->getDoubleProperty("ss_min_poc", 0) * 86400);
  Duration ss_max_poc = static_cast<long>(b->getDoubleProperty("ss_max_poc", 3650) * 86400);
  double price = 0.0;
  if (b->getItem())
    price = b->getItem()->getPrice();

  // Get the lead time from the operation replenishing this buffer
  Duration leadtime = getBufferLeadTime(b);

  // Report parameter settings
  if (loglevel > 1)
  {
    logger << "   lead time: " << leadtime
      << ", lead time std deviation: " << leadtime_deviation
      << ", demand std deviation: " << demand_deviation
      << ", price: " << price << endl;
    logger << "   ROQ: type: " << roq_type
      << ", min quantity: " << roq_min_qty
      << ", max quantity: " << roq_max_qty
      << ", min cover: " << roq_min_poc
      << ", max cover: " << roq_max_poc
      << ", multiple: " << roq_multiple << endl;
    logger << "   SS: type: " << ss_type
      << ", min quantity: " << ss_min_qty
      << ", max quantity: " << ss_max_qty
      << ", min cover: " << ss_min_poc
      << ", max cover: " << ss_max_poc
      << ", multiple: " << ss_multiple << endl;
    logger << "   don't stock: " << nostock
      << ", distribution: " << distname
      << ", service level: " << service_level << endl;
  }

  // Sanity check for the parameters
  if (roq_multiple > 0.0 && roq_min_qty < roq_multiple)
    roq_min_qty = roq_multiple;
  if (ss_min_qty < 0.0)
    ss_min_qty = 0.0;
  if (ss_max_qty < 0.0)
    ss_max_qty = 0.0;
  if (ss_multiple < 0.0)
    ss_multiple = 0.0;
  if (ss_multiple > 0.0 && ss_min_qty < ss_multiple)
    ss_min_qty = ss_multiple;

  // Prepare the calendars to retrieve the results
  Calendar *roq_calendar = NULL;
  Operation *oper = b->getProducingOperation();
  if (oper)
    roq_calendar = oper->getSizeMinimumCalendar();
  if (!roq_calendar)
    // Automatically association based on the calendar name.
    roq_calendar = Calendar::find("ROQ for " + b->getName());
  if (roq_calendar)
  {
    // Erase existing calendar buckets
    // We don't delete immediately, but wait until the iterator has moved on
    CalendarBucket *to_delete = NULL;
    CalendarBucket::iterator bcktiter = roq_calendar->getBuckets();
    while (CalendarBucket *bckt = bcktiter.next())
    {
      if (to_delete)
      {
        roq_calendar->removeBucket(to_delete);
        to_delete = NULL;
      }
      if (bckt->getSource() == SOURCE_IP)
        to_delete = bckt;
    }
    if (to_delete)
      roq_calendar->removeBucket(to_delete);
  }
  else
  {
    // Create a brand new calendar
    roq_calendar = new CalendarDefault();
    roq_calendar->setName("ROQ for " + b->getName());
    roq_calendar->setSource(SOURCE_IP);
  }
  Calendar *ss_calendar = b->getMinimumCalendar();
  if (!ss_calendar)
    // Automatically association based on the calendar name.
    ss_calendar = Calendar::find("SS for " + b->getName());
  if (ss_calendar)
  {
    // Erase existing calendar buckets
    // We don't delete immediately, but wait until the iterator has moved on
    CalendarBucket *to_delete = NULL;
    CalendarBucket::iterator bcktiter = ss_calendar->getBuckets();
    while (CalendarBucket *bckt = bcktiter.next())
    {
      if (to_delete)
      {
        ss_calendar->removeBucket(to_delete);
        to_delete = NULL;
      }
      if (bckt->getSource() == SOURCE_IP)
        to_delete = bckt;
    }
    if (to_delete)
      ss_calendar->removeBucket(to_delete);
  }
  else
  {
    // Create a brand new calendar
    ss_calendar = new CalendarDefault();
    ss_calendar->setName("SS for " + b->getName());
    ss_calendar->setSource(SOURCE_IP);
  }
  CalendarBucket *roq_calendar_bucket = NULL;
  CalendarBucket *ss_calendar_bucket = NULL;

  // Variables for the output metrics
  double calculated_roq = 1;
  double calculated_ss = 0;

  // Loop over all buckets in the horizon
  Date startdate = Plan::instance().getCurrent() - Duration(horizon_start * 86400L);
  Date enddate = Plan::instance().getCurrent() + Duration(horizon_end * 86400L);
  Date bucketstart;
  bool firstBucket = true;
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

    // Get the demand per day, measured over a period (which can be lead time,
    // roq min period of cover, or ss min period of cover).
    // Demand in buckets which are completely within the period is all added.
    // The last bucket will be only partially within the period. We take the
    // complete demand in that last bucket and add a proportion of it to the
    // demand.
    double demand_lt = 0.0;
    double lastdemand_lt = 0.0;
    Date fence_lt = bucketstart + leadtime;
    double demand_roq_poc = 0.0;
    double lastdemand_roq_poc = 0.0;
    Date fence_roq_poc = bucketstart + roq_min_poc;
    double demand_ss_poc = 0.0;
    double lastdemand_ss_poc = 0.0;
    Date fence_ss_poc = bucketstart + ss_min_poc;

    Date last_bucket_start_lt;
    Date last_bucket_end_lt;
    Date last_bucket_start_roq_poc;
    Date last_bucket_end_roq_poc;
    Date last_bucket_start_ss_poc;
    Date last_bucket_end_ss_poc;
    Date prev;
    for (Calendar::EventIterator tmp(cal, bucketstart, true);
      !last_bucket_start_lt || !last_bucket_start_roq_poc || !last_bucket_start_ss_poc;
      ++tmp)
    {
      if (prev <= fence_lt && tmp.getDate() >= fence_lt)
      {
        last_bucket_start_lt = prev;
        last_bucket_end_lt = tmp.getDate();
      }
      if (prev <= fence_roq_poc && tmp.getDate() >= fence_roq_poc)
      {
        last_bucket_start_roq_poc = prev;
        last_bucket_end_roq_poc = tmp.getDate();
      }
      if (prev <= fence_ss_poc && tmp.getDate() >= fence_ss_poc)
      {
        last_bucket_start_ss_poc = prev;
        last_bucket_end_ss_poc = tmp.getDate();
      }
      prev = tmp.getDate();
    }
    for (Buffer::flowplanlist::const_iterator i = b->getFlowPlans().begin();
      i != b->getFlowPlans().end(); ++i)
    {
      // Only consider consumption
      if (i->getEventType() != 1 || i->getQuantity() >= 0)
        continue;
      unsigned short beyond = 0;
      if (i->getDate() < last_bucket_end_lt)
      {
        if (i->getDate() >= last_bucket_start_lt)
          lastdemand_lt -= i->getQuantity();
        else if (i->getDate() >= bucketstart)
          demand_lt -= i->getQuantity();
      }
      else
        ++beyond;
      if (i->getDate() < last_bucket_end_roq_poc)
      {
        if (i->getDate() >= last_bucket_start_roq_poc)
          lastdemand_roq_poc -= i->getQuantity();
        else if (i->getDate() >= bucketstart)
          demand_roq_poc -= i->getQuantity();
      }
      else
        ++beyond;
      if (i->getDate() < last_bucket_end_ss_poc)
      {
        if (i->getDate() >= last_bucket_start_ss_poc)
          lastdemand_ss_poc -= i->getQuantity();
        else if (i->getDate() >= bucketstart)
          demand_ss_poc -= i->getQuantity();
      }
      else
        ++beyond;
      if (beyond == 3)
        break;
    }

    if (lastdemand_lt > 0)
      demand_lt += lastdemand_lt
        * (fence_lt - last_bucket_start_lt)
        / (last_bucket_end_lt - last_bucket_start_lt);
    if (lastdemand_roq_poc > 0)
      demand_roq_poc += lastdemand_roq_poc
      * (fence_roq_poc - last_bucket_start_roq_poc)
      / (last_bucket_end_roq_poc - last_bucket_start_roq_poc);
    if (lastdemand_ss_poc > 0)
      demand_ss_poc += lastdemand_ss_poc
      * (fence_ss_poc - last_bucket_start_ss_poc)
      / (last_bucket_end_ss_poc - last_bucket_start_ss_poc);

    if (leadtime)
      // Normal case
      demand_lt = demand_lt / leadtime * 86400;
    else
      // Special case when the leadtime is zero. We then consider the
      // average demand of the current bucket.
      demand_lt = demand_lt / static_cast<long>(last_bucket_end_lt - last_bucket_start_lt) * 86400;
    if (roq_min_poc)
      // Normal case
      demand_roq_poc = demand_roq_poc / roq_min_poc * 86400;
    else
      // Special case when the period of cover is zero. We then use the same
      // demand as measured over the leadtile.
      demand_roq_poc = demand_lt;
    if (ss_min_poc)
      // Normal case
      demand_ss_poc = demand_ss_poc / ss_min_poc * 86400;
    else
      // Special case when the period of cover is zero. We then use the same
      // demand as measured over the leadtile.
      demand_ss_poc = demand_lt;

    // Compute the reorder quantity
    // 1. start with the wilson formula for the optimal reorder quantity
    // 2. apply the minimum constraints in quantity and period of cover
    // 3. round up to the next roq_multiple, if specified
    // 4. apply the maximum constraints in quantity and period of cover, and
    //    round down when doing so.
    double roq = 1.0;
    if (price && (roq_type == "combined" || roq_type == "calculated" || firstBucket))
    {
      double tmp = ceil(sqrt(2 * 365 * demand_lt * fixed_order_cost / holding_cost / price));
      if (roq_type == "combined" || roq_type == "calculated")
        roq = tmp;
      if (firstBucket)
        // Remember the economic order metric in the first bucket
        calculated_roq = tmp;
    }
    if (roq < roq_min_qty && (roq_type == "combined" || roq_type == "quantity"))
      roq = roq_min_qty;
    if (roq_type == "combined" || roq_type == "periodofcover")
    {
      double tmp = ceil(demand_roq_poc * roq_min_poc / 86400);
      if (roq < tmp)
        roq = tmp;
    }
    if (roq_multiple > 0)
      roq = ceil(roq_multiple * static_cast<int>(roq / roq_multiple + 0.99999999));
    if (roq > roq_max_qty && (roq_type == "combined" || roq_type == "quantity"))
    {
      roq = roq_max_qty;
      if (roq_multiple > 0)
        roq = floor(roq_multiple * static_cast<int>(roq / roq_multiple));
    }
    if (roq_type == "combined" || roq_type == "periodofcover")
    {
      double tmp = demand_roq_poc * roq_max_poc / 86400;
      if (roq > tmp)
      {
        roq = tmp;
        if (roq_multiple > 0)
          roq = ceil(roq_multiple * static_cast<int>(roq / roq_multiple));
      }
    }
    if (roq < 1.0)
      roq = 1.0;

    // Compute the safety stock
    // 1. start with the value based on the desired service level
    // 2. apply the minimum constraints in quantity and period of cover
    // 3. round up to the next roq_multiple, if specified
    // 4. apply the maximum constraints in quantity and period of cover, and
    //    round down when doing so.
    double ss = 0.0;
    if (service_level > 0 && (ss_type == "combined" || ss_type == "calculated" || firstBucket))
    {
      // We compute the service level with the minimum of the replenishment
      // lead time and the expected time to consume the ROQ quantity.
      // This is used to consider situations with multiple replenishments
      // over the lead time.
      Duration tmp_leadtime = demand_roq_poc ? static_cast<long>(roq / demand_roq_poc * 86400) : 0L;
      if (leadtime < tmp_leadtime)
        tmp_leadtime = leadtime;
      double tmp = calulateStockLevel(
        // Argument 1: demand over the "risk period", which is the max of the
        // lead time and the time between replenishments.
        demand_lt * tmp_leadtime / 86400,
        // Argument 2: Variation, as the sum of the demand variation
        // over the risk period and the variation caused by the lead time
        // variance
        demand_deviation * demand_deviation * tmp_leadtime / bucket_size
        + leadtime_deviation * leadtime_deviation * demand_lt * demand_lt,
        // Argument 3: Reorder quantity
        static_cast<int>(ceil(roq)),
        // Argument 4: Fill rate minimum
        service_level/100,
        // Argument 5: Fill rate maximum
        1,
        // Argument 6: Minimum is the strongest
        true,
        // Argument 7: Distribution
        dist
        );
      tmp -= demand_lt * tmp_leadtime / 86400;
      if (tmp < 0)
        tmp = 0;
      if (firstBucket)
        calculated_ss = tmp;
      if (ss_type == "combined" || ss_type == "calculated")
        ss = tmp;
    }
    if (ss < ss_min_qty && (ss_type == "combined" || ss_type == "quantity"))
      ss = ss_min_qty;
    if (ss_type == "combined" || ss_type == "periodofcover")
    {
      double tmp = ceil(demand_ss_poc * ss_min_poc / 86400);
      if (ss < tmp)
        ss = tmp;
    }
    if (ss_multiple > 0)
      ss = ceil(ss_multiple * static_cast<int>(ss / ss_multiple + 0.99999999));
    if (ss > ss_max_qty && (ss_type == "combined" || ss_type == "quantity"))
    {
      ss = ss_max_qty;
      if (ss_multiple > 0)
        ss = ceil(ss_multiple * static_cast<int>(ss / ss_multiple));
    }
    if (ss_type == "combined" || ss_type == "periodofcover")
    {
      double tmp = demand_ss_poc * ss_max_poc;
      if (ss > tmp)
      {
        ss = tmp;
        if (ss_multiple > 0)
          ss = ceil(ss_multiple * static_cast<int>(ss / ss_multiple));
      }
    }
    double rop = ss + demand_lt * leadtime / 86400;

    // Final result
    if (loglevel > 2)
      logger << "demand: " << demand_lt << " / " << demand_roq_poc << " / " << demand_ss_poc
        << ", roq: " << roq
        << ", ss: " << ss
        << ", rop: " << rop << endl;

    // Store the metrics in the first bucket.
    // The values are averaged over the lead time.
    if (firstBucket && Plan::instance().getCurrent() >= bucketstart && Plan::instance().getCurrent() < bucketend)
    {
      firstBucket = false;
      PythonData res;
      res.setDouble(leadtime);
      const_cast<Buffer*>(b)->setProperty("ip_leadtime", res, 3);
      res.setDouble(calculated_roq);
      const_cast<Buffer*>(b)->setProperty("ip_calculated_roq", res, 3);
      res.setDouble(calculated_ss);
      const_cast<Buffer*>(b)->setProperty("ip_calculated_ss", res, 3);
      res.setDouble(roq);
      const_cast<Buffer*>(b)->setProperty("ip_roq", res, 3);
      res.setDouble(ss);
      const_cast<Buffer*>(b)->setProperty("ip_ss", res, 3);
      res.setDouble(demand_lt);
      const_cast<Buffer*>(b)->setProperty("ip_demand", res, 3);
      if (dist == AUTOMATIC)
      {
        distribution choosen = chooseDistribution(
          demand_lt * leadtime / 86400,
          demand_deviation * demand_deviation
          );
        if (choosen == POISSON)
          res.setString("Poisson");
        else if (choosen == NORMAL)
          res.setString("Normal");
        else if (choosen == NEGATIVE_BINOMIAL)
          res.setString("Negative Binomial");
        else
          throw LogicException("Distribution not recognized");
      }
      else
        res.setString(distname);
      const_cast<Buffer*>(b)->setProperty("ip_distribution", res, 4);
    }

    // Store the result on the ROQ and SS calendars
    if (!ss_calendar_bucket || fabs(ss_calendar_bucket->getValue() - ss) > ROUNDING_ERROR)
    {
      if (ss_calendar_bucket)
        ss_calendar_bucket->setEnd(bucketstart);
      ss_calendar_bucket = ss_calendar->addBucket(bucketstart, Date::infiniteFuture, ss);
      ss_calendar_bucket->setSource(SOURCE_IP);
      ss_calendar_bucket->setPriority(999);
    }
    if (!roq_calendar_bucket || fabs(roq_calendar_bucket->getValue() - roq) > ROUNDING_ERROR)
    {
      if (roq_calendar_bucket)
        roq_calendar_bucket->setEnd(bucketstart);
      roq_calendar_bucket = roq_calendar->addBucket(bucketstart, Date::infiniteFuture, roq);
      roq_calendar_bucket->setSource(SOURCE_IP);
      roq_calendar_bucket->setPriority(999);
    }

    // Prepare for the next bucket
    bucketstart = bucket.getDate();
  }

  // Associate the new or updated created calendars
  if (oper)
  {
    if (oper->getSizeMinimumCalendar())
      oper->setSizeMinimumCalendar(NULL);
    oper->setSizeMinimumCalendar(roq_calendar);
  }
  if (b->getMinimumCalendar())
    const_cast<Buffer*>(b)->setMinimumCalendar(NULL);
  const_cast<Buffer*>(b)->setMinimumCalendar(ss_calendar);
}


double InventoryPlanningSolver::computeStockOutProbability(const Buffer* b){

  short loglevel = getLogLevel();
  if (loglevel > 1)
    logger << "Computing stockout probability on buffer " << b << endl;


  Duration leadtime = getBufferLeadTime(b);
  Date currentDate = Plan::instance().getCurrent();
  double stockOutProbability, maxStockOutProbability = 0;
  double demand = 0;


  Date bucketStart;
  for (Calendar::EventIterator tmp(cal, currentDate, true);;
    ++tmp)
  {
    Date bucketEnd = tmp.getDate();

    // We are before current, let's move on
    if (bucketEnd <= currentDate)
    {
      bucketStart = bucketEnd;
      continue;
    }

    // We perform the bucket where the leadtime falls and then we stop
    if (bucketStart >= currentDate + leadtime)
      break;

    // on_hand at the end of the bucket or at the leadtime
    double on_hand = b->getOnHand(min(bucketEnd,currentDate+leadtime));
    // If on-hand at end of bucket is 0, no need to go further
    // StockOut probability is 100% whatever the demand is
    if (on_hand <= 0) {
      return 1;
    }

    //for the current bucket, we need to find out the demand
    for (Buffer::flowplanlist::const_iterator i = b->getFlowPlans().begin();
      i != b->getFlowPlans().end(); ++i) {
        // Only consider consumption
        if (i->getEventType() != 1 || i->getQuantity() >= 0)
          continue;
        // Calculate quantity or remaining quantity if current date is not at the beginning of month until leadtime
        if (i->getDate() >= max(bucketStart,currentDate) &&  i->getDate() < min(bucketEnd,currentDate+leadtime))
          demand -= i->getQuantity();
    }

    // If there is no demand so far, the stockout probability is 0%
    // so we can go to next bucket
    if (demand = 0)
      continue;
    else
      // on_hand is the TSL so ROP = TSL-1 and ROQ = 1
      stockOutProbability = 1 - calculateFillRate(demand,demand,int(on_hand)-1,1,AUTOMATIC);


    if (loglevel > 2)
      logger << "Stockout probability for bucket starting on " << bucketStart << " : " << stockOutProbability << endl;


    maxStockOutProbability = max(maxStockOutProbability,stockOutProbability);

    if (loglevel > 2)
      logger << "Greatest stockout probability so far : " << maxStockOutProbability << endl;

    bucketStart = bucketEnd;
  }

  return maxStockOutProbability;
}


PyObject* InventoryPlanningSolver::computeStockoutProbability(PyObject *self, PyObject *args)
{
  // Parse the argument
  PyObject *buf = NULL;
  if (args && !PyArg_ParseTuple(args, "|O:computeStockoutRisk", &buf))
    return NULL;
  if (buf && !PyObject_TypeCheck(buf, Buffer::metadata->pythonClass))
  {
    PyErr_SetString(PythonDataException, "solve(d) argument must be a buffer");
    return NULL;
  }

  try
  {
    InventoryPlanningSolver *solver = static_cast<InventoryPlanningSolver*>(self);
    return Py_BuildValue(
      "d", solver->computeStockOutProbability(static_cast<Buffer*>(buf))
      );
  }
  catch(...)
  {
    PythonType::evalException();
    return NULL;
  }
}


/************************************************************************************************************
function calculateStockLevel
This function will compute a rop respecting the minimum fill rate lower bound.
The algorithm will start from a rop equal to 0 and will increment it until the minimum fill rate is respected.
It will then check if maximum fill rate is respected. If yes then the calculated rop is returned.
If no, it will check the value of the boolean minimumStrongest. If true, the calculated rop is returned else
the calculated rop-1 is returned. The calculated rop-1 will respect the maximum fill rate.

mean : average demand during lead time
variance : the demand variance
roq : reorder quantity
fillRateMinimum : The minimum fill rate
fillRateMaximum : The maximum fill rate
minimumStrongest : The algorithm will start from a rop equal to 0 and will increment it until
**************************************************************************************************************/
int InventoryPlanningSolver::calulateStockLevel(
  double mean, double variance, int roq, double fillRateMinimum,
  double fillRateMaximum, bool minimumStrongest, distribution dist
  )
{
  /* Checks that the fill rates are between 0 and 1*/
  if (fillRateMinimum < 0)
    fillRateMinimum = 0;

  if (fillRateMaximum > 1)
    fillRateMaximum = 1;
  // TODO Below code is definitely not optimal, we might think of coding a dichotomical approach
  // or think of a formula giving the stock level based on the fill rate without iterating
  unsigned int rop = static_cast<int>(floor(mean));
  double fillRate;

  // Compute the fill rate, either based on the average inventory or based on the safety stock only
  while ((fillRate = calculateFillRate(mean, variance, rop, service_level_on_average_inventory ? roq : 1, dist)) < fillRateMinimum)
    ++rop;

  // Now we are sure the that lower bound is respected, what about the upper bound
  if (minimumStrongest == true || fillRate <= fillRateMaximum)
    return rop;
  else
    return rop - 1;
}


distribution InventoryPlanningSolver::chooseDistribution(
  double mean, double variance
  )
{
  // When the average is sufficiently large, any distribution can efficiently
  // be approximated with a normal distribution.
  // See http://www.stat.ucla.edu/~dinov/courses_students.dir/Applets.dir/NormalApprox2PoissonApplet.html
  if (mean >= 20)
    return NORMAL;

  double varianceToMean = variance/mean;
  if (varianceToMean > 1.1)
    /* If the variance to mean ratio is greater than 1.1, we switch to negative binomial */
    return NEGATIVE_BINOMIAL;
  else
    /* Else apply a Poisson distribution. */
    return POISSON;
}


/************************************************************************************************************
function calculateFillRate
This function returns the fill rate given a mean, a rop and a roq

mean : average demand during lead time
rop reorder point
roq : reorder quantity
@return : a double between 0 and 1
**************************************************************************************************************/
double InventoryPlanningSolver::calculateFillRate(
  double mean, double variance, int rop, int roq, distribution dist
  )
{
  if (mean <= 0)
    return 1;

  if (dist == AUTOMATIC)
    dist = chooseDistribution(mean, variance);

  if (dist == POISSON)
  {
    if (mean >= 20)
      // A poisson distribution is very close to a normal distribution for
      // large input values. And a normal distribution can be computed much
      // faster.
      return NormalDistribution::calculateFillRate(mean, variance, rop, roq);
    else
      return PoissonDistribution::calculateFillRate(mean, rop, roq);
  }
  else if (dist == NORMAL)
    return NormalDistribution::calculateFillRate(mean, variance, rop, roq);
  else if (dist == NEGATIVE_BINOMIAL)
  {
    if (mean >= 20)
      // A negative binomial distribution is very close to a normal distribution
      // for large input values. And a normal distribution can be computed much
      // faster.
      return NormalDistribution::calculateFillRate(mean, variance, rop, roq);
    else
      return NegativeBinomialDistribution::calculateFillRate(mean, variance, rop, roq);
  }
  else
    throw DataException("Invalid distribution");
}


}       // end namespace
