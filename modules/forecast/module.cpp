/***************************************************************************
 *                                                                         *
 * Copyright (C) 2012-2013 by Johan De Taeye, frePPLe bvba                 *
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
bool Forecast::Customer_Then_Item_Hierarchy = true;
bool Forecast::Match_Using_Delivery_Operation = true;
TimePeriod Forecast::Net_Late(0L);
TimePeriod Forecast::Net_Early(0L);
unsigned long Forecast::Forecast_Iterations(15L);
double Forecast::Forecast_SmapeAlfa(0.95);
unsigned long Forecast::Forecast_Skip(5);

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
const Keyword ForecastSolver::tag_Croston_initialAlfa("Croston_initialAlfa");
const Keyword ForecastSolver::tag_Croston_minAlfa("Croston_minAlfa");
const Keyword ForecastSolver::tag_Croston_maxAlfa("Croston_maxAlfa");
const Keyword ForecastSolver::tag_Croston_minIntermittence("Croston_minIntermittence");
const Keyword ForecastSolver::tag_Outlier_maxDeviation("Outlier_maxDeviation");


MODULE_EXPORT const char* initialize(const Environment::ParameterList& z)
{
  // Initialize only once
  static bool init = false;
  static const char* name = "forecast";
  if (init)
  {
    logger << "Warning: Initializing module forecast more than once." << endl;
    return name;
  }
  init = true;

  // Verify you have an enterprise license.
  // This specific value of the flag is when the customer name is "Community Edition users".
  if (flags == 836125) return "";

  // Register the Python extensions
  PyGILState_STATE state = PyGILState_Ensure();
  try
  {
    // Register new Python data types
    if (Forecast::initialize())
      throw RuntimeException("Error registering forecast");
    if (ForecastBucket::initialize())
      throw RuntimeException("Error registering forecastbucket");
    if (ForecastSolver::initialize())
      throw RuntimeException("Error registering forecastsolver");
    PyGILState_Release(state);
  }
  catch (const exception &e)
  {
    PyGILState_Release(state);
    logger << "Error: " << e.what() << endl;
  }
  catch (...)
  {
    PyGILState_Release(state);
    logger << "Error: unknown exception" << endl;
  }

  // Return the name of the module
  return name;
}


int ForecastSolver::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(tag_DueAtEndOfBucket))
    ForecastBucket::setDueAtEndOfBucket(field.getBool());
  // Netting
  else if (attr.isA(tag_Net_CustomerThenItemHierarchy))
    Forecast::setCustomerThenItemHierarchy(field.getBool());
  else if (attr.isA(tag_Net_MatchUsingDeliveryOperation))
    Forecast::setMatchUsingDeliveryOperation(field.getBool());
  else if (attr.isA(tag_Net_NetEarly))
    Forecast::setNetEarly(field.getTimeperiod());
  else if (attr.isA(tag_Net_NetLate))
    Forecast::setNetLate(field.getTimeperiod());
  // Forecasting
  else if (attr.isA(tag_Iterations))
    Forecast::setForecastIterations(field.getUnsignedLong());
  else if (attr.isA(tag_SmapeAlfa))
    Forecast::setForecastSmapeAlfa(field.getDouble());
  else if (attr.isA(tag_Skip))
    Forecast::setForecastSkip(field.getUnsignedLong());
  else if (attr.isA(tag_Outlier_maxDeviation))
    Forecast::setForecastMaxDeviation(field.getDouble());
  // Moving average forecast method
  else if (attr.isA(tag_MovingAverage_order))
    Forecast::MovingAverage::setDefaultOrder(field.getUnsignedLong());
  // Single exponential forecast method
  else if (attr.isA(tag_SingleExponential_initialAlfa))
    Forecast::SingleExponential::setInitialAlfa(field.getDouble());
  else if (attr.isA(tag_SingleExponential_minAlfa))
    Forecast::SingleExponential::setMinAlfa(field.getDouble());
  else if (attr.isA(tag_SingleExponential_maxAlfa))
    Forecast::SingleExponential::setMaxAlfa(field.getDouble());
  // Double exponential forecast method
  else if (attr.isA(tag_DoubleExponential_initialAlfa))
    Forecast::DoubleExponential::setInitialAlfa(field.getDouble());
  else if (attr.isA(tag_DoubleExponential_minAlfa))
    Forecast::DoubleExponential::setMinAlfa(field.getDouble());
  else if (attr.isA(tag_DoubleExponential_maxAlfa))
    Forecast::DoubleExponential::setMaxAlfa(field.getDouble());
  else if (attr.isA(tag_DoubleExponential_initialGamma))
    Forecast::DoubleExponential::setInitialGamma(field.getDouble());
  else if (attr.isA(tag_DoubleExponential_minGamma))
    Forecast::DoubleExponential::setMinGamma(field.getDouble());
  else if (attr.isA(tag_DoubleExponential_maxGamma))
    Forecast::DoubleExponential::setMaxGamma(field.getDouble());
  else if (attr.isA(tag_DoubleExponential_dampenTrend))
    Forecast::DoubleExponential::setDampenTrend(field.getDouble());
  // Seasonal forecast method
  else if (attr.isA(tag_Seasonal_initialAlfa))
    Forecast::Seasonal::setInitialAlfa(field.getDouble());
  else if (attr.isA(tag_Seasonal_minAlfa))
    Forecast::Seasonal::setMinAlfa(field.getDouble());
  else if (attr.isA(tag_Seasonal_maxAlfa))
    Forecast::Seasonal::setMaxAlfa(field.getDouble());
  else if (attr.isA(tag_Seasonal_initialBeta))
    Forecast::Seasonal::setInitialBeta(field.getDouble());
  else if (attr.isA(tag_Seasonal_minBeta))
    Forecast::Seasonal::setMinBeta(field.getDouble());
  else if (attr.isA(tag_Seasonal_maxBeta))
    Forecast::Seasonal::setMaxBeta(field.getDouble());
  else if (attr.isA(tag_Seasonal_gamma))
    Forecast::Seasonal::setGamma(field.getDouble());
  else if (attr.isA(tag_Seasonal_dampenTrend))
    Forecast::Seasonal::setDampenTrend(field.getDouble());
  else if (attr.isA(tag_Seasonal_minPeriod))
    Forecast::Seasonal::setMinPeriod(field.getInt());
  else if (attr.isA(tag_Seasonal_maxPeriod))
    Forecast::Seasonal::setMaxPeriod(field.getInt());
  // Croston forecast method
  else if (attr.isA(tag_Croston_initialAlfa))
    Forecast::Croston::setInitialAlfa(field.getDouble());
  else if (attr.isA(tag_Croston_minAlfa))
    Forecast::Croston::setMinAlfa(field.getDouble());
  else if (attr.isA(tag_Croston_maxAlfa))
    Forecast::Croston::setMaxAlfa(field.getDouble());
  else if (attr.isA(tag_Croston_minIntermittence))
    Forecast::Croston::setMinIntermittence(field.getDouble());
  // Default fields
  else
    return Solver::setattro(attr, field);
  return 0;  // OK
}

}       // end namespace
