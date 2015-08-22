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
  public:
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
      // No fields yet to define
    }
};

}   // End namespace

#endif


