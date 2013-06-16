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
  if (!strcmp(attr.getName(), "DueAtEndOfBucket"))
    ForecastBucket::setDueAtEndOfBucket(field.getBool());
  // Netting
  else if (!strcmp(attr.getName(),"Net_CustomerThenItemHierarchy"))
    Forecast::setCustomerThenItemHierarchy(field.getBool());
  else if (!strcmp(attr.getName(), "Net_MatchUsingDeliveryOperation"))
    Forecast::setMatchUsingDeliveryOperation(field.getBool());
  else if (!strcmp(attr.getName(), "Net_NetEarly"))
    Forecast::setNetEarly(field.getTimeperiod());
  else if (!strcmp(attr.getName(), "Net_NetLate"))
    Forecast::setNetLate(field.getTimeperiod());
  // Forecasting
  else if (!strcmp(attr.getName(), "Iterations"))
    Forecast::setForecastIterations(field.getUnsignedLong());
  else if (!strcmp(attr.getName(), "SmapeAlfa"))
    Forecast::setForecastSmapeAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Skip"))
    Forecast::setForecastSkip(field.getUnsignedLong());
  // Moving average forecast method
  else if (!strcmp(attr.getName(), "MovingAverage_order"))
    Forecast::MovingAverage::setDefaultOrder(field.getUnsignedLong());
  // Single exponential forecast method
  else if (!strcmp(attr.getName(), "SingleExponential_initialAlfa"))
    Forecast::SingleExponential::setInitialAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "SingleExponential_minAlfa"))
    Forecast::SingleExponential::setMinAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "SingleExponential_maxAlfa"))
    Forecast::SingleExponential::setMaxAlfa(field.getDouble());
  // Double exponential forecast method
  else if (!strcmp(attr.getName(), "DoubleExponential_initialAlfa"))
    Forecast::DoubleExponential::setInitialAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "DoubleExponential_minAlfa"))
    Forecast::DoubleExponential::setMinAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "DoubleExponential_maxAlfa"))
    Forecast::DoubleExponential::setMaxAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "DoubleExponential_initialGamma"))
    Forecast::DoubleExponential::setInitialGamma(field.getDouble());
  else if (!strcmp(attr.getName(), "DoubleExponential_minGamma"))
    Forecast::DoubleExponential::setMinGamma(field.getDouble());
  else if (!strcmp(attr.getName(), "DoubleExponential_maxGamma"))
    Forecast::DoubleExponential::setMaxGamma(field.getDouble());
  else if (!strcmp(attr.getName(), "DoubleExponential_dampenTrend"))
    Forecast::DoubleExponential::setDampenTrend(field.getDouble());
  // Seasonal forecast method
  else if (!strcmp(attr.getName(), "Seasonal_initialAlfa"))
    Forecast::Seasonal::setInitialAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_minAlfa"))
    Forecast::Seasonal::setMinAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_maxAlfa"))
    Forecast::Seasonal::setMaxAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_initialBeta"))
    Forecast::Seasonal::setInitialGamma(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_minBeta"))
    Forecast::Seasonal::setMinBeta(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_maxBeta"))
    Forecast::Seasonal::setMaxBeta(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_initialGamma"))
    Forecast::Seasonal::setInitialBeta(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_minGamma"))
    Forecast::Seasonal::setMinGamma(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_maxGamma"))
    Forecast::Seasonal::setMaxGamma(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_dampenTrend"))
    Forecast::Seasonal::setDampenTrend(field.getDouble());
  else if (!strcmp(attr.getName(), "Seasonal_minPeriod"))
    Forecast::Seasonal::setMinPeriod(field.getInt());
  else if (!strcmp(attr.getName(), "Seasonal_maxPeriod"))
    Forecast::Seasonal::setMaxPeriod(field.getInt());
  // Croston forecast method
  else if (!strcmp(attr.getName(), "Croston_initialAlfa"))
    Forecast::Croston::setInitialAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Croston_minAlfa"))
    Forecast::Croston::setMinAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Croston_maxAlfa"))
    Forecast::Croston::setMaxAlfa(field.getDouble());
  else if (!strcmp(attr.getName(), "Croston_minIntermittence"))
    Forecast::Croston::setMinIntermittence(field.getDouble());
  else
    return Solver::setattro(attr, field);
  return 0;  // OK
}

}       // end namespace
