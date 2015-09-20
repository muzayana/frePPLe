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
      return fixed_order_cost;
    }

    void setHoldingCost(double d)
    {
      if (d <= 0)
        throw DataException("Holding cost must be greater than 0");
      fixed_order_cost = d;
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

	static int calulateStockLevel(double mean, double variance, int roq, double fillRateMinimum, double fillRateMaximum, bool minimumStrongest, string distribution);
	static double calculateFillRate(double mean, double variance, int rop, int roq, string distribution);
};


class PoissonDistribution
{
  public:
	  static double calculateFillRate(double mean, int rop, int roq);

  private:
    static double getCumulativePoissonProbability(double mean, int x);
	  static double getPoissonProbability(double mean, int x);
	  static double factorial(unsigned int n);
};


class NormalDistribution
{
  public:
	  static double calculateFillRate(double mean, double variance, int rop, int roq);

  private:
	  static inline double getNormalProbabilityDensityFunction(double mean, double variance, double x)
    {
	    double z = (x - mean) / sqrt(variance);
	    return (1 / sqrt(2*3.14159265358979323846)) * exp(-0.5 * z * z);
    }

	  static inline double getNormalDistributionFunction(double mean, double variance, int x)
    {
      assert(variance > 0);
	    return phi((x - mean) / sqrt(variance));
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


