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

#define ACCURACY 0.01


double Forecast::Forecast_maxDeviation = 4.0;


void Forecast::generateFutureValues(
  const double history[], unsigned int historycount,
  const Date buckets[], unsigned int bucketcount,
  ForecastSolver* solver)
{
  if (!methods)
    // No computation here
    return;

  // Validate the input
  if (!history || !buckets)
    throw RuntimeException("Null argument to forecast function");
  if (bucketcount < 2)
    throw DataException("Need at least 2 forecast dates");

  // Strip zero demand buckets at the start.
  // Eg when demand only starts in the middle of horizon, we only want to
  // use the second part of the horizon for forecasting. The zeros before the
  // demand start would distort the results.
  while (historycount >= 1 && *history == 0.0)
  {
    ++history;
    --historycount;
  }

  // We create the forecasting objects in stack memory for best performance.
  MovingAverage moving_avg;
  Croston croston;
  SingleExponential single_exp;
  DoubleExponential double_exp;
  Seasonal seasonal;
  int numberOfMethods = 0;
  ForecastMethod* qualifiedmethods[5];

  // Rules to determine which forecast methods can be applied
  if (historycount <= getForecastSkip() + 5)
  {
    // If there is too little history, only use moving average or the forced methods
    if (methods & METHOD_MOVINGAVERAGE)
      qualifiedmethods[numberOfMethods++] = &moving_avg;
  }
  else
  {
    unsigned int zero = 0;
    for (unsigned long i = 0; i < historycount; ++i)
      if (!history[i]) ++zero;
    if (zero > Croston::getMinIntermittence() * historycount)
    {
      // If there are too many zeros: use croston or moving average.
      if (methods & METHOD_CROSTON)
        qualifiedmethods[numberOfMethods++] = &croston;
    }
    else
    {
      // The most common case: enough values and not intermittent
      if (methods & METHOD_MOVINGAVERAGE)
        qualifiedmethods[numberOfMethods++] = &moving_avg;
      if (methods & METHOD_CONSTANT)
        qualifiedmethods[numberOfMethods++] = &single_exp;
      if (methods & METHOD_TREND)
        qualifiedmethods[numberOfMethods++] = &double_exp;
      if (methods & METHOD_SEASONAL)
        qualifiedmethods[numberOfMethods++] = &seasonal;
    }
  }

  // Special case: no method qualifies at all based on our criteria
  // We will take only the enforced methods.
  if (numberOfMethods == 0)
  {
    if (solver->getLogLevel()>0)
      logger << getName() << ": Warning: The specified forecast methods are potentially not suitable!" << endl;
    if (methods & METHOD_MOVINGAVERAGE)
      qualifiedmethods[numberOfMethods++] = &moving_avg;
    if (methods & METHOD_CROSTON)
      qualifiedmethods[numberOfMethods++] = &croston;
    if (methods & METHOD_CONSTANT)
      qualifiedmethods[numberOfMethods++] = &single_exp;
    if (methods & METHOD_TREND)
      qualifiedmethods[numberOfMethods++] = &double_exp;
    if (methods & METHOD_SEASONAL)
      qualifiedmethods[numberOfMethods++] = &seasonal;
  }

  // Initialize a vector with the smape weights
  double *weight = new double[historycount+1];
  weight[historycount] = 1.0;
  for (int i=historycount-1; i>=0; --i)
    weight[i] = weight[i+1] * Forecast::getForecastSmapeAlfa();

  // Evaluate each forecast method
  double best_error = DBL_MAX;
  int best_method = -1;
  bool forced_method = false;
  try
  {
    for (int i=0; i<numberOfMethods; ++i)
    {
      Metrics res = qualifiedmethods[i]->generateForecast(
        this, history, historycount, weight, solver
        );
      if (res.smape < best_error || res.force)
      {
        best_error = res.smape;
        best_method = i;
        deviation = res.standarddeviation;
        if (res.force)
        {
          forced_method = true;
          deviation = res.standarddeviation;
          break;
        }
      }
    }
    if (methods==METHOD_SEASONAL && best_error == DBL_MAX)
    {
      // Special case: the only allowed forecast method is seasonal and we
      // couldn't detect any cycles. We fall back to the trend method.
      qualifiedmethods[0] = &double_exp;
      best_method = 0;
      Metrics res = double_exp.generateForecast(this, history, historycount, weight, solver);
      best_error = res.smape;
      deviation = res.standarddeviation;
    }
  }
  catch (...)
  {
    delete[] weight;
    throw;
  }
  delete[] weight;

  // Apply the most appropriate forecasting method
  if (best_method >= 0)
  {
    if (solver->getLogLevel()>0)
      logger << getName()
        << ": chosen method: " << qualifiedmethods[best_method]->getName()
        << ", standard deviation: " << deviation
        << endl;
    qualifiedmethods[best_method]->applyForecast(this, buckets, bucketcount);
    method = qualifiedmethods[best_method]->getName();
    smape_error = best_error;
  }
  else
  {
    method = "None";
    smape_error = 0.0;
  }
}


//
// MOVING AVERAGE FORECAST
//


unsigned int Forecast::MovingAverage::defaultorder = 5;


