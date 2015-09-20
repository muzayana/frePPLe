#include "inventoryplanning.h"

namespace module_inventoryplanning
{

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

	for (int i = 0 ; i <= x ; i++)
	{
		sum += getPoissonProbability(mean,i);
	}
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
	else {
		return (pow(mean,x) * exp(-mean))/factorial(x);
	}
}

/***************************************************
function factorial
input : n 
returns : n! that is 1*2*3*4...*n
This function can be improved in the future in case of
performance issues.
We can :
1) create a array of all the possible factorial values until we exceed the c++ largest double
2) We can cache the result of an already calculated value
I would suggest the first option
****************************************************/
double PoissonDistribution::factorial(unsigned int n)
{
	double ret = 1;
	for(unsigned int i = 1; i <= n; ++i)
		ret *= i;
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

	if (roq == 0)
		return 0;

	if (rop+roq == 0)
		return 0;

	double sumFillRate = 0;
	for (int i = 0 ; i < roq ; i++) {
		sumFillRate += getCumulativePoissonProbability(mean, rop + i);
	}

	return sumFillRate/roq;
}

double NormalDistribution::calculateFillRate(double mean, double variance, int rop, int roq) {

	if (mean == 0)
		return 1;

	if (roq == 0)
		return 0;

	if (rop+roq == 0)
		return 0;

	double sumFillRate = 0;
	for (int i = 1 ; i <= roq ; i++) {
		sumFillRate += getNormalDistributionFunction(mean, variance, rop+i);
	}

	return sumFillRate/roq;
}


/***********************************************************************
Function getNormalDistributionFucntion
input : 
mean : The mean of the distribution
variance : the variance of the distribution
x : the variable
output :
The textbook normal distribution value
************************************************************************/
double NormalDistribution::getNormalDistributionFunction(double mean, double variance, int x){
	double z_score = (x - mean) / sqrt(variance);
	return phi(z_score);
}

double NormalDistribution::phi(double x)
{
	// constants
	double a1 =  0.254829592;
	double a2 = -0.284496736;
	double a3 =  1.421413741;
	double a4 = -1.453152027;
	double a5 =  1.061405429;
	double p  =  0.3275911;

	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = fabs(x)/sqrt(2.0);

	double t = 1.0/(1.0 + p*x);
	double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

	return 0.5*(1.0 + sign*y);
}

double NormalDistribution::getNormalProbabilityDensityFunction(double mean, double variance, double x)
{
	double z = (x - mean) / sqrt(variance);
	return (1 / sqrt(2*M_PI)) * exp(-0.5 * z * z);
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
***********************************************************************/
double NegativeBinomialDistribution::negativeBinomialDistributionFunction(int x, double a, double b) {

	double result;
	if (x==0)
		result =  pow(1-b,a);
	else 
		result =  (x+a-1)*b/x*negativeBinomialDistributionFunction(x-1,a,b);

	return result;
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
double NegativeBinomialDistribution::negativeBinomialCumulativeDistributionFunction(int x, double a, double b) {

	double result = 0;
	for (int i = 0 ; i <= x ; i++)
		result += negativeBinomialDistributionFunction(i,a,b);

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
double NegativeBinomialDistribution::calculateFillRate(double mean, double variance, int rop, int roq) {

	if (rop+roq == 0)
		return 0;

	if (mean == 0)
		return 1;

	if (roq == 0)
		return 0;
	
	double varianceToMean = variance/mean;

	double a = mean/(varianceToMean-1);
	double b = (varianceToMean-1)/varianceToMean;

	double sumFillRate = 0;
	for (int i = 0 ; i < roq ; i++) {
		sumFillRate += negativeBinomialCumulativeDistributionFunction(rop+i, a, b);
	}

	return sumFillRate/roq;

}


}// close namespace