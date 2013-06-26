/***************************************************************************
  file : $URL: file:///C:/Users/Johan/Dropbox/SVNrepository/frepple/addon/modules/forecast/pythonforecast.cpp $
  version : $LastChangedRevision: 449 $  $LastChangedBy: Johan $
  date : $LastChangedDate: 2012-12-28 18:59:56 +0100 (Fri, 28 Dec 2012) $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * Copyright (C) 2012 by Johan De Taeye, frePPLe bvba                      *
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


PyObject* Forecast::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_calendar))
    return PythonObject(getCalendar());
  else if (attr.isA(Tags::tag_discrete))
    return PythonObject(getDiscrete());
  return Demand::getattro(attr);
}


int Forecast::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_calendar))
  {
    if (!field.check(Calendar::metadata))
    {
      PyErr_SetString(PythonDataException, "forecast calendar must be of type calendar");
      return -1;
    }
    Calendar* y = static_cast<Calendar*>(static_cast<PyObject*>(field));
    setCalendar(y);
  }
  else if (attr.isA(Tags::tag_discrete))
    setDiscrete(field.getBool());
  else
    return Demand::setattro(attr, field);
  return 0; // OK
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
    int ok = PyArg_ParseTuple(args, "dO|O:setQuantity", &value, &pystart, &pyend);
    if (!ok) return NULL;

    // Update the forecast
    PythonObject start(pystart), end(pyend);
    if (pyend)
      forecast->setTotalQuantity(DateRange(start.getDate(), end.getDate()), value);
    else
      forecast->setTotalQuantity(start.getDate(), value);
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
  PythonObject pyfcst(fcst); 
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
    bucketdata[bucketcount++] = PythonObject(item).getDate();
    Py_DECREF(item);
    if (bucketcount>=300) break;
  }
  Py_DECREF(bucketiterator);

  Py_BEGIN_ALLOW_THREADS  // Free the Python interpreter for other threads
  try
  {
    // Generate the forecast
    static_cast<Forecast*>(fcst)->generateFutureValues
      (data, historycount, bucketdata, bucketcount, solver->getLogLevel()>0);
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


PyObject* ForecastBucket::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_startdate))
    return PythonObject(getDueRange().getStart());
  if (attr.isA(Tags::tag_enddate))
    return PythonObject(getDueRange().getEnd());
  if (attr.isA(Forecast::tag_total))
    return PythonObject(getTotal());
  if (attr.isA(Forecast::tag_consumed))
    return PythonObject(getConsumed());
  if (attr.isA(Tags::tag_weight))
    return PythonObject(getWeight());
  return Demand::getattro(attr);
}


int ForecastBucket::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Forecast::tag_total))
    setTotal(field.getDouble());
  else if (attr.isA(Forecast::tag_consumed))
    setConsumed(field.getDouble());
  else if (attr.isA(Tags::tag_weight))
    setWeight(field.getDouble());
  else
    return Demand::setattro(attr, field);
  return 0;  // OK
}


} // end namespace