Forecast::Metrics Forecast::MovingAverage::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  double error_smape, error_smape_weights;
  double clean_history[300];

  // Loop over the outliers 'scan'/0 and 'filter'/1 modes
  double standarddeviation = 0.0;
  double maxdeviation = 0.0;
  for (short outliers = 0; outliers<=1; outliers++)
  {
    if (outliers)
      clean_history[0] = history[0];
    error_smape = 0.0;
    error_smape_weights = 0.0;

    // Calculate the forecast and forecast error.
    for (unsigned int i = 1; i <= count; ++i)
    {
      if (outliers == 0)
      {
        double sum = 0.0;
        if (i > order)
        {
          for (unsigned int j = 0; j < order; ++j)
            sum += history[i-j-1];
          avg = sum / order;
        }
        else
        {
          // For the first few values
          for (unsigned int j = 0; j < i; ++j)
            sum += history[i-j-1];
          avg = sum / i;
        }
        if (i == count) break;

        // Scan outliers by computing the standard deviation
        // and keeping track of the difference between actuals and forecast
        standarddeviation += (avg - history[i]) * (avg - history[i]);
        if (fabs(avg - history[i]) > maxdeviation)
          maxdeviation = fabs(avg - history[i]);
      }
      else
      {
        double sum = 0.0;
        if (i > order)
        {
          for (unsigned int j = 0; j < order; ++j)
            sum += clean_history[i-j-1];
          avg = sum / order;
        }
        else
        {
          // For the first few values
          for (unsigned int j = 0; j < i; ++j)
            sum += clean_history[i-j-1];
          avg = sum / i;
        }
        if (i == count) break;

        // Clean outliers from history.
        // We copy the cleaned history data in a new array.
        if (history[i] > avg + Forecast::Forecast_maxDeviation * standarddeviation)
          clean_history[i] = avg + Forecast::Forecast_maxDeviation * standarddeviation;
        else if (history[i] < avg - Forecast::Forecast_maxDeviation * standarddeviation)
          clean_history[i] = avg - Forecast::Forecast_maxDeviation * standarddeviation;
        else
          clean_history[i] = history[i];
      }

      if (i >= fcst->getForecastSkip() && i < count && fabs(avg + history[i]) > ROUNDING_ERROR)
      {
        error_smape += fabs(avg - history[i]) / fabs(avg + history[i]) * weight[i] * 2;
        error_smape_weights += weight[i];
      }
    }

    // Check outliers
    if (outliers == 0)
    {
      standarddeviation = sqrt(standarddeviation / (count-1));
      maxdeviation /= standarddeviation;
      // Don't repeat if there are no outliers
      if (maxdeviation < Forecast::Forecast_maxDeviation) break;
    }
  } // End loop: 'scan' or 'filter' mode for outliers

  // Echo the result
  if (error_smape_weights)
    error_smape /= error_smape_weights;
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": moving average : "
        << "smape " << error_smape
        << ", forecast " << avg
        << ", standard deviation " << standarddeviation
        << endl;
  return Forecast::Metrics(error_smape, standarddeviation, false);
}


void Forecast::MovingAverage::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a constant value
  if (forecast->discrete)
  {
    double carryover = 0.0;
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      carryover += avg;
      double val = ceil(carryover - 0.5);
      carryover -= val;
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        val > 0.0 ? val : 0.0
      );
    }
  }
  else
    for (unsigned int i = 1; i < bucketcount; ++i)
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        avg > 0.0 ? avg : 0.0
      );
}


//
// SINGLE EXPONENTIAL FORECAST
//


double Forecast::SingleExponential::initial_alfa = 0.2;
double Forecast::SingleExponential::min_alfa = 0.03;
double Forecast::SingleExponential::max_alfa = 1.0;


