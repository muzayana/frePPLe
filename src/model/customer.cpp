/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2015 by frePPLe bvba                                 *
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

template<class Customer> DECLARE_EXPORT Tree utils::HasName<Customer>::st;
DECLARE_EXPORT const MetaCategory* Customer::metadata;
DECLARE_EXPORT const MetaClass* CustomerDefault::metadata;


int Customer::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<Customer>("customer", "customers", reader, writer, finder);
  registerFields<Customer>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python class
  return FreppleCategory<Customer>::initialize();
}


int CustomerDefault::initialize()
{
  // Initialize the metadata
  CustomerDefault::metadata = MetaClass::registerClass<CustomerDefault>(
    "customer", "customer_default",
    Object::create<CustomerDefault>, true
    );

  // Initialize the Python class
  return FreppleClass<CustomerDefault,Customer>::initialize();
}


DECLARE_EXPORT Customer::~Customer()
{
  // Remove all references from demands to this customer
  for (Demand::iterator i = Demand::begin(); i != Demand::end(); ++i)
    if (i->getCustomer() == this) i->setCustomer(NULL);
}

} // end namespace
