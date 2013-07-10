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

#define ACCURACY 0.01


double Forecast::Forecast_maxDeviation = 4.0;


void Forecast::generateFutureValues(
  const double history[], unsigned int historycount,
  const Date buckets[], unsigned int bucketcount,
  ForecastSolver* solver)
{
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
  int numberOfMethods = 4;
  ForecastMethod* methods[4];

  // Rules to determine which forecast methods can be applied
  methods[0] = &moving_avg;
  if (historycount < getForecastSkip() + 5)
    // Too little history: only use moving average
    numberOfMethods = 1;
  else
  {
    unsigned int zero = 0;
    for (unsigned long i = 0; i < historycount; ++i)
      if (!history[i]) ++zero;
    if (zero > Croston::getMinIntermittence() * historycount)
    {
      // If there are too many zeros: use croston or moving average.
      numberOfMethods = 2;
      methods[1] = &croston;
    }
    else
    {
      // The most common case: enough values and not intermittent
      methods[1] = &single_exp;
      methods[2] = &double_exp;
      methods[3] = &seasonal;
    }
  }

  // Initialize a vector with the smape weights
  double *weight = new double[historycount+1];
  weight[historycount] = 1.0;
  for (int i=historycount-1; i>=0; --i)
    weight[i] = weight[i+1] * Forecast::getForecastSmapeAlfa();

  // Evaluate each forecast method
  double best_error = DBL_MAX;
  int best_method = -1;
  double error;
  try
  {
    for (int i=0; i<numberOfMethods; ++i)
    {
      error = methods[i]->generateForecast(this, history, historycount, weight, solver);
      if (error<best_error)
      {
        best_error = error;
        best_method = i;
      }
    }
  }
  catch (...)
  {
    delete weight;
    throw;
  }
  delete weight;

  // Apply the most appropriate forecasting method
  if (best_method >= 0)
  {
    if (solver->getLogLevel()>0)
      logger << getName() << ": chosen method: " << methods[best_method]->getName() << endl;
    methods[best_method]->applyForecast(this, buckets, bucketcount);
  }
}


//
// MOVING AVERAGE FORECAST
//


unsigned int Forecast::MovingAverage::defaultorder = 5;


double Forecast::MovingAverage::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  double error_smape = 0.0;

  // Calculate the forecast and forecast error.
  for (unsigned int i = 1; i <= count; ++i)
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
    if (i >= fcst->getForecastSkip() && i < count && avg + history[i] > ROUNDING_ERROR)
      error_smape += fabs(avg - history[i]) / (avg + history[i]) * weight[i];
  }

  // Echo the result
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": moving average : "
        << "smape " << error_smape
        << ", forecast " << avg << endl;
  return error_smape;
}


void Forecast::MovingAverage::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a constant value
  if (avg < 0) return;
  for (unsigned int i = 1; i < bucketcount; ++i)
    forecast->setTotalQuantity(
      DateRange(buckets[i-1], buckets[i]),
      avg
    );
}


//
// SINGLE EXPONENTIAL FORECAST
//


double Forecast::SingleExponential::initial_alfa = 0.2;
double Forecast::SingleExponential::min_alfa = 0.03;
double Forecast::SingleExponential::max_alfa = 1.0;


double Forecast::SingleExponential::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Verify whether this is a valid forecast method.
  //   - We need at least 5 buckets after the warmup period.
  if (count < fcst->getForecastSkip() + 5)
    return DBL_MAX;

  unsigned int iteration = 1;
  bool upperboundarytested = false;
  bool lowerboundarytested = false;
  double error = 0.0, error_smape = 0.0, best_smape = 0.0, delta, df_dalfa_i, sum_11, sum_12;
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
      df_dalfa_i = sum_11 = sum_12 = error_smape = error = 0.0;

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
          if (f_i + history[i] > ROUNDING_ERROR)
            error_smape += fabs(f_i - history_i) / (f_i + history_i) * weight[i];
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
      best_smape = error_smape;
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
        << ", smape " << error_smape << endl;

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
        << ", standard deviation " << best_standarddeviation << endl;
  return best_smape;
}