Forecast::Metrics Forecast::SingleExponential::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Verify whether this is a valid forecast method.
  //   - We need at least 5 buckets after the warmup period.
  if (count < fcst->getForecastSkip() + 5)
    return Forecast::Metrics(DBL_MAX, DBL_MAX, false);

  unsigned int iteration = 1;
  bool upperboundarytested = false;
  bool lowerboundarytested = false;
  double error = 0.0, error_smape = 0.0, error_smape_weights = 0.0, best_smape = 0.0, delta, df_dalfa_i, sum_11, sum_12;
  double best_error = DBL_MAX, best_alfa = initial_alfa, best_f_i = 0.0;
  double best_standarddeviation = 0.0;
  for (; iteration <= Forecast::getForecastIterations(); ++iteration)
  {

    // Loop over the outliers 'scan'/0 and 'filter'/1 modes
    double standarddeviation = 0.0;
    double maxdeviation = 0.0;
    for (short outliers = 0; outliers<=1; outliers++)
    {
      // Initialize variables
      df_dalfa_i = sum_11 = sum_12 = error_smape = error_smape_weights = error = 0.0;

      // Initialize the iteration with the average of the first 3 values.
      f_i = (history[0] + history[1] + history[2]) / 3;
      if (outliers == 1)
      {
        // TODO this logic isn't the right concept?
        double t = 0.0;
        if (history[0] > f_i + Forecast::Forecast_maxDeviation * standarddeviation)
          t += f_i + Forecast::Forecast_maxDeviation * standarddeviation;
        else if (history[0] < f_i - Forecast::Forecast_maxDeviation * standarddeviation)
          t += f_i - Forecast::Forecast_maxDeviation * standarddeviation;
        else
          t += history[0];
        if (history[1] > f_i + Forecast::Forecast_maxDeviation * standarddeviation)
          t += f_i + Forecast::Forecast_maxDeviation * standarddeviation;
        else if (history[1] < f_i - Forecast::Forecast_maxDeviation * standarddeviation)
          t += f_i - Forecast::Forecast_maxDeviation * standarddeviation;
        else
          t += history[1];
        if (history[2] > f_i + Forecast::Forecast_maxDeviation * standarddeviation)
          t += f_i + Forecast::Forecast_maxDeviation * standarddeviation;
        else if (history[2] < f_i - Forecast::Forecast_maxDeviation * standarddeviation)
          t += f_i - Forecast::Forecast_maxDeviation * standarddeviation;
        else
          t += history[2];
        f_i = t / 3;
      }

      // Calculate the forecast and forecast error.
      // We also compute the sums required for the Marquardt optimization.
      double history_i = history[0];
      double history_i_min_1 = history[0];
      for (unsigned long i = 1; i <= count; ++i)
      {
        history_i_min_1 = history_i;
        history_i = history[i];
        df_dalfa_i = history_i_min_1 - f_i + (1 - alfa) * df_dalfa_i;
        f_i = history_i_min_1 * alfa + (1 - alfa) * f_i;
        if (i == count) break;
        if (outliers == 0)
        {
          // Scan outliers by computing the standard deviation
          // and keeping track of the difference between actuals and forecast
          standarddeviation += (f_i - history[i]) * (f_i - history[i]);
          if (fabs(f_i - history[i]) > maxdeviation)
            maxdeviation = fabs(f_i - history[i]);
        }
        else
        {
          // Clean outliers from history
          if (history_i > f_i + Forecast::Forecast_maxDeviation * standarddeviation)
            history_i = f_i + Forecast::Forecast_maxDeviation * standarddeviation;
          else if (history_i < f_i - Forecast::Forecast_maxDeviation * standarddeviation)
            history_i = f_i - Forecast::Forecast_maxDeviation * standarddeviation;
        }
        sum_12 += df_dalfa_i * (history_i - f_i) * weight[i];
        sum_11 += df_dalfa_i * df_dalfa_i * weight[i];
        if (i >= fcst->getForecastSkip())
        {
          error += (f_i - history_i) * (f_i - history_i) * weight[i];
          if (fabs(f_i + history[i]) > ROUNDING_ERROR)
          {
            error_smape += fabs(f_i - history_i) / (f_i + history_i) * weight[i] * 2;
            error_smape_weights += weight[i];
          }
        }
      }

      // Check outliers
      if (outliers == 0)
      {
        standarddeviation = sqrt(standarddeviation / (count-1));
        maxdeviation /= standarddeviation;
        // Don't repeat if there are no outliers
        if (maxdeviation < Forecast::Forecast_maxDeviation) break;
      }
    } // End loop: 'scan' or 'filter' mode for outliers

    // Better than earlier iterations?
    if (error < best_error)
    {
      best_error = error;
      best_smape = error_smape_weights ? error_smape / error_smape_weights : 0.0;
      best_alfa = alfa;
      best_f_i = f_i;
      best_standarddeviation = standarddeviation;
    }

    // Add Levenberg - Marquardt damping factor
    if (fabs(sum_11 + error / iteration) > ROUNDING_ERROR)
      sum_11 += error / iteration;

    // Calculate a delta for the alfa parameter
    if (fabs(sum_11) < ROUNDING_ERROR) break;
    delta = sum_12 / sum_11;

    // Stop when we are close enough and have tried hard enough
    if (fabs(delta) < ACCURACY && iteration > 3) break;

    // Debugging info on the iteration
    if (solver->getLogLevel()>5)
      logger << (fcst ? fcst->getName() : "")
        << ": single exponential : iteration " << iteration
        << ": alfa " << alfa
        << ", smape " << (error_smape_weights ? error_smape / error_smape_weights : 0.0)
        << endl;

    // New alfa
    alfa += delta;

    // Stop when we repeatedly bounce against the limits.
    // Testing a limits once is allowed.
    if (alfa > max_alfa)
    {
      alfa = max_alfa;
      if (upperboundarytested) break;
      upperboundarytested = true;
    }
    else if (alfa < min_alfa)
    {
      alfa = min_alfa;
      if (lowerboundarytested) break;
      lowerboundarytested = true;
    }
  }

  // Keep the best result
  f_i = best_f_i;

  // Echo the result
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": single exponential : "
        << "alfa " << best_alfa
        << ", smape " << best_smape
        << ", " << iteration << " iterations"
        << ", forecast " << f_i
        << ", standard deviation " << best_standarddeviation
        << endl;
  return Forecast::Metrics(best_smape, best_standarddeviation, false);
}


void Forecast::SingleExponential::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a constant value
  if (forecast->discrete)
  {
    double carryover = 0.0;
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      carryover += f_i;
      double val = ceil(carryover - 0.5);
      carryover -= val;
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        val > 0.0 ? val : 0.0
      );
    }
  }
  else
    for (unsigned int i = 1; i < bucketcount; ++i)
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        f_i > 0.0 ? f_i : 0.0
      );
}


//
// DOUBLE EXPONENTIAL FORECAST
//

double Forecast::DoubleExponential::initial_alfa = 0.2;
double Forecast::DoubleExponential::min_alfa = 0.02;
double Forecast::DoubleExponential::max_alfa = 1.0;
double Forecast::DoubleExponential::initial_gamma = 0.2;
double Forecast::DoubleExponential::min_gamma = 0.05;
double Forecast::DoubleExponential::max_gamma = 1.0;
double Forecast::DoubleExponential::dampenTrend = 0.8;


