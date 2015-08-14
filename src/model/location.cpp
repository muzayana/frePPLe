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

template<class Location> DECLARE_EXPORT Tree utils::HasName<Location>::st;
DECLARE_EXPORT const MetaCategory* Location::metadata;
DECLARE_EXPORT const MetaClass* LocationDefault::metadata;


int Location::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<Location>("location", "locations", reader, finder);
  registerFields<Location>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python class
  return FreppleCategory<Location>::initialize();
}


int LocationDefault::initialize()
{
  // Initialize the metadata
  LocationDefault::metadata = MetaClass::registerClass<LocationDefault>("location", "location_default",
      Object::create<LocationDefault>, true);

  // Initialize the Python class
  return FreppleClass<LocationDefault,Location>::initialize();
}


DECLARE_EXPORT Location::~Location()
{
  // Remove all references from buffers to this location
  for (Buffer::iterator buf = Buffer::begin();
      buf != Buffer::end(); ++buf)
    if (buf->getLocation() == this)
      buf->setLocation(NULL);

  // Remove all references from resources to this location
  for (Resource::iterator res = Resource::begin();
      res != Resource::end(); ++res)
    if (res->getLocation() == this)
      res->setLocation(NULL);

  // Remove all references from operations to this location
  for (Operation::iterator oper = Operation::begin();
      oper != Operation::end(); ++oper)
    if (oper->getLocation() == this)
      oper->setLocation(NULL);

  // Remove all references from demands to this location
  for (Demand::iterator dmd = Demand::begin();
      dmd != Demand::end(); ++dmd)
    if (dmd->getLocation() == this)
      dmd->setLocation(NULL);

  // Remove all item suppliers referencing this location
  for (Supplier::iterator sup = Supplier::begin();
    sup != Supplier::end(); ++sup)
  {
    for (Supplier::itemlist::const_iterator it = sup->getItems().begin();
      it != sup->getItems().end(); )
    {
      if (it->getLocation() == this)
      {
        const ItemSupplier *itemsup = &*it;
        ++it;   // Advance iterator before the delete
        delete itemsup;
      }
      else
        ++it;
    }
  }

  // The ItemDistribution objects are automatically deleted by the
  // destructor of the Association list class.
}

} // end namespace
