/***************************************************************************
 *                                                                         *
 * Copyright (C) 2014 by Johan De Taeye, frePPLe bvba                      *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#define FREPPLE_CORE
#include "frepple/model.h"

namespace frepple
{

template<class Supplier> DECLARE_EXPORT Tree utils::HasName<Supplier>::st;
DECLARE_EXPORT const MetaCategory* Supplier::metadata;
DECLARE_EXPORT const MetaClass* SupplierDefault::metadata;


int Supplier::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<Supplier>("supplier", "suppliers", reader, writer, finder);
  registerFields<Supplier>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python class
  return FreppleCategory<Supplier>::initialize();
}


int SupplierDefault::initialize()
{
  // Initialize the metadata
  SupplierDefault::metadata = MetaClass::registerClass<SupplierDefault>(
    "supplier", "supplier_default",
    Object::create<SupplierDefault>, true
    );

  // Initialize the Python class
  return FreppleClass<SupplierDefault,Supplier>::initialize();
}


DECLARE_EXPORT Supplier::~Supplier()
{
}

} // end namespace