Forecast::Metrics Forecast::DoubleExponential::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Verify whether this is a valid forecast method.
  //   - We need at least 5 buckets after the warmup period.
  if (count < fcst->getForecastSkip() + 5)
    return Forecast::Metrics(DBL_MAX, DBL_MAX, false);

  // Define variables
  double error=0.0, error_smape=0.0, error_smape_weights=0.0, delta_alfa, delta_gamma, determinant;
  double constant_i_prev, trend_i_prev, d_constant_d_gamma_prev,
         d_constant_d_alfa_prev, d_constant_d_alfa, d_constant_d_gamma,
         d_trend_d_alfa, d_trend_d_gamma, d_forecast_d_alfa, d_forecast_d_gamma,
         sum11, sum12, sum22, sum13, sum23;
  double best_error = DBL_MAX, best_smape = 0, best_alfa = initial_alfa,
         best_gamma = initial_gamma, best_constant_i = 0.0, best_trend_i = 0.0;
  double best_standarddeviation = 0.0;

  // Iterations
  unsigned int iteration = 1, boundarytested = 0;
  for (; iteration <= Forecast::getForecastIterations(); ++iteration)
  {
    // Loop over the outliers 'scan'/0 and 'filter'/1 modes
    double standarddeviation = 0.0;
    double maxdeviation = 0.0;
    for (short outliers = 0; outliers<=1; outliers++)
    {
      // Initialize variables
      error = error_smape = error_smape_weights = sum11 = sum12 = sum22 = sum13 = sum23 = 0.0;
      d_constant_d_alfa = d_constant_d_gamma = d_trend_d_alfa = d_trend_d_gamma = 0.0;
      d_forecast_d_alfa = d_forecast_d_gamma = 0.0;

      // Initialize the iteration
      constant_i = (history[0] + history[1] + history[2]) / 3;
      trend_i = (history[3] - history[0]) / 3;
      if (outliers == 1)
      {
        // TODO this logic isn't the right concept?
        double t1 = 0.0;
        if (history[0] > constant_i + Forecast::Forecast_maxDeviation * standarddeviation)
          t1 = constant_i + Forecast::Forecast_maxDeviation * standarddeviation;
        else if (history[0] < constant_i - Forecast::Forecast_maxDeviation * standarddeviation)
          t1 = constant_i - Forecast::Forecast_maxDeviation * standarddeviation;
        else
          t1 = history[0];
        double t2 = - t1;
        if (history[1] > constant_i + trend_i + Forecast::Forecast_maxDeviation * standarddeviation)
          t1 += constant_i + trend_i + Forecast::Forecast_maxDeviation * standarddeviation;
        else if (history[1] < constant_i + trend_i - Forecast::Forecast_maxDeviation * standarddeviation)
          t1 += constant_i + trend_i - Forecast::Forecast_maxDeviation * standarddeviation;
        else
          t1 += history[1];
        if (history[2] > constant_i + 2 * trend_i + Forecast::Forecast_maxDeviation * standarddeviation)
        {
          t1 += constant_i + 2 * trend_i + Forecast::Forecast_maxDeviation * standarddeviation;
          t2 += constant_i + 2 * trend_i + Forecast::Forecast_maxDeviation * standarddeviation;
        }
        else if (history[2] < constant_i + 2 * trend_i - Forecast::Forecast_maxDeviation * standarddeviation)
        {
          t1 += constant_i + 2 * trend_i - Forecast::Forecast_maxDeviation * standarddeviation;
          t2 += constant_i + 2 * trend_i - Forecast::Forecast_maxDeviation * standarddeviation;
        }
        else
        {
          t1 += history[2];
          t2 += history[2];
        }
        constant_i = t1 / 3;
        trend_i = t2 / 3;
      }

      // Calculate the forecast and forecast error.
      // We also compute the sums required for the Marquardt optimization.
      double history_i = history[0];
      double history_i_min_1 = history[0];
      for (unsigned long i = 1; i <= count; ++i)
      {
        history_i_min_1 = history_i;
        history_i = history[i];
        constant_i_prev = constant_i;
        trend_i_prev = trend_i;
        constant_i = history_i_min_1 * alfa + (1 - alfa) * (constant_i_prev + trend_i_prev);
        trend_i = gamma * (constant_i - constant_i_prev) + (1 - gamma) * trend_i_prev;
        if (i == count) break;
        if (outliers == 0)
        {
          // Scan outliers by computing the standard deviation
          // and keeping track of the difference between actuals and forecast
          standarddeviation += (constant_i + trend_i - history[i]) * (constant_i + trend_i - history[i]);
          if (fabs(constant_i + trend_i - history[i]) > maxdeviation)
            maxdeviation = fabs(constant_i + trend_i - history[i]);
        }
        else
        {
          // Clean outliers from history
          if (history_i > constant_i + trend_i + Forecast::Forecast_maxDeviation * standarddeviation)
            history_i = constant_i + trend_i + Forecast::Forecast_maxDeviation * standarddeviation;
          else if (history_i < constant_i + trend_i - Forecast::Forecast_maxDeviation * standarddeviation)
            history_i = constant_i + trend_i - Forecast::Forecast_maxDeviation * standarddeviation;
        }
        d_constant_d_gamma_prev = d_constant_d_gamma;
        d_constant_d_alfa_prev = d_constant_d_alfa;
        d_constant_d_alfa = history_i_min_1 - constant_i_prev - trend_i_prev
            + (1 - alfa) * d_forecast_d_alfa;
        d_constant_d_gamma = (1 - alfa) * d_forecast_d_gamma;
        d_trend_d_alfa = gamma * (d_constant_d_alfa - d_constant_d_alfa_prev)
            + (1 - gamma) * d_trend_d_alfa;
        d_trend_d_gamma = constant_i - constant_i_prev - trend_i_prev
            + gamma * (d_constant_d_gamma - d_constant_d_gamma_prev)
            + (1 - gamma) * d_trend_d_gamma;
        d_forecast_d_alfa = d_constant_d_alfa + d_trend_d_alfa;
        d_forecast_d_gamma = d_constant_d_gamma + d_trend_d_gamma;
        sum11 += weight[i] * d_forecast_d_alfa * d_forecast_d_alfa;
        sum12 += weight[i] * d_forecast_d_alfa * d_forecast_d_gamma;
        sum22 += weight[i] * d_forecast_d_gamma * d_forecast_d_gamma;
        sum13 += weight[i] * d_forecast_d_alfa * (history_i - constant_i - trend_i);
        sum23 += weight[i] * d_forecast_d_gamma * (history_i - constant_i - trend_i);
        if (i >= fcst->getForecastSkip()) // Don't measure during the warmup period
        {
          error += (constant_i + trend_i - history_i) * (constant_i + trend_i - history_i) * weight[i];
          if (fabs(constant_i + trend_i + history_i) > ROUNDING_ERROR)
          {
            error_smape += fabs(constant_i + trend_i - history_i) / fabs(constant_i + trend_i + history_i) * weight[i] * 2;
            error_smape_weights += weight[i];
          }
        }
      }

      // Check outliers
      if (outliers == 0)
      {
        standarddeviation = sqrt(standarddeviation / (count-1));
        maxdeviation /= standarddeviation;
        // Don't repeat if there are no outliers
        if (maxdeviation < Forecast::Forecast_maxDeviation) break;
      }
    } // End loop: 'scan' or 'filter' mode for outliers

    // Better than earlier iterations?
    if (error < best_error)
    {
      best_error = error;
      best_smape = error_smape_weights ? error_smape / error_smape_weights : 0.0;
      best_alfa = alfa;
      best_gamma = gamma;
      best_constant_i = constant_i;
      best_trend_i = trend_i;
      best_standarddeviation = standarddeviation;
    }

    // Add Levenberg - Marquardt damping factor
    //if (alfa < max_alfa && alfa > min_alfa)
    sum11 += error / iteration; // * d_forecast_d_alfa;
    //if (gamma < max_gamma && gamma > min_gamma)
    sum22 += error / iteration; // * d_forecast_d_gamma;

    // Calculate a delta for the alfa and gamma parameters
    determinant = sum11 * sum22 - sum12 * sum12;
    if (fabs(determinant) < ROUNDING_ERROR)
    {
      // Almost singular matrix. Try without the damping factor.
      //if (alfa < max_alfa && alfa > min_alfa)
      sum11 -= error / iteration;
      //if (gamma < max_gamma && gamma > min_gamma)
      sum22 -= error / iteration;
      determinant = sum11 * sum22 - sum12 * sum12;
      if (fabs(determinant) < ROUNDING_ERROR)
        // Still singular - stop iterations here
        break;
    }
    delta_alfa = (sum13 * sum22 - sum23 * sum12) / determinant;
    delta_gamma = (sum23 * sum11 - sum13 * sum12) / determinant;

    // Stop when we are close enough and have tried hard enough
    if (fabs(delta_alfa) + fabs(delta_gamma) < 2 * ACCURACY && iteration > 3)
      break;

    // Debugging info on the iteration
    if (solver->getLogLevel()>5)
      logger << (fcst ? fcst->getName() : "")
        << ": double exponential : iteration " << iteration
        << ": alfa " << alfa << ", gamma " << gamma
        << ", smape " << (error_smape_weights ? error_smape / error_smape_weights : 0)
        << endl;

    // New values for the next iteration
    alfa += delta_alfa;
    gamma += delta_gamma;

    // Limit the parameters in their allowed range.
    if (alfa > max_alfa)
      alfa = max_alfa;
    else if (alfa < min_alfa)
      alfa = min_alfa;
    if (gamma > max_gamma)
      gamma = max_gamma;
    else if (gamma < min_gamma)
      gamma = min_gamma;

    // Verify repeated running with both parameters at the boundary
    if ((gamma == min_gamma || gamma == max_gamma)
        && (alfa == min_alfa || alfa == max_alfa))
    {
      if (boundarytested++ > 5) break;
    }
  }

  // Keep the best result
  constant_i = best_constant_i;
  trend_i = best_trend_i;

  // Echo the result
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": double exponential : "
        << "alfa " << best_alfa
        << ", gamma " << best_gamma
        << ", smape " << best_smape
        << ", " << iteration << " iterations"
        << ", constant " << constant_i
        << ", trend " << trend_i
        << ", forecast " << (trend_i + constant_i)
        << ", standard deviation " << best_standarddeviation
        << endl;
  return Forecast::Metrics(best_smape, best_standarddeviation, false);
}


