/***************************************************************************
 *                                                                         *
 * Copyright (C) 2012-2015 by frePPLe bvba                                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "forecast.h"

namespace module_forecast
{

Forecast::MapOfForecasts Forecast::ForecastDictionary;
unsigned long Forecast::Forecast_Iterations(15L);
double Forecast::Forecast_SmapeAlfa(0.95);
unsigned long Forecast::Forecast_Skip(5);

const Keyword Forecast::tag_planned("planned");
const Keyword Forecast::tag_methods("methods");
const Keyword Forecast::tag_method("method");
const Keyword ForecastBucket::tag_weight("weight");
const Keyword ForecastBucket::tag_total("total");
const Keyword ForecastBucket::tag_consumed("consumed");
const Keyword ForecastBucket::tag_forecast("forecast");
const MetaClass *Forecast::metadata;
const MetaClass *ForecastBucket::metadata;
const MetaCategory* ForecastBucket::metacategory;
short ForecastBucket::DueWithinBucket = 1;
const string ForecastBucket::DUEATSTART = "start";
const string ForecastBucket::DUEATMIDDLE = "middle";
const string ForecastBucket::DUEATEND = "end";

int Forecast::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<Forecast>(
    "demand", "demand_forecast", Object::create<Forecast>
    );
  registerFields<Forecast>(const_cast<MetaClass*>(metadata));

  // Get notified when a calendar is deleted
  FunctorStatic<Calendar,Forecast>::connect(SIG_REMOVE);

  // Initialize the Python class
  FreppleClass<Forecast, Demand>::getPythonType().addMethod(
    "setQuantity", Forecast::setPythonTotalQuantity, METH_VARARGS,
    "Update the total quantity in one or more buckets"
    );
  return FreppleClass<Forecast,Demand>::initialize();
}


int ForecastBucket::initialize()
{
  // Initialize the metadata
  // No factory method for this class
  metacategory = MetaCategory::registerCategory<ForecastBucket>(
    "forecastbucket", "forecastbuckets", reader
    );
  metadata = MetaClass::registerClass<ForecastBucket>(
    "forecastbucket", "demand_forecastbucket"
    );
  registerFields<ForecastBucket>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  // No support for creation
  PythonType& x = FreppleClass<ForecastBucket, Demand>::getPythonType();
  x.setName("demand_forecastbucket");
  x.setDoc("frePPLe forecastbucket");
  x.supportgetattro();
  x.supportsetattro();
  x.supportstr();
  x.supportcompare();
  x.setBase(Demand::metadata->pythonClass);
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


Object* ForecastBucket::reader(const MetaClass* cat, const DataValueDict& in)
{
  // Pick up the forecast attribute. An error is reported if it's missing.
  const DataValue* fcstElement = in.get(ForecastBucket::tag_forecast);
  if (!fcstElement)
    throw DataException("Missing forecast field");
  Object* fcstobject = fcstElement->getObject();
  if (!fcstobject || fcstobject->getType() != *Forecast::metadata)
    throw DataException("Invalid forecast field");

  // Pick up the start and end date. At least one of these needs to be
  // provided.
  const DataValue* strtElement = in.get(Tags::start);
  const DataValue* ndElement = in.get(Tags::end);
  Date strt;
  Date nd;
  if (strtElement)
    strt = strtElement->getDate();
  if (ndElement)
    nd = ndElement->getDate();
  if (!strt && !nd)
    throw DataException("Start and/or end date must be provided");

  // Pick up the total value
  const DataValue* totalElement = in.get(ForecastBucket::tag_total);
  if (totalElement)
  {
    static_cast<Forecast*>(fcstobject)->setTotalQuantity(
      DateRange(strt, nd),
      totalElement->getDouble()
      );
  }
  return NULL;
}


ForecastBucket::ForecastBucket(Forecast* f, Date d, Date e, double w, ForecastBucket* p)
  : weight(w), consumed(0.0), total(0.0), timebucket(d, e),
  prev(p), next(NULL)
{
  setName(f->getName() + " - " + string(d));
  if (p) p->next = this;
  setOwner(f);
  setHidden(true);  // Avoid the subdemands show up in the list of demands
  setItem(f->getItem());
  setCustomer(f->getCustomer());
  switch (DueWithinBucket)
  {
    case 0: // Start
      setDue(d);
      break;
    case 1: // Middle
      {
      Date tmp = d + Duration((e - d) / 2);
      tmp -= tmp.getSecondsDay();  // Truncate to the start of the day
      setDue(tmp);
      break;
      }
    case 2: // End
      setDue(e);
      break;
  }
  setPriority(f->getPriority());
  setMaxLateness(f->getMaxLateness());
  setMinShipment(f->getMinShipment());
  setOperation(f->getOperation());
  setLocation(f->getLocation());
  initType(metadata);
}


bool Forecast::callback(Calendar* l, const Signal a)
{
  // This function is called when a calendar is about to be deleted.
  // If that calendar is being used for a forecast we reset the calendar
  // pointer to null.
  for (MapOfForecasts::iterator x = ForecastDictionary.begin();
      x != ForecastDictionary.end(); ++x)
    if (x->second->calptr == l)
      // Calendar in use for this forecast
      x->second->calptr = NULL;
  return true;
}


Forecast::~Forecast()
{
  // Update the dictionary
  for (MapOfForecasts::iterator x=
      ForecastDictionary.lower_bound(make_pair(&*getItem(),&*getCustomer()));
      x != ForecastDictionary.end(); ++x)
    if (x->second == this)
    {
      ForecastDictionary.erase(x);
      break;
    }

  // Delete all children demands
  ForecastBucket::bucketiterator iter(this);
  while (ForecastBucket* bckt = iter.next())
    delete bckt;
}


void Forecast::instantiate()
{
  if (!calptr)
    throw DataException("Missing forecast calendar");

  // Create a demand for every bucket
  ForecastBucket* prev = NULL;
  Date prevDate;
  double prevValue(0.0);
  for (Calendar::EventIterator i(calptr); i.getDate() <= Date::infiniteFuture; ++i)
  {
    if ((prevDate || i.getDate() == Date::infiniteFuture) && prevValue > 0.0)
      prev = new ForecastBucket(this, prevDate, i.getDate(), prevValue, prev);
    if (i.getDate() == Date::infiniteFuture)
      break;
    prevDate = i.getDate();
    prevValue = i.getValue();
  }
}


void Forecast::setDiscrete(const bool b)
{
  // Update the flag
  discrete = b;

  // Round down any forecast demands that may already exist.
  if (discrete)
    for (memberIterator m = getMembers(); m!=end(); ++m)
      m->setQuantity(floor(m->getQuantity()));
}


void Forecast::setTotalQuantity(const DateRange& d, double f, bool add)
{
  // Initialize, if not done yet
  if (!isGroup()) instantiate();

  // Find all forecast demands, and sum their weights
  double weights = 0.0;
  for (memberIterator m = getMembers(); m!=end(); ++m)
  {
    ForecastBucket* x = dynamic_cast<ForecastBucket*>(&*m);
    if (!x)
      throw DataException("Invalid subdemand of forecast '" + getName() +"'");
    if (d.intersect(x->getDueRange()))
    {
      // Bucket intersects with daterange
      if (!d.getDuration())
      {
        // Single date provided. Update that one bucket.
        if (add) x->incTotal(f);
        else x->setTotal(f);
        return;
      }
      weights += x->getWeight() * static_cast<long>(x->getDueRange().overlap(d));
    }
  }

  // Expect to find at least one non-zero weight...
  if (!weights)
  {
    ostringstream o;
    o << "No valid forecast date in range " << d
      << " of forecast '" << getName() << "'";
    throw DataException(o.str());
  }

  // Update the forecast quantity, respecting the weights
  f /= weights;
  double carryover = 0.0;
  for (memberIterator m = getMembers(); m!=end(); ++m)
  {
    ForecastBucket* x = dynamic_cast<ForecastBucket*>(&*m);
    if (d.intersect(x->getDueRange()))
    {
      // Bucket intersects with daterange
      Duration o = x->getDueRange().overlap(d);
      double percent = x->getWeight() * static_cast<long>(o);
      if (getDiscrete())
      {
        // Rounding to discrete numbers
        carryover += f * percent;
        double intdelta = ceil(carryover - 0.5);
        carryover -= intdelta;
        if (o < x->getDueRange().getDuration() || add)
          // The bucket is only partially updated
          x->incTotal(intdelta);
        else
          // The bucket is completely updated
          x->setTotal(intdelta);
      }
      else
      {
        // No rounding
        if (o < x->getDueRange().getDuration() || add)
          // The bucket is only partially updated
          x->incTotal(f * percent);
        else
          // The bucket is completely updated
          x->setTotal(f * percent);
      }
    }
  }
}


void Forecast::setTotalQuantity(const Date d, double f, bool add)
{
  // Initialize, if not done yet
  if (!isGroup()) instantiate();

  // Find the bucket
  for (memberIterator m = getMembers(); m!=end(); ++m)
  {
    ForecastBucket* x = dynamic_cast<ForecastBucket*>(&*m);
    if (!x)
      throw DataException("Invalid subdemand of forecast '" + getName() +"'");
    if (x->getDueRange().within(d))
    {
      // Update the bucket
      if (add) x->incTotal(f);
      else x->setTotal(f);
      return;
    }
  }
}


void Forecast::setCalendar(Calendar* c)
{
  if (isGroup())
    throw DataException(
      "Changing the calendar of an initialized forecast isn't allowed");
  calptr = c;
}


void Forecast::setItem(Item* i)
{
  // No change
  if (getItem() == i) return;

  // Update the dictionary
  for (MapOfForecasts::iterator x =
      ForecastDictionary.lower_bound(make_pair(
          &*getItem(),&*getCustomer()
          ));
      x != ForecastDictionary.end(); ++x)
    if (x->second == this)
    {
      ForecastDictionary.erase(x);
      break;
    }
  ForecastDictionary.insert(make_pair(make_pair(i,&*getCustomer()),this));

  // Update data field
  Demand::setItem(i);

  // Update the item for all buckets/subdemands
  for (memberIterator m = getMembers(); m!=end(); ++m)
    m->setItem(i);
}


void Forecast::setCustomer(Customer* i)
{
  // No change
  if (getCustomer() == i) return;

  // Update the dictionary
  for (MapOfForecasts::iterator x =
      ForecastDictionary.lower_bound(make_pair(
          getItem(), getCustomer()
          ));
      x != ForecastDictionary.end(); ++x)
    if (x->second == this)
    {
      ForecastDictionary.erase(x);
      break;
    }
  ForecastDictionary.insert(make_pair(make_pair(&*getItem(),i),this));

  // Update data field
  Demand::setCustomer(i);

  // Update the customer for all buckets/subdemands
  for (memberIterator m = getMembers(); m!=end(); ++m)
    m->setCustomer(i);
}


void Forecast::setMaxLateness(Duration i)
{
  Demand::setMaxLateness(i);
  // Update the maximum lateness for all buckets/subdemands
  for (memberIterator m = getMembers(); m!=end(); ++m)
    m->setMaxLateness(i);
}


void Forecast::setMinShipment(double i)
{
  Demand::setMinShipment(i);
  // Update the minimum shipment for all buckets/subdemands
  for (memberIterator m = getMembers(); m!=end(); ++m)
    m->setMinShipment(i);
}


void Forecast::setPriority(int i)
{
  Demand::setPriority(i);
  // Update the priority for all buckets/subdemands
  for (memberIterator m = getMembers(); m!=end(); ++m)
    m->setPriority(i);
}


void Forecast::setOperation(Operation *o)
{
  Demand::setOperation(o);
  // Update the priority for all buckets/subdemands
  for (memberIterator m = getMembers(); m!=end(); ++m)
    m->setOperation(o);
}


extern "C" PyObject* Forecast::setPythonTotalQuantity(PyObject *self, PyObject *args)
{
  try
  {
    // Get the forecast model
    Forecast* forecast = static_cast<Forecast*>(self);

    // Parse the Python arguments
    double value;
    PyObject* pystart;
    PyObject* pyend = NULL;
    PyObject* pyadd = NULL;
    int ok = PyArg_ParseTuple(args, "dO|OO:setQuantity", &value, &pystart, &pyend, &pyadd);
    if (!ok) return NULL;

    // Update the forecast
    PythonData start(pystart), end(pyend), add(pyadd);
    if (pyend)
      forecast->setTotalQuantity(DateRange(start.getDate(), end.getDate()), value, add.getBool());
    else
      forecast->setTotalQuantity(start.getDate(), value, add.getBool());
  }
  catch(...)
  {
    PythonType::evalException();
    return NULL;
  }
  return Py_BuildValue("");
}


extern "C" PyObject* ForecastSolver::timeseries(PyObject *self, PyObject *args)
{
  // Get the forecast model
  ForecastSolver* solver = static_cast<ForecastSolver*>(self);

  // Parse the Python arguments
  PyObject* fcst;
  PyObject* history;
  PyObject* buckets = NULL;
  int ok = PyArg_ParseTuple(args, "OO|O:timeseries", &fcst, &history, &buckets);
  if (!ok) return NULL;

  // Verify the object type
  PythonData pyfcst(fcst);
  if (!pyfcst.check(Forecast::metadata))
  {
    PyErr_SetString(PythonDataException, "first argument must be of type forecast");
    return NULL;
  }

  // Verify we can iterate over the arguments
  PyObject *historyiterator = PyObject_GetIter(history);
  PyObject *bucketiterator = NULL;
  if (!historyiterator)
  {
    PyErr_Format(PyExc_AttributeError,"Invalid type for time series");
    return NULL;
  }
  if (buckets) bucketiterator = PyObject_GetIter(buckets);
  if (!bucketiterator)
  {
    PyErr_Format(PyExc_AttributeError,"Invalid type for time series");
    return NULL;
  }

  // Copy the history data into a C++ data structure
  double data[300];
  unsigned int historycount = 0;
  PyObject *item;
  while ((item = PyIter_Next(historyiterator)))
  {
    data[historycount++] = PyFloat_AsDouble(item);
    Py_DECREF(item);
    if (historycount>=300) break;
  }
  Py_DECREF(historyiterator);

  // Copy the bucket data into a C++ data structure
  Date bucketdata[300];
  unsigned int bucketcount = 0;
  while ((item = PyIter_Next(bucketiterator)))
  {
    bucketdata[bucketcount++] = PythonData(item).getDate();
    Py_DECREF(item);
    if (bucketcount>=300) break;
  }
  Py_DECREF(bucketiterator);

  Py_BEGIN_ALLOW_THREADS  // Free the Python interpreter for other threads
  try
  {
    // Generate the forecast
    static_cast<Forecast*>(fcst)->generateFutureValues
      (data, historycount, bucketdata, bucketcount, solver);
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Release the Python interpreter
  return Py_BuildValue("");
}


}       // end namespace
