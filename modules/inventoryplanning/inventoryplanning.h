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
    static Date startdate;

    /** End date of the horizon for which to compute the SS and ROQ. */
    static Date enddate;

    static double fixed_order_cost;

    static double holding_cost;

  public:
    static const Keyword tag_fixed_order_cost;
    static const Keyword tag_holding_cost;

    Calendar* getCalendar() const
    {
      return cal;
    }

    void setCalendar(Calendar* c)
    {
      cal = c;
    }

    Date getStart() const
    {
      return startdate;
    }

    void setStart(Date d)
    {
      startdate = d;
    }

    Date getEnd() const
    {
      return enddate;
    }

    void setEnd(Date d)
    {
      enddate = d;
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
      m->addDateField<Cls>(Tags::start, &Cls::getStart, &Cls::setStart);
      m->addDateField<Cls>(Tags::end, &Cls::getEnd, &Cls::setEnd);
      m->addDoubleField<Cls>(tag_fixed_order_cost, &Cls::getFixedOrderCost, &Cls::setFixedOrderCost);
      m->addDoubleField<Cls>(tag_holding_cost, &Cls::getHoldingCost, &Cls::setHoldingCost);
    }
};

}   // End namespace

#endif


