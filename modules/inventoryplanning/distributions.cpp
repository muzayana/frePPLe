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

namespace module_inventoryplanning
{


double PoissonDistribution::factorialcache[150];


distribution InventoryPlanningSolver::matchDistributionName(string& name)
{
  if (name == "Automatic")
    return AUTOMATIC;
  if (name == "Poisson")
    return POISSON;
  if (name == "Normal")
    return NORMAL;
  if (name == "Negative Binomial")
    return NEGATIVE_BINOMIAL;
  throw DataException("Invalid distribution name");
}


/*****************************************************
function getCumulativePoissonProbability
input :
mean : the mean of the distribution.
x : The variable
returns :
The textbook cumulative poisson value. As the poisson distribution is discrete,
this is the sum from 0 to x for of the poisson distribution function
*****************************************************/
double PoissonDistribution::getCumulativePoissonProbability(double mean, int x)
{
	if (x < 0)
		return 0;

	double sum = 0;
	for (int i = 0 ; i <= x ; ++i)
		sum += getPoissonProbability(mean,i);
	return sum;
}


/*****************************************************
function getPoissonProbability
input :
mean : the mean of the distribution.
x : The variable
returns :
The textbook poisson distribution value.
*****************************************************/
double PoissonDistribution::getPoissonProbability(double mean, int x)
{
	if (x < 0)
		return 0;
	else
		return pow(mean,x) * exp(-mean) / factorial(x);
}


double PoissonDistribution::factorial(unsigned int n)
{
  // Protect for overflow
  // Highest number that can be represented is DBL_MAX = 1.79769e+308
  // and 170! = 7.2574156153e+306
  if (n > 170)
    throw DataException("Factorial is too large to compute");

  if (n < 150 && factorialcache[n])
    return factorialcache[n];

	double ret = 1;
	for (unsigned int i = n; i > 0; --i)
  {
    if (i < 150 && factorialcache[i])
    {
      // The factorial of a lower value already exists
      factorialcache[n] = factorialcache[i] * ret;
      return factorialcache[n];
    }
    else
		  ret *= i;
  }
  if (n < 150)
    factorialcache[n] = ret;
	return ret;
}


/**************************************************
function calculateFillRate
input :
mean is the demand during the lead time
rop and roq are reorder point and reorder quantity
returns : The fill rate as a double in the [0-1] range calculated as the cumulative Poisson distribution
***************************************************/
double PoissonDistribution::calculateFillRate(double mean, int rop, int roq)
{
	if (mean == 0)
		return 1;

	if (roq == 0 || rop+roq == 0)
		return 0;

	double sumFillRate = 0;
  // The loop below is an optimized version of the nested loop:
	//   for (int i = 0 ; i < roq ; ++i)
	//     sumFillRate += getCumulativePoissonProbability(mean, rop + i);
  int times = roq;
  for (int i = 0 ; i < rop + roq ; ++i)
  {
    double tmp = times * getPoissonProbability(mean, i);
		sumFillRate += tmp;
    if (i >= rop)
      --times;
    if (tmp < 1e-06 && i > mean)
      // Contribution is getting ridiculously low and will get even lower
      break;
  }
	return sumFillRate / roq;
}


double NormalDistribution::calculateFillRate(double mean, double variance, int rop, int roq)
{
	if (mean == 0)
		return 1;

	if (roq == 0)
		return 0;

	if (rop+roq == 0)
		return 0;

  double stddev = sqrt(variance);
	double sumFillRate = 0;
  if (roq <= 100)
  {
    // For low ROQ values, we evaluate all possible values between ROP+1 and ROP+ROQ
	  for (int i = 1 ; i <= roq ; ++i)
    {
		  double tmp = getNormalDistributionFunction(mean, stddev, rop+i);
      if (tmp < 1e-06 && i > mean)
        // Contribution is getting ridiculously low and will get even lower
        break;
      sumFillRate += tmp;
    }
	  return sumFillRate/roq;
  }
  else
  {
    // For high ROQ values, we sample 100 values between ROP+1 and ROP+ROQ
    unsigned short samples = 0;
    double counter = 0.0;
    double prevcounter = -1.0;
    double step = 100.0 / roq;
	  for (int i = 1 ; i <= roq ; ++i)
    {
		  counter += step;
      double tmp = floor(counter);
      if (tmp <= prevcounter)
        continue;
      ++samples;
      prevcounter = tmp;
      sumFillRate += getNormalDistributionFunction(mean, stddev, rop+i);
    }
	  return sumFillRate / samples;
  }
}


double NormalDistribution::phi(double x)
{
	// constants
	static const double a1 =  0.254829592;
	static const double a2 = -0.284496736;
	static const double a3 =  1.421413741;
	static const double a4 = -1.453152027;
	static const double a5 =  1.061405429;
	static const double p  =  0.3275911;

	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = fabs(x)/sqrt(2.0);

	double t = 1.0 / (1.0 + p*x);
	double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

	return 0.5*(1.0 + sign*y);
}


/***********************************************************************
function negativeBinomialDistributionFunction is the textbook definition
of the negative binomial distribution mass function.
This is based on Sherbrooke's book, page 62.
It defines the probability that it takes a + x trials to achieve exactly
 a successes where each trial has a probability of success equal to
 (1 - b). This is just the binomial probability for a - 1 successes in
 the first a + x - 1 trials times the probability that the next trial is a success.
input :
a  Must be positive is the threshold of successes to reach
b is a double that must be in the interval ]0;1[ is the probability of failures, 1-b is the probability of success
x the variable, integer as the negative binomial distribution is discrete, the number of failures
output :
a double in the [0;1] interval representing the probability.

See also http://scialert.net/fulltext/?doi=itj.2013.688.695

***********************************************************************/
double NegativeBinomialDistribution::negativeBinomialDistributionFunction(int x, double a, double b)
{
	if (x)
		return (x + a - 1) * b / x * negativeBinomialDistributionFunction(x-1, a, b);
	else
		return pow(1-b, a);
}


/****************************************************************
function negativeBinomialCumulativeDistributionFunction
It sums all the probabilities for i in [0;x] range
This function calls x+1 times function negativeBinomialDistributionFunction
a  Must be positive is the threshold of successes to reach
b is a double that must be in the interval ]0;1[ is the probability of failures, 1-b is the probability of success
x the variable, integer as the negative binomial distribution is discrete, the number of failures
output :
a double in the [0;1] interval representing the probability.
****************************************************************/
double NegativeBinomialDistribution::negativeBinomialCumulativeDistributionFunction(int x, double a, double b)
{
	double result = 0;
	for (int i = 0 ; i <= x ; ++i)
		result += negativeBinomialDistributionFunction(i, a, b);
	return result;
}


/****************************************************************
function calculateFillRate
calculates the fill rate given
mean : the mean of the distribution
variance : the variance of the distribution
rop : reorder point
roq : reorder quantity
output :
the fill rate based on the negative binomial distribution
****************************************************************/
double NegativeBinomialDistribution::calculateFillRate(double mean, double variance, int rop, int roq)
{
	if (rop+roq == 0)
		return 0;

	if (mean == 0)
		return 1;

	if (roq == 0)
		return 0;

	double varianceToMean = variance / mean;

	double a = mean / (varianceToMean - 1);
	double b = (varianceToMean - 1) / varianceToMean;

	double sumFillRate = 0;
  // The loop below is an optimized version of the nested loop:
  //   for (int i = 0 ; i < roq ; ++i)
	//     sumFillRate += negativeBinomialCumulativeDistributionFunction(rop + i, a, b);
  int times = roq;
  for (int i = 0 ; i < rop + roq ; ++i)
  {
    double tmp = times * negativeBinomialDistributionFunction(i, a, b);
		sumFillRate += tmp;
    if (i >= rop)
      --times;
    if (tmp < 1e-06 && i > mean)
      // Contribution is getting ridiculously low and will get even lower
      break;
  }
	return sumFillRate / roq;
}


} // close namespace