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

#pragma once
#ifndef INVENTORYPLANNING_H
#define INVENTORYPLANNING_H

#include "frepple.h"
using namespace frepple;


namespace module_inventoryplanning
{

/** Initialization routine for the library. */
MODULE_EXPORT const char* initialize(const Environment::ParameterList&);


/** Statistical distributions. */
enum distribution
{
  AUTOMATIC = 1,
  NORMAL = 2,
  POISSON = 3,
  NEGATIVE_BINOMIAL = 4
};


class InventoryPlanningSolver : public Solver
{
  private:
    /** Calendar to define the bucket size.
      * The attribute is made static to avoid having to set the values for
      * every solver object we create.
      */
    static Calendar *cal;

    /** Start date of the horizon for which to compute the SS & ROQ. */
    static int horizon_start;

    /** End date of the horizon for which to compute the SS and ROQ. */
    static int horizon_end;

    /** Average bucket size. */
    static Duration bucket_size;

    static double fixed_order_cost;

    static double holding_cost;

  public:
    static const Keyword tag_fixed_order_cost;
    static const Keyword tag_holding_cost;
    static const Keyword tag_horizon_start;
    static const Keyword tag_horizon_end;

    Calendar* getCalendar() const
    {
      return cal;
    }

    void setCalendar(Calendar* c)
    {
      cal = c;
    }

    int getHorizonStart() const
    {
      return horizon_start;
    }

    void setHorizonStart(int d)
    {
      horizon_start = d;
    }

    int getHorizonEnd() const
    {
      return horizon_end;
    }

    void setHorizonEnd(int d)
    {
      horizon_end = d;
    }

    double getFixedOrderCost() const
    {
      return fixed_order_cost;
    }

    void setFixedOrderCost(double d)
    {
      if (d <= 0)
        throw DataException("Fixed order cost must be greater than 0");
      fixed_order_cost = d;
    }

    double getHoldingCost() const
    {
      return holding_cost;
    }

    void setHoldingCost(double d)
    {
      if (d <= 0)
        throw DataException("Holding cost must be greater than 0");
      holding_cost = d;
    }

    static int initialize();
    virtual const MetaClass& getType() const {return *metadata;}
    static const MetaClass *metadata;
    virtual size_t getSize() const
    {
      return sizeof(InventoryPlanningSolver);
    }

    static PyObject* create(PyTypeObject*, PyObject*, PyObject*);

    /** Constructor. */
    InventoryPlanningSolver()
    {
      initType(metadata);
    }

    virtual void solve(void* = NULL);
    virtual void solve(const Buffer*,void* = NULL);

    template<class Cls> static inline void registerFields(MetaClass* m)
    {
      m->addPointerField<Cls, Calendar>(Tags::calendar, &Cls::getCalendar, &Cls::setCalendar);
      m->addIntField<Cls>(tag_horizon_start, &Cls::getHorizonStart, &Cls::setHorizonStart);
      m->addIntField<Cls>(tag_horizon_end, &Cls::getHorizonEnd, &Cls::setHorizonEnd);
      m->addDoubleField<Cls>(tag_fixed_order_cost, &Cls::getFixedOrderCost, &Cls::setFixedOrderCost);
      m->addDoubleField<Cls>(tag_holding_cost, &Cls::getHoldingCost, &Cls::setHoldingCost);
    }

  static distribution matchDistributionName(string&);
  static distribution chooseDistribution(double mean, double variance);
  static int calulateStockLevel(double mean, double variance, int roq, double fillRateMinimum, double fillRateMaximum, bool minimumStrongest, distribution dist);
  static double calculateFillRate(double mean, double variance, int rop, int roq, distribution dist);
};


class PoissonDistribution
{
  public:
	  static double calculateFillRate(double mean, int rop, int roq);
    static void init()
    {
      for(int i = 0; i < 150; ++i)
        factorialcache[i] = 0.0;
      factorialcache[0] = 1;
    }

  private:
    static double getCumulativePoissonProbability(double mean, int x);
	  static double getPoissonProbability(double mean, int x);

    /** Computes the factorial of an integer number N as 1*2*3*4...*N */
	  static double factorial(unsigned int n);

    /** Cache variable to compute the factorial only once. */
    static double factorialcache[150];
};


class NormalDistribution
{
  public:
	  static double calculateFillRate(double mean, double variance, int rop, int roq);

  private:
	  static inline double getNormalProbabilityDensityFunction(double mean, double stddev, double x)
    {
      if (!stddev)
        return (x > mean) ? 1.0 : 0.0;
	    double z = (x - mean) / stddev;
	    return (1 / sqrt(2*3.14159265358979323846)) * exp(-0.5 * z * z);
    }

	  static inline double getNormalDistributionFunction(double mean, double stddev, int x)
    {
      if (!stddev)
        return (x > mean) ? 1.0 : 0.0;
      double z = (x - mean) / stddev;
      if (z < -4)
        return 0.0;
      if (z > 8)
        return 1.0;
	    return phi(z);
    }

    static double phi(double x);
};


class NegativeBinomialDistribution
{
  public:
	  static double calculateFillRate(double mean, double variance, int rop, int roq);
  private:
	  static double negativeBinomialDistributionFunction(int x, double a, double b);
	  static double negativeBinomialCumulativeDistributionFunction(int x, double a, double b);
};

}   // End namespace

#endif


