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
bool Forecast::Customer_Then_Item_Hierarchy = true;
bool Forecast::Match_Using_Delivery_Operation = true;
Duration Forecast::Net_Late(0L);
Duration Forecast::Net_Early(0L);
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
const Keyword ForecastSolver::tag_Seasonal_minAutocorrelation("Seasonal_minAutocorrelation");
const Keyword ForecastSolver::tag_Seasonal_maxAutocorrelation("Seasonal_maxAutocorrelation");
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
  if (flags == 269264) return "";

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

}       // end namespace