void Forecast::SingleExponential::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a constant value
  if (f_i < 0) return;
  for (unsigned int i = 1; i < bucketcount; ++i)
    forecast->setTotalQuantity(
      DateRange(buckets[i-1], buckets[i]),
      f_i
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


double Forecast::DoubleExponential::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Verify whether this is a valid forecast method.
  //   - We need at least 5 buckets after the warmup period.
  if (count < fcst->getForecastSkip() + 5)
    return DBL_MAX;

  // Define variables
  double error = 0.0, error_smape = 0.0, delta_alfa, delta_gamma, determinant;
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
      error = error_smape = sum11 = sum12 = sum22 = sum13 = sum23 = 0.0;
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
        sum23 += weight[i] * d_forecast_d_gamma * (history_i_min_1 - constant_i - trend_i);
        if (i >= fcst->getForecastSkip()) // Don't measure during the warmup period
        {
          error += (constant_i + trend_i - history_i) * (constant_i + trend_i - history_i) * weight[i];
          if (constant_i + trend_i + history_i > ROUNDING_ERROR)
            error_smape += fabs(constant_i + trend_i - history_i) / (constant_i + trend_i + history_i) * weight[i];
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
      best_smape = error_smape;
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
        << ", smape " << error_smape << endl;

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
        << ", standard deviation " << best_standarddeviation << endl;
  return best_smape;
}


void Forecast::DoubleExponential::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a linearly changing value
  for (unsigned int i = 1; i < bucketcount; ++i)
  {
    constant_i += trend_i;
    trend_i *= dampenTrend; // Reduce slope in the future
    if (constant_i > 0)
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        constant_i
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
  double prev = 10.0;
  double prevprev = 10.0;
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
    if ((p > min_period + 1 && prev > prevprev*1.1) && prev > correlation*1.1 && prev > 0.3)
    {
      period = p - 1;
      return;
    }
    prevprev = prev;
    prev = correlation;
  }
}


