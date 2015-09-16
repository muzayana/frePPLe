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

const MetaClass *ForecastSolver::metadata;
bool ForecastSolver::Customer_Then_Item_Hierarchy = true;
bool ForecastSolver::Match_Using_Delivery_Operation = true;
Duration ForecastSolver::Net_Late(0L);
Duration ForecastSolver::Net_Early(0L);

const Keyword ForecastSolver::tag_DueAtEndOfBucket("DueAtEndOfBucket");
const Keyword ForecastSolver::tag_Net_CustomerThenItemHierarchy("Net_CustomerThenItemHierarchy");
const Keyword ForecastSolver::tag_Net_MatchUsingDeliveryOperation("Net_MatchUsingDeliveryOperation");
const Keyword ForecastSolver::tag_Net_NetEarly("Net_NetEarly");
const Keyword ForecastSolver::tag_Net_NetLate("Net_NetLate");
const Keyword ForecastSolver::tag_Iterations("Iterations");
const Keyword ForecastSolver::tag_SmapeAlfa("SmapeAlfa");
const Keyword ForecastSolver::tag_Skip("Skip");
const Keyword ForecastSolver::tag_MovingAverage_order("MovingAverage_order");
const Keyword ForecastSolver::tag_SingleExponential_initialAlfa("SingleExponential_initialAlfa");
const Keyword ForecastSolver::tag_SingleExponential_minAlfa("SingleExponential_minAlfa");
const Keyword ForecastSolver::tag_SingleExponential_maxAlfa("SingleExponential_maxAlfa");
const Keyword ForecastSolver::tag_DoubleExponential_initialAlfa("DoubleExponential_initialAlfa");
const Keyword ForecastSolver::tag_DoubleExponential_minAlfa("DoubleExponential_minAlfa");
const Keyword ForecastSolver::tag_DoubleExponential_maxAlfa("DoubleExponential_maxAlfa");
const Keyword ForecastSolver::tag_DoubleExponential_initialGamma("DoubleExponential_initialGamma");
const Keyword ForecastSolver::tag_DoubleExponential_minGamma("DoubleExponential_minGamma");
const Keyword ForecastSolver::tag_DoubleExponential_maxGamma("DoubleExponential_maxGamma");
const Keyword ForecastSolver::tag_DoubleExponential_dampenTrend("DoubleExponential_dampenTrend");
const Keyword ForecastSolver::tag_Seasonal_initialAlfa("Seasonal_initialAlfa");
const Keyword ForecastSolver::tag_Seasonal_minAlfa("Seasonal_minAlfa");
const Keyword ForecastSolver::tag_Seasonal_maxAlfa("Seasonal_maxAlfa");
const Keyword ForecastSolver::tag_Seasonal_initialBeta("Seasonal_initialBeta");
const Keyword ForecastSolver::tag_Seasonal_minBeta("Seasonal_minBeta");
const Keyword ForecastSolver::tag_Seasonal_maxBeta("Seasonal_maxBeta");
const Keyword ForecastSolver::tag_Seasonal_gamma("Seasonal_gamma");
const Keyword ForecastSolver::tag_Seasonal_dampenTrend("Seasonal_dampenTrend");
const Keyword ForecastSolver::tag_Seasonal_minPeriod("Seasonal_minPeriod");
const Keyword ForecastSolver::tag_Seasonal_maxPeriod("Seasonal_maxPeriod");
const Keyword ForecastSolver::tag_Seasonal_minAutocorrelation("Seasonal_minAutocorrelation");
const Keyword ForecastSolver::tag_Seasonal_maxAutocorrelation("Seasonal_maxAutocorrelation");
const Keyword ForecastSolver::tag_Croston_initialAlfa("Croston_initialAlfa");
const Keyword ForecastSolver::tag_Croston_minAlfa("Croston_minAlfa");
const Keyword ForecastSolver::tag_Croston_maxAlfa("Croston_maxAlfa");
const Keyword ForecastSolver::tag_Croston_minIntermittence("Croston_minIntermittence");
const Keyword ForecastSolver::tag_Outlier_maxDeviation("Outlier_maxDeviation");


int ForecastSolver::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<ForecastSolver>(
    "solver", "solver_forecast", Object::create<ForecastSolver>
    );
  registerFields<ForecastSolver>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  PythonType& x = FreppleClass<ForecastSolver, Solver>::getPythonType();
  x.setName("solver_forecast");
  x.setDoc("frePPLe solver_forecast");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("solve", Solver::solve, METH_NOARGS, "run the solver");
  x.addMethod(
    "timeseries", ForecastSolver::timeseries, METH_VARARGS,
    "Set the future based on the timeseries of historical data"
    );
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


PyObject* ForecastSolver::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Create the solver
    ForecastSolver *s = new ForecastSolver();

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


bool ForecastSolver::callback(Demand* l, const Signal a)
{
  // Call the netting function
  solve(l, NULL);

  // Always return 'okay'
  return true;
}


void ForecastSolver::solve(const Demand* l, void* v)
{
  // Forecast don't net themselves, and hidden demands either...
  if (!l || dynamic_cast<const Forecast*>(l) || l->getHidden()) return;

  // TODO Add also a location matching in the forecast netting

  // Message
  if (getLogLevel()>0)
    logger << "  Netting of demand '" << l << "'  ('" << l->getCustomer()
        << "', '" << l->getItem() << "', '" << l->getLocation()
        << "', '" << l->getDeliveryOperation()
        << "'): " << l->getDue() << ", " << l->getQuantity() << endl;

  // Find a matching forecast
  Forecast *fcst = matchDemandToForecast(l);

  if (!fcst)
  {
    // Message
    if (getLogLevel()>0)
      logger << "    No matching forecast available" << endl;
    return;
  }
  else if (getLogLevel()>0)
    logger << "    Matching forecast: " << fcst << endl;

  // Netting the order from the forecast
  netDemandFromForecast(l,fcst);
}