void Forecast::DoubleExponential::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a linearly changing value
  if (forecast->discrete)
  {
    double carryover = 0.0;
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      constant_i += trend_i;
      trend_i *= dampenTrend; // Reduce slope in the future
      carryover += constant_i;
      double val = ceil(carryover - 0.5);
      carryover -= val;
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        val > 0.0 ? val : 0.0
      );
    }
  }
  else
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      constant_i += trend_i;
      trend_i *= dampenTrend; // Reduce slope in the future
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        constant_i > 0.0 ? constant_i : 0.0
      );
    }
}


//
// SEASONAL FORECAST
//

unsigned int Forecast::Seasonal::min_period = 2;
unsigned int Forecast::Seasonal::max_period = 14;
double Forecast::Seasonal::dampenTrend = 0.8;
double Forecast::Seasonal::initial_alfa = 0.2;
double Forecast::Seasonal::min_alfa = 0.02;
double Forecast::Seasonal::max_alfa = 1.0;
double Forecast::Seasonal::initial_beta = 0.2;
double Forecast::Seasonal::min_beta = 0.2;
double Forecast::Seasonal::max_beta = 1.0;
double Forecast::Seasonal::gamma = 0.05;
double Forecast::Seasonal::min_autocorrelation = 0.5;
double Forecast::Seasonal::max_autocorrelation = 0.8;


void Forecast::Seasonal::detectCycle(const double history[], unsigned int count)
{
  // We need at least 2 cycles
  if (count < min_period*2) return;

  // Compute the average value
  double average = 0.0;
  for (unsigned int i = 0; i < count; ++i)
    average += history[i];
  average /= count;

  // Compute variance
  double variance = 0.0;
  for (unsigned int i = 0; i < count; ++i)
    variance += (history[i]-average)*(history[i]-average);
  variance /= count;

  // Compute autocorrelation for different periods
  unsigned short best_period = 0;
  double best_autocorrelation = min_autocorrelation; // Minimum required correlation!
  double prev = 10.0;
  double prevprev = 10.0;
  double prevprevprev = 10.0;
  for (unsigned short p = min_period; p <= max_period && p < count/2; ++p)
  {
    // Compute correlation
    double correlation = 0.0;
    for (unsigned int i = p; i < count; ++i)
      correlation += (history[i]-average)*(history[i-p]-average);
    correlation /= count - p;
    correlation /= variance;
    // Detect cycles if we find a period with a big autocorrelation which
    // is significantly larger than the adjacent periods.
    if (p > min_period + 1
      && prev > prevprev * 1.1
      && prev > correlation * 1.1
      && prev > best_autocorrelation
      )
    {
      // Autocorrelation peak at a single period
      best_autocorrelation = prev;
      best_period = p - 1;
    }
    if (p > min_period + 2
      && prevprev > prevprevprev * 1.1
      && fabs(prevprev - prev) < 0.05
      && prev > correlation * 1.1
      )
    {
      // Autocorrelation peak across 2 periods
      if (prev > best_autocorrelation)
      {
        best_autocorrelation = prev;
        best_period = p - 1;
      }
      if (prevprev > best_autocorrelation)
      {
        best_autocorrelation = prevprev;
        best_period = p - 2;
      }
    }
    prevprevprev = prevprev;
    prevprev = prev;
    prev = correlation;
  }
  autocorrelation = best_autocorrelation;
  period = best_period;
}