double Forecast::Seasonal::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  // Check for seasonal cycles
  detectCycle(history, count);

  // Return if no seasonality is found
  if (!period) return DBL_MAX;

  // Define variables
  double error = 0.0, error_smape = 0.0, determinant, delta_alfa, delta_beta;
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
  double best_S_i[24], best_L_i, best_T_i;

  // Compute initialization values for the timeseries and seasonal index.
  // L_i = average over first cycle
  // T_i = average delta measured in second cycle
  // S_i[index] = actual divided by average in first cycle
  double L_i_initial = 0.0;
  for (unsigned long i = 0; i < period; ++i)
    L_i_initial += history[i];
  L_i_initial /= period;
  double T_i_initial = 0.0;
  for (unsigned long i = 0; i < period; ++i)
  {
    T_i_initial += history[i+period] - history[i];
    S_i[i] = history[i] / L_i;
  }
  T_i_initial /= period * period;

  // Iterations
  double L_i_prev;
  unsigned int iteration = 1, boundarytested = 0;
  for (; iteration <= Forecast::getForecastIterations(); ++iteration)
  {
    // Initialize variables
    error = error_smape = sum11 = sum12 = sum13 = sum22 = sum23 = 0.0;
    d_L_d_alfa = d_L_d_beta = 0.0;
    d_T_d_alfa = d_T_d_beta = 0.0;
    L_i = L_i_initial;
    T_i = T_i_initial;
    for (unsigned long i = 0; i < period; ++i)
    {
      S_i[i] = history[i] / L_i;
      d_S_d_alfa[i] = 0.0;
      d_S_d_beta[i] = 0.0;
    }

    // Calculate the forecast and forecast error.
    // We also compute the sums required for the Marquardt optimization.
    cycleindex = 0;
    for (unsigned long i = period; i <= count; ++i)
    {
      // Base calculations
      L_i_prev = L_i;
      if (S_i[cycleindex] > ROUNDING_ERROR)
        L_i = alfa * history[i-1] / S_i[cycleindex] + (1 - alfa) * (L_i + T_i);
      else
        L_i = (1 - alfa) * (L_i + T_i);
      T_i = beta * (L_i - L_i_prev) + (1 - beta) * T_i;
      S_i[cycleindex] = gamma * history[i-1] / L_i + (1 - gamma) * S_i[cycleindex];
      if (i == count) break;
      // Calculations for the delta of the parameters
      d_L_d_alfa_prev = d_L_d_alfa;
      d_L_d_beta_prev = d_L_d_beta;
      d_T_d_alfa_prev = d_T_d_alfa;
      d_T_d_beta_prev = d_T_d_beta;
      d_S_d_alfa_prev = d_S_d_alfa[cycleindex];
      d_S_d_beta_prev = d_S_d_beta[cycleindex];
      d_L_d_alfa = history[i-1] / S_i[cycleindex]
         - alfa * history[i-1] / S_i[cycleindex] /  S_i[cycleindex] * d_S_d_alfa_prev
         - (L_i + T_i)
         + (1 - alfa) * (d_L_d_alfa_prev + d_T_d_alfa_prev);
      d_L_d_beta = - alfa * history[i-1] / S_i[cycleindex] /  S_i[cycleindex] * d_S_d_beta_prev
        + (1 - alfa) * (d_L_d_beta_prev + d_T_d_beta_prev);
      d_S_d_alfa[cycleindex] = - gamma * history[i-1] / L_i / L_i * d_L_d_alfa_prev
        + (1 - gamma) * d_S_d_alfa_prev;
      d_S_d_beta[cycleindex] = - gamma * history[i-1] / L_i / L_i * d_L_d_beta_prev
        + (1 - gamma) * d_S_d_beta_prev;
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
        if (fcst + history[i] > ROUNDING_ERROR)
          error_smape += fabs(fcst - history[i]) / (fcst + history[i]) * weight[i];
      }
      if (++cycleindex >= period) cycleindex = 0;
    }

    // Better than earlier iterations?
    best_smape = error_smape;
    if (error < best_error)
    {
      best_error = error;
      best_smape = error_smape;
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
        << ", smape " << error_smape << endl;

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
    // Correction on the error: we counted less buckets. We now
    // proportionally increase the error to account for this and have a
    // value that can be compared with the other forecast methods.
    best_smape *= (count - fcst->getForecastSkip()) / (count - period);

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
        << ", forecast " << ((L_i + T_i) * S_i[count % period])
        << ", standard deviation " << best_standarddeviation
        << endl;
  return best_smape;
}