void ForecastSolver::solve(void *v)
{
  // Sort the demands using the same sort function as used for planning.
  // Note: the memory consumption of the sorted list can be significant
  sortedDemandList l;
  for (Demand::iterator i = Demand::begin(); i != Demand::end(); ++i)
    // Only sort non-forecast demand.
    if (!dynamic_cast<Forecast*>(&*i)
        && !dynamic_cast<ForecastBucket*>(&*i))
      l.insert(&*i);

  // Netting loop
  for(sortedDemandList::iterator i = l.begin(); i != l.end(); ++i)
    try {solve(*i, NULL);}
    catch (...)
    {
      // Error message
      logger << "Error: Caught an exception while netting demand '"
          << (*i)->getName() << "':" << endl;
      try {throw;}
      catch (const bad_exception&) {logger << "  bad exception" << endl;}
      catch (const exception& e) {logger << "  " << e.what() << endl;}
      catch (...) {logger << "  Unknown type" << endl;}
    }
}


Forecast* ForecastSolver::matchDemandToForecast(const Demand* l)
{
  pair<const Item*, const Customer*> key
    = make_pair(&*(l->getItem()), &*(l->getCustomer()));

  do  // Loop through second dimension
  {
    do // Loop through first dimension
    {
      Forecast::MapOfForecasts::iterator x = Forecast::ForecastDictionary.lower_bound(key);

      // Loop through all matching keys
      while (x != Forecast::ForecastDictionary.end() && x->first == key)
      {
        if (!getMatchUsingDeliveryOperation()
            || x->second->getDeliveryOperation() == l->getDeliveryOperation())
          // Bingo! Found a matching key, if required plus matching delivery operation
          return x->second;
        else
          ++ x;
      }
      // Not found: try a higher level match in first dimension
      if (Customer_Then_Item_Hierarchy)
      {
        // First customer hierarchy
        if (key.second) key.second = key.second->getOwner();
        else break;
      }
      else
      {
        // First item hierarchy
        if (key.first) key.first = key.first->getOwner();
        else break;
      }
    }
    while (true);

    // Not found at any level in the first dimension

    // Try a new level in the second dimension
    if (Customer_Then_Item_Hierarchy)
    {
      // Second is item
      if (key.first) key.first = key.first->getOwner();
      else return NULL;
      // Reset to lowest level in the first dimension again
      key.second = &*(l->getCustomer());
    }
    else
    {
      // Second is customer
      if (key.second) key.second = key.second->getOwner();
      else return NULL;
      // Reset to lowest level in the first dimension again
      key.first = &*(l->getItem());
    }
  }
  while (true);
}


void ForecastSolver::netDemandFromForecast(const Demand* dmd, Forecast* fcst)
{

  // Empty forecast model
  if (!fcst->isGroup())
  {
    if (getLogLevel()>1)
      logger << "    Empty forecast model" << endl;
    if (getLogLevel()>0 && dmd->getQuantity()>0.0)
      logger << "    Remains " << dmd->getQuantity() << " that can't be netted" << endl;
    return;
  }

  // Find the bucket with the due date
  ForecastBucket* zerobucket = NULL;
  ForecastBucket::bucketiterator i(fcst);
  while (zerobucket = i.next())
    if (zerobucket && zerobucket->getDueRange().within(dmd->getDue()))
      // Found...
      break;
  if (!zerobucket)
    throw LogicException("Can't find forecast bucket for "
        + string(dmd->getDue()) + " in forecast '" + fcst->getName() + "'");

  // Netting - looking for time buckets with net forecast
  double remaining = dmd->getQuantity();
  ForecastBucket* curbucket = zerobucket;
  bool backward = true;
  while ( remaining > 0 && curbucket
      && (dmd->getDue() - getNetEarly() < curbucket->getDueRange().getEnd())
      && (dmd->getDue() + getNetLate() >= curbucket->getDueRange().getStart())
        )
  {
    // Net from the current bucket
    double available = curbucket->getQuantity();
    if (available > 0)
    {
      if (available >= remaining)
      {
        // Partially consume a bucket
        if (getLogLevel()>1)
          logger << "    Consuming " << remaining << " from bucket "
              << curbucket->getDueRange() << " (" << available
              << " available)" << endl;
        curbucket->incConsumed(remaining);
        remaining = 0;
      }
      else
      {
        // Completely consume a bucket
        if (getLogLevel()>1)
          logger << "    Consuming " << available << " from bucket "
              << curbucket->getDueRange() << " (" << available
              << " available)" << endl;
        remaining -= available;
        curbucket->incConsumed(available);
      }
    }
    else if (getLogLevel()>1)
      logger << "    Nothing available in bucket "
          << curbucket->getDueRange() << endl;

    // Find the next forecast bucket
    if (backward)
    {
      // Moving to earlier buckets
      curbucket = curbucket->getPreviousBucket();
      if (!curbucket)
      {
        backward = false;
        curbucket = zerobucket->getNextBucket();
      }
    }
    else
      // Moving to later buckets
      curbucket = curbucket->getNextBucket();
  }

  // Quantity for which no bucket is found
  if (remaining > 0 && getLogLevel()>0)
    logger << "    Remains " << remaining << " that can't be netted" << endl;

}

}       // end namespace