Forecast::Metrics Forecast::Seasonal::generateForecast  // TODO No outlier detection in this method
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Check for seasonal cycles
  detectCycle(history, count);

  // Return if no seasonality is found
  if (!period)
    return Forecast::Metrics(DBL_MAX, DBL_MAX, false);

  // Define variables
  double error=0.0, error_smape=0.0, error_smape_weights=0.0, determinant, delta_alfa, delta_beta;
  double forecast_i, d_forecast_d_alfa, d_forecast_d_beta;
  double d_L_d_alfa, d_L_d_beta;
  double d_T_d_alfa, d_T_d_beta;
  double d_S_d_alfa[24], d_S_d_beta[24];
  double d_L_d_alfa_prev, d_L_d_beta_prev;
  double d_T_d_alfa_prev, d_T_d_beta_prev;
  double d_S_d_alfa_prev, d_S_d_beta_prev;
  double sum11, sum12, sum13, sum22, sum23;
  double best_error = DBL_MAX, best_smape = 0, best_alfa = initial_alfa,
         best_beta = initial_beta, best_standarddeviation = 0.0;
  double initial_S_i[24];
  double best_S_i[24], best_L_i, best_T_i;

  // Compute initialization values for the timeseries and seasonal index.
  // L_i = average over first cycle
  // T_i = average delta measured in second cycle
  // S_i[index] = seasonality index, measured over all complete cycles
  double L_i_initial = 0.0;
  double T_i_initial = 0.0;
  for (unsigned short i = 0; i < period; ++i)
  {
    L_i_initial += history[i];
    T_i_initial += history[i+period] - history[i];
    initial_S_i[i] = 0.0;
  }
  T_i_initial /= period;
  L_i_initial = L_i_initial / period;
  unsigned short cyclecount = 0;
  for (unsigned int i = 0; i + period <= count; i += period)
  {
    ++cyclecount;
    double cyclesum = 0.0;
    for (short j = 0; j < period; ++j)
      cyclesum += history[i+j];
    if (cyclesum)
      for (short j = 0; j < period; ++j)
        initial_S_i[j] += history[i+j] / cyclesum * period;
  }
  for (unsigned long i = 0; i < period; ++i)
    initial_S_i[i] /= cyclecount;

  // Iterations
  double L_i_prev;
  unsigned int iteration = 1, boundarytested = 0;
  double cyclesum;
  for (; iteration <= Forecast::getForecastIterations(); ++iteration)
  {
    // Initialize variables
    error = error_smape = error_smape_weights = sum11 = sum12 = sum13 = sum22 = sum23 = 0.0;
    d_L_d_alfa = d_L_d_beta = 0.0;
    d_T_d_alfa = d_T_d_beta = 0.0;
    L_i = L_i_initial;
    T_i = T_i_initial;
    cyclesum = 0.0;
    for (unsigned short i = 0; i < period; ++i)
    {
      S_i[i] = initial_S_i[i];
      d_S_d_alfa[i] = 0.0;
      d_S_d_beta[i] = 0.0;
      if (i) cyclesum += history[i-1];
    }

    // Calculate the forecast and forecast error.
    // We also compute the sums required for the Marquardt optimization.
    unsigned int prevcycleindex = period - 1;
    cycleindex = 0;
    for (unsigned int i = period; i <= count; ++i)
    {
      // Base calculations
      L_i_prev = L_i;
      cyclesum += history[i-1];
      if (i > period) cyclesum -= history[i-1-period];
      // Textbook approach for Holt-Winters multiplicative method:
      // L_i = alfa * history[i-1] / S_i[prevcycleindex] + (1 - alfa) * (L_i + T_i);
      // FrePPLe uses a variation to compute the constant component.
      // The alternative gives more stable and intuitive results for data that show variability.
      L_i = alfa * cyclesum / period + (1 - alfa) * (L_i + T_i);
      if (L_i < 0) L_i = 0.0;
      T_i = beta * (L_i - L_i_prev) + (1 - beta) * T_i;
      double factor = - S_i[prevcycleindex];
      if (L_i)
        S_i[prevcycleindex] = gamma * history[i-1] / L_i + (1 - gamma) * S_i[prevcycleindex];
      if (S_i[prevcycleindex] < 0.0)
        S_i[prevcycleindex] = 0.0;

      // Rescale the seasonal indexes to add up to "period"
      factor = period / (period + factor + S_i[prevcycleindex]);
      for (unsigned short i2 = 0; i2 < period; ++i2)
        S_i[i2] *= factor;

      if (i == count) break;
      // Calculations for the delta of the parameters
      d_L_d_alfa_prev = d_L_d_alfa;
      d_L_d_beta_prev = d_L_d_beta;
      d_T_d_alfa_prev = d_T_d_alfa;
      d_T_d_beta_prev = d_T_d_beta;
      d_S_d_alfa_prev = d_S_d_alfa[prevcycleindex];
      d_S_d_beta_prev = d_S_d_beta[prevcycleindex];
      d_L_d_alfa = cyclesum / period
        - (L_i + T_i)
        + (1 - alfa) * (d_L_d_alfa_prev + d_T_d_alfa_prev);
      d_L_d_beta = (1 - alfa) * (d_L_d_beta_prev + d_T_d_beta_prev);

      if (L_i > ROUNDING_ERROR)
      {
        d_S_d_alfa[prevcycleindex] = - gamma * history[i-1] / L_i / L_i * d_L_d_alfa_prev
          + (1 - gamma) * d_S_d_alfa_prev;
        d_S_d_beta[prevcycleindex] = - gamma * history[i-1] / L_i / L_i * d_L_d_beta_prev
          + (1 - gamma) * d_S_d_beta_prev;
      }
      else
      {
        d_S_d_alfa[prevcycleindex] = (1 - gamma) * d_S_d_alfa_prev;
        d_S_d_beta[prevcycleindex] = (1 - gamma) * d_S_d_beta_prev;
      }
      d_T_d_alfa = beta * (d_L_d_alfa - d_L_d_alfa_prev)
        + (1 - beta) * d_T_d_alfa_prev;
      d_T_d_beta = (L_i - L_i_prev)
        + beta * (d_L_d_beta - d_L_d_beta_prev)
        - T_i
        + (1 - beta) * d_T_d_beta_prev;
      d_forecast_d_alfa = (d_L_d_alfa + d_T_d_alfa) * S_i[cycleindex] + (L_i + T_i) * d_S_d_alfa[cycleindex];
      d_forecast_d_beta = (d_L_d_beta + d_T_d_beta) * S_i[cycleindex] + (L_i + T_i) * d_S_d_beta[cycleindex];
      forecast_i = (L_i + T_i) * S_i[cycleindex];
      sum11 += weight[i] * d_forecast_d_alfa * d_forecast_d_alfa;
      sum12 += weight[i] * d_forecast_d_alfa * d_forecast_d_beta;
      sum22 += weight[i] * d_forecast_d_beta * d_forecast_d_beta;
      sum13 += weight[i] * d_forecast_d_alfa * (history[i] - forecast_i);
      sum23 += weight[i] * d_forecast_d_beta * (history[i] - forecast_i);
      if (i >= fcst->getForecastSkip()) // Don't measure during the warmup period
      {
        double fcst = (L_i + T_i) * S_i[cycleindex];
        error += (fcst - history[i]) * (fcst - history[i]) * weight[i];
        if (fabs(fcst + history[i]) > ROUNDING_ERROR)
        {
          error_smape += fabs(fcst - history[i]) / fabs(fcst + history[i]) * weight[i] * 2;
          error_smape_weights += weight[i];
        }
      }
      if (++cycleindex >= period) cycleindex = 0;
      if (++prevcycleindex >= period) prevcycleindex = 0;
    }

    // Better than earlier iterations?
    if (error < best_error)
    {
      best_error = error;
      best_smape = error_smape_weights ? error_smape / error_smape_weights : 0.0;
      best_alfa = alfa;
      best_beta = beta;
      best_L_i = L_i;
      best_T_i = T_i;
      for (unsigned short i = 0; i < period; ++i)
        best_S_i[i] = S_i[i];
    }

    // Add Levenberg - Marquardt damping factor
    //if (alfa < max_alfa && alfa > min_alfa)
    sum11 += error / iteration; // * d_forecast_d_alfa;
    //if (beta < max_beta && beta > min_beta)
    sum22 += error / iteration; // * d_forecast_d_beta;

    // Calculate a delta for the alfa and gamma parameters
    determinant = sum11 * sum22 - sum12 * sum12;
    if (fabs(determinant) < ROUNDING_ERROR)
    {
      // Almost singular matrix. Try without the damping factor.
      //if (alfa < max_alfa && alfa > min_alfa)
      sum11 -= error / iteration;
      //if (beta < max_beta && beta > min_beta)
      sum22 -= error / iteration;
      determinant = sum11 * sum22 - sum12 * sum12;
      if (fabs(determinant) < ROUNDING_ERROR)
        // Still singular - stop iterations here
        break;
    }
    delta_alfa = (sum13 * sum22 - sum23 * sum12) / determinant;
    delta_beta = (sum23 * sum11 - sum13 * sum12) / determinant;

    // Stop when we are close enough and have tried hard enough
    if ((fabs(delta_alfa) + fabs(delta_beta)) < 3 * ACCURACY
        && iteration > 3)
      break;

    // Debugging info on the iteration
    if (solver->getLogLevel()>5)
      logger << (fcst ? fcst->getName() : "")
        << ": seasonal : iteration " << iteration
        << ": alfa " << alfa << ", beta " << beta
        << ", smape " << (error_smape_weights ? error_smape / error_smape_weights : 0.0)
        << endl;

    // New values for the next iteration
    alfa += delta_alfa;
    beta += delta_beta;

    // Limit the parameters in their allowed range.
    if (alfa > max_alfa)
      alfa = max_alfa;
    else if (alfa < min_alfa)
      alfa = min_alfa;
    if (beta > max_beta)
      beta = max_beta;
    else if (beta < min_beta)
      beta = min_beta;

    // Verify repeated running with any parameters at the boundary
    if ((beta == min_beta || beta == max_beta)
        && (alfa == min_alfa || alfa == max_alfa))
    {
      if (boundarytested++ > 5) break;
    }
  }

  if (period > fcst->getForecastSkip())
  {
    // Correction on the error: we counted less buckets. We now
    // proportionally increase the error to account for this and have a
    // value that can be compared with the other forecast methods.
    best_smape *= (count - fcst->getForecastSkip());
    best_smape /= (count - period);
  }

  // Restore best results
  alfa = best_alfa;
  beta = best_beta;
  L_i = best_L_i;
  T_i = best_T_i;

  for (unsigned short i = 0; i < period; ++i)
    S_i[i] = best_S_i[i];

  // Echo the result
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": seasonal : "
        << "alfa " << best_alfa
        << ", beta " << best_beta
        << ", smape " << best_smape
        << ", " << iteration << " iterations"
        << ", period " << period
        << ", constant " << L_i
        << ", trend " << T_i
        << ", forecast " << ((L_i + T_i/period) * S_i[count % period])
        << ", standard deviation " << best_standarddeviation
        << ", autocorrelation " << autocorrelation
        << endl;

  // If the autocorrelation is high enough (ie there is a very obvious
  // seasonal pattern) the third element in the return struct is "true".
  // This enforces the use of the seasonal method.
  return Forecast::Metrics(best_smape, best_standarddeviation, autocorrelation > max_autocorrelation);
}