void Forecast::Seasonal::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a linearly changing value
  for (unsigned int i = 1; i < bucketcount; ++i)
  {
    L_i += T_i;
    T_i *= dampenTrend; // Reduce slope in the future
    if (L_i * S_i[cycleindex] > 0)
      forecast->setTotalQuantity(
        DateRange(buckets[i-1], buckets[i]),
        L_i * S_i[cycleindex]
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


double Forecast::Croston::generateForecast
(Forecast* fcst, const double history[], unsigned int count, const double weight[], ForecastSolver* solver)
{
  unsigned int iteration = 1;
  bool upperboundarytested = false;
  bool lowerboundarytested = false;
  double error = 0.0, error_smape = 0.0, best_smape = 0.0, delta;
  double q_i, p_i, d_p_i, d_q_i, d_f_i, sum1, sum2;
  double best_error = DBL_MAX, best_alfa = initial_alfa, best_f_i = 0.0;
  double best_standarddeviation = 0.0;
  unsigned int between_demands = 1;
  for (; iteration <= Forecast::getForecastIterations(); ++iteration)
  {
    // Loop over the outliers 'scan'/0 and 'filter'/1 modes
    double standarddeviation = 0.0;
    double maxdeviation = 0.0;
    for (short outliers = 0; outliers<=1; outliers++)
    {
      // Initialize variables
      error_smape = error = d_p_i = d_q_i = d_f_i = sum1 = sum2 = 0.0;

      // Initialize the iteration.
      q_i = f_i = history[0];
      p_i = 0;

      // Calculate the forecast and forecast error.
      // We also compute the sums required for the Marquardt optimization.
      double history_i = history[0];
      double history_i_min_1 = history[0];
      for (unsigned long i = 1; i <= count; ++i)
      {
        history_i_min_1 = history_i;
        history_i = history[i];
        if (history_i_min_1)
        {
          // Non-zero bucket
          d_p_i = between_demands - p_i + (1 - alfa) * d_p_i;
          d_q_i = history_i_min_1 - q_i + (1 - alfa) * d_q_i;
          q_i = alfa * history_i_min_1 + (1 - alfa) * q_i;
          p_i = alfa * between_demands + (1 - alfa) * q_i;
          f_i = q_i / p_i;
          d_f_i = (d_q_i - d_p_i * q_i / p_i) / p_i;
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
          if (history[i] - f_i  > maxdeviation)
            maxdeviation = f_i - history[i];
        }
        else
        {
          // Clean outliers from history. Note that there is no correction to the lower
          // limit for the Croston method (because 0's are normal and accepted).
          if (history_i > f_i + Forecast::Forecast_maxDeviation * standarddeviation)
            history_i = f_i + Forecast::Forecast_maxDeviation * standarddeviation;
        }
        sum1 += weight[i] * d_f_i * (history_i - f_i);
        sum2 += weight[i] * d_f_i * d_f_i;
        if (i >= fcst->getForecastSkip() && p_i > 0)
        {
          error += (f_i - history_i) * (f_i - history_i) * weight[i];
          if (f_i + history[i] > ROUNDING_ERROR)
            error_smape += fabs(f_i - history_i) / (f_i + history_i) * weight[i];
        }
      }

      // Check outliers
      if (outliers == 0)
      {
        standarddeviation = sqrt(standarddeviation / (count-1));
        maxdeviation /= standarddeviation;
        // Don't repeat if there are no outliers
        if (maxdeviation < Forecast::Forecast_maxDeviation) break;
        else logger << "detecting CR outlier! " << maxdeviation<< endl;
      }
    } // End loop: 'scan' or 'filter' mode for outliers

    // Better than earlier iterations?
    if (error < best_error)
    {
      best_error = error;
      best_smape = error_smape;
      best_alfa = alfa;
      best_f_i = f_i;
      best_standarddeviation = standarddeviation;
    }

    // Add Levenberg - Marquardt damping factor
    if (fabs(sum2 + error / iteration) > ROUNDING_ERROR)
      sum2 += error / iteration;

    // Calculate a delta for the alfa parameter
    if (fabs(sum2) < ROUNDING_ERROR) break;
    delta = sum1 / sum2;

    // Debugging info on the iteration
    if (solver->getLogLevel()>5)
      logger << (fcst ? fcst->getName() : "")
        << ": croston: iteration " << iteration
        << ": alfa " << alfa
        << ", smape " << error_smape << endl;

    // Stop when we are close enough and have tried hard enough
    if (fabs(delta) < ACCURACY && iteration > 3) break;

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
  alfa = best_alfa;

  // Echo the result
  if (solver->getLogLevel()>0)
    logger << (fcst ? fcst->getName() : "") << ": croston : "
        << "alfa " << best_alfa
        << ", smape " << best_smape
        << ", " << iteration << " iterations"
        << ", forecast " << f_i
        << ", standard deviation " << best_standarddeviation << endl;
  return best_smape;
}


void Forecast::Croston::applyForecast
(Forecast* forecast, const Date buckets[], unsigned int bucketcount)
{
  // Loop over all buckets and set the forecast to a constant value
  if (f_i < 0) return;
  for (unsigned int i = 1; i < bucketcount; ++i)
    forecast->setTotalQuantity(
      DateRange(buckets[i-1], buckets[i]),
      f_i
    );
}

}       // end namespace
