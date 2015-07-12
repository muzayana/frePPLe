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
      const MetaFieldBase* fmeta = SolverMRP::metadata->findField(attr.getHash());
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

  // Message
  if (getLogLevel()>0)
    logger << "  Netting of demand '" << l << "'  ('" << l->getCustomer()
        << "','" << l->getItem() << "', '" << l->getDeliveryOperation()
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
        if (!Forecast::getMatchUsingDeliveryOperation()
            || x->second->getDeliveryOperation() == l->getDeliveryOperation())
          // Bingo! Found a matching key, if required plus matching delivery operation
          return x->second;
        else
          ++ x;
      }
      // Not found: try a higher level match in first dimension
      if (Forecast::Customer_Then_Item_Hierarchy)
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
    if (Forecast::Customer_Then_Item_Hierarchy)
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
  for (Forecast::memberIterator i = fcst->getMembers(); i != fcst->end(); ++i)
  {
    zerobucket = dynamic_cast<ForecastBucket*>(&*i);
    if (zerobucket && zerobucket->getDueRange().within(dmd->getDue())) break;
  }
  if (!zerobucket)
    throw LogicException("Can't find forecast bucket for "
        + string(dmd->getDue()) + " in forecast '" + fcst->getName() + "'");

  // Netting - looking for time buckets with net forecast
  double remaining = dmd->getQuantity();
  ForecastBucket* curbucket = zerobucket;
  bool backward = true;
  while ( remaining > 0 && curbucket
      && (dmd->getDue()-Forecast::getNetEarly() < curbucket->getDueRange().getEnd())
      && (dmd->getDue()+Forecast::getNetLate() >= curbucket->getDueRange().getStart())
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