void Forecast::Seasonal::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a linearly changing value
  if (forecast->discrete)
  {
    double carryover = 0.0;
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      L_i += T_i;
      T_i *= dampenTrend; // Reduce slope in the future
      carryover += L_i * S_i[cycleindex];
      double val = ceil(carryover - 0.5);
      carryover -= val;
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        val > 0.0 ? val : 0.0
      );
      if (++cycleindex >= period) cycleindex = 0;
    }
  }
  else
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      L_i += T_i;
      T_i *= dampenTrend; // Reduce slope in the future
      double fcst = L_i * S_i[cycleindex];
      if (L_i * S_i[cycleindex] > 0)
        forecast->setTotalQuantity(
          DateRange(buckets[i-1], buckets[i]),
          fcst > 0.0 ? fcst : 0.0
        );
      if (++cycleindex >= period) cycleindex = 0;
    }
}


//
// CROSTON'S FORECAST METHOD
//


double Forecast::Croston::initial_alfa = 0.1;
double Forecast::Croston::min_alfa = 0.03;
double Forecast::Croston::max_alfa = 1.0;
double Forecast::Croston::min_intermittence = 0.33;


Forecast::Metrics Forecast::Croston::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Count non-zero buckets
  double nonzero = 0.0;
  double totalsum = 0.0;
  for (unsigned long i = 0; i < count; ++i)
    if (history[i])
    {
      ++nonzero;
      totalsum += history[i];
    }

  unsigned int iteration = 0;
  double error_smape = 0.0, error_smape_weights = 0.0, best_smape = 0.0;
  double q_i, p_i;
  double best_error = DBL_MAX, best_alfa = min_alfa, best_f_i = 0.0;
  double best_standarddeviation = 0.0;
  unsigned int between_demands = 1;
  alfa = min_alfa;
  double delta = (max_alfa - min_alfa) / (Forecast::getForecastIterations()-1);
  bool withoutOutliers = false;
  for (; iteration < Forecast::getForecastIterations(); ++iteration)
  {
    // Loop over the outliers 'scan'/0 and 'filter'/1 modes
    double standarddeviation = 0.0;
    double maxdeviation = 0.0;
    for (short outliers = 0; outliers<=1; outliers++)
    {
      // Initialize variables.
      // We initialize to the overall average, since we potentially have
      // very few data points to adjust the forecast.
      error_smape = error_smape_weights = 0.0;
      q_i = totalsum / nonzero;
      p_i = count / nonzero;
      f_i = (1 - alfa / 2) * q_i / p_i;

      // Calculate the forecast and forecast error.
      double history_i = history[0];
      double history_i_min_1 = history[0];  // Note: if the very first point is an outlier, we never detect it as such
      for (unsigned long i = 1; i <= count; ++i)
      {
        history_i_min_1 = history_i;
        history_i = history[i];
        if (history_i_min_1)
        {
          // Non-zero bucket
          q_i = alfa * history_i_min_1 + (1 - alfa) * q_i;
          p_i = alfa * between_demands + (1 - alfa) * p_i;
          f_i = (1 - alfa / 2) * q_i / p_i;
          between_demands = 1;
        }
        else
          ++between_demands;
        if (i == count) break;
        if (outliers == 0)
        {
          // Scan outliers by computing the standard deviation
          // and keeping track of the difference between actuals and forecast
          standarddeviation += (f_i - history[i]) * (f_i - history[i]);
          if (fabs(history[i] - f_i)  > maxdeviation)
            maxdeviation = fabs(f_i - history[i]);
        }
        else
        {
          // Clean outliers from history. Note that there is no correction to the lower
          // limit for the Croston method (because 0's are normal and accepted).
          if (history_i > f_i + Forecast::Forecast_maxDeviation * standarddeviation)
            history_i = f_i + Forecast::Forecast_maxDeviation * standarddeviation;
        }
        if (i >= fcst->getForecastSkip() && p_i > 0) // Don't measure during the warmup period
        {
          if (fabs(f_i + history[i]) > ROUNDING_ERROR)
          {
            error_smape += fabs(f_i - history_i) / fabs(f_i + history_i) * weight[i] * 2;
            error_smape_weights += weight[i];
          }
        }
      }

      // Check outliers
      if (outliers == 0)
      {
        standarddeviation = sqrt(standarddeviation / (count-1));
        maxdeviation /= standarddeviation;
        // Don't repeat if there are no outliers
        if (maxdeviation < Forecast::Forecast_maxDeviation) break;
      }
    } // End loop: 'scan' or 'filter' mode for outliers

    // Better than earlier iterations?
    if (error_smape < best_error)
    {
      best_error = error_smape;
      best_smape = error_smape_weights ? error_smape / error_smape_weights : 0.0;
      best_alfa = alfa;
      best_f_i = f_i;
      best_standarddeviation = standarddeviation;
    }

    // Debugging info on the iteration
    if (solver->getLogLevel()>5)
      logger << (fcst ? fcst->getName() : "")
        << ": croston: iteration " << iteration
        << ": alfa " << alfa
        << ", smape " << (error_smape_weights ? error_smape / error_smape_weights : 0.0)
        << endl;

    // New alfa
    if (delta)
      alfa += delta;
    else
      break; // min_alfa == max_alfa, and no loop is required
  }

  // Keep the best result
  f_i = best_f_i;
  alfa = best_alfa;

  // Echo the result
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": croston : "
        << "alfa " << best_alfa
        << ", smape " << best_smape
        << ", " << iteration << " iterations"
        << ", forecast " << f_i
        << ", standard deviation " << best_standarddeviation
        << endl;
  return Forecast::Metrics(best_smape, best_standarddeviation, false);
}


void Forecast::Croston::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a constant value
  if (forecast->discrete)
  {
    double carryover = 0.0;
    for (unsigned int i = 1; i < bucketcount; ++i)
    {
      carryover += f_i;
      double val = ceil(carryover - 0.5);
      carryover -= val;
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        val > 0.0 ? val : 0.0
      );
    }
  }
  else
    for (unsigned int i = 1; i < bucketcount; ++i)
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        f_i > 0.0 ? f_i : 0.0
      );
}

}       // end namespace
