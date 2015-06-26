/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2015 by Johan De Taeye, frePPLe bvba                 *
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
  metadata = MetaCategory::registerCategory<Location>("location", "locations", reader, writer, finder);
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
    if (buf->getLocation() == this) buf->setLocation(NULL);

  // Remove all references from resources to this location
  for (Resource::iterator res = Resource::begin();
      res != Resource::end(); ++res)
    if (res->getLocation() == this) res->setLocation(NULL);

  // Remove all references from operations to this location
  for (Operation::iterator oper = Operation::begin();
      oper != Operation::end(); ++oper)
    if (oper->getLocation() == this) oper->setLocation(NULL);
}

} // end namespace
