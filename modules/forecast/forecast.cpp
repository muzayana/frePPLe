/***************************************************************************
 *                                                                         *
 * Copyright (C) 2012 by Johan De Taeye, frePPLe bvba                      *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "forecast.h"

namespace module_forecast
{

const Keyword Forecast::tag_total("total");
const Keyword Forecast::tag_net("net");
const Keyword Forecast::tag_consumed("consumed");
const Keyword Forecast::tag_planned("planned");
const Keyword Forecast::tag_methods("methods");
const MetaClass *Forecast::metadata;
const MetaClass *ForecastBucket::metadata;
bool ForecastBucket::DueAtEndOfBucket = false;


int Forecast::initialize()
{
  // Initialize the metadata
  metadata = new MetaClass("demand", "demand_forecast",
      Object::createString<Forecast>);

  // Get notified when a calendar is deleted
  FunctorStatic<Calendar,Forecast>::connect(SIG_REMOVE);

  // Initialize the Python class
  FreppleClass<Forecast,Demand>::getType().addMethod("setQuantity", Forecast::setPythonTotalQuantity, METH_VARARGS,
      "Update the total quantity in one or more buckets");
  return FreppleClass<Forecast,Demand>::initialize();
}


int ForecastBucket::initialize()
{
  // Initialize the metadata
  // No factory method for this class
  metadata = new MetaClass("demand", "demand_forecastbucket");

  // Initialize the Python class
  // No support for creation
  PythonType& x = FreppleClass<ForecastBucket,Demand>::getType();
  x.setName("demand_forecastbucket");
  x.setDoc("frePPLe forecastbucket");
  x.supportgetattro();
  x.supportsetattro();
  x.supportstr();
  x.supportcompare();
  x.setBase(Demand::metadata->pythonClass);
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


bool Forecast::callback(Calendar* l, const Signal a)
{
  // This function is called when a calendar is about to be deleted.
  // If that calendar is being used for a forecast we reset the calendar
  // pointer to null.
  for (MapOfForecasts::iterator x = ForecastDictionary.begin();
      x != ForecastDictionary.end(); ++x)
    if (x->second->calptr == l)
      // Calendar in use for this forecast
      x->second->calptr = NULL;
  return true;
}


Forecast::~Forecast()
{
  // Update the dictionary
  for (MapOfForecasts::iterator x=
      ForecastDictionary.lower_bound(make_pair(&*getItem(),&*getCustomer()));
      x != ForecastDictionary.end(); ++x)
    if (x->second == this)
    {
      ForecastDictionary.erase(x);
      break;
    }

  // Delete all children demands
  for(memberIterator i = beginMember(); i != end(); )
  {
    Demand *tmp = &*i;
    ++i;
    delete tmp;
  }
}


void Forecast::instantiate()
{
  if (!calptr) throw DataException("Missing forecast calendar");

  // Create a demand for every bucket. The weight value depends on the
  // calendar type: double, integer, bool or other
  const CalendarDouble* c = dynamic_cast<const CalendarDouble*>(calptr);
  ForecastBucket* prev = NULL;
  Date prevDate;
  double prevValue(0.0);
  if (c)
  {
    // Double calendar
    for (CalendarDouble::EventIterator i(c); i.getDate()<=Date::infiniteFuture; ++i)
    {
      if ((prevDate || i.getDate() == Date::infiniteFuture) && prevValue > 0.0)
      {
        prev = new ForecastBucket(this, prevDate, i.getDate(), prevValue, prev);
        Demand::add(prev);
      }
      if (i.getDate() == Date::infiniteFuture) break;
      prevDate = i.getDate();
      prevValue = i.getValue();
    }
  }
  else
  {
    // Other calendar
    for (Calendar::EventIterator i(calptr); true; ++i)
    {
      if (prevDate || i.getDate() == Date::infiniteFuture)
      {
        prev = new ForecastBucket(this, prevDate, i.getDate(), 1.0, prev);
        Demand::add(prev);
        if (i.getDate() == Date::infiniteFuture) break;
      }
      prevDate = i.getDate();
    }
  }
}


void Forecast::setDiscrete(const bool b)
{
  // Update the flag
  discrete = b;

  // Round down any forecast demands that may already exist.
  if (discrete)
    for (memberIterator m = beginMember(); m!=end(); ++m)
      m->setQuantity(floor(m->getQuantity()));
}


void Forecast::setTotalQuantity(const DateRange& d, double f, bool add)
{
  // Initialize, if not done yet
  if (!isGroup()) instantiate();

  // Find all forecast demands, and sum their weights
  double weights = 0.0;
  for (memberIterator m = beginMember(); m!=end(); ++m)
  {
    ForecastBucket* x = dynamic_cast<ForecastBucket*>(&*m);
    if (!x)
      throw DataException("Invalid subdemand of forecast '" + getName() +"'");
    if (d.intersect(x->getDueRange()))
    {
      // Bucket intersects with daterange
      if (!d.getDuration())
      {
        // Single date provided. Update that one bucket.
        if (add) x->incTotal(f);
        else x->setTotal(f);
        return;
      }
      weights += x->getWeight() * static_cast<long>(x->getDueRange().overlap(d));
    }
  }

  // Expect to find at least one non-zero weight...
  if (!weights)
  {
    ostringstream o;
    o << "No valid forecast date in range " << d
      << " of forecast '" << getName() << "'";
    throw DataException(o.str());
  }

  // Update the forecast quantity, respecting the weights
  f /= weights;
  double carryover = 0.0;
  for (memberIterator m = beginMember(); m!=end(); ++m)
  {
    ForecastBucket* x = dynamic_cast<ForecastBucket*>(&*m);
    if (d.intersect(x->getDueRange()))
    {
      // Bucket intersects with daterange
      TimePeriod o = x->getDueRange().overlap(d);
      double percent = x->getWeight() * static_cast<long>(o);
      if (getDiscrete())
      {
        // Rounding to discrete numbers
        carryover += f * percent;
        int intdelta = static_cast<int>(ceil(carryover - 0.5));
        carryover -= intdelta;
        if (o < x->getDueRange().getDuration() || add)
          // The bucket is only partially updated
          x->incTotal(static_cast<double>(intdelta));
        else
          // The bucket is completely updated
          x->setTotal(static_cast<double>(intdelta));
      }
      else
      {
        // No rounding
        if (o < x->getDueRange().getDuration() || add)
          // The bucket is only partially updated
          x->incTotal(f * percent);
        else
          // The bucket is completely updated
          x->setTotal(f * percent);
      }
    }
  }
}


void Forecast::setTotalQuantity(const Date d, double f, bool add)
{
  // Initialize, if not done yet
  if (!isGroup()) instantiate();

  // Find the bucket
  for (memberIterator m = beginMember(); m!=end(); ++m)
  {
    ForecastBucket* x = dynamic_cast<ForecastBucket*>(&*m);
    if (!x)
      throw DataException("Invalid subdemand of forecast '" + getName() +"'");
    if (x->getDueRange().within(d))
    {
      // Update the bucket
      if (add) x->incTotal(f);
      else x->setTotal(f);
      return;
    }
  }
}


void Forecast::writeElement(Serializer *o, const Keyword &tag, mode m) const
{
  // Writing a reference
  if (m == REFERENCE)
  {
    o->writeElement
    (tag, Tags::tag_name, getName(), Tags::tag_type, getType().type);
    return;
  }

  // Write the complete object
  if (m != NOHEAD) o->BeginObject
    (tag, Tags::tag_name, getName(), Tags::tag_type, getType().type);

  o->writeElement(Tags::tag_item, &*getItem());
  o->writeElement(Tags::tag_operation, &*getOperation());
  if (getPriority()) o->writeElement(Tags::tag_priority, getPriority());
  o->writeElement(Tags::tag_calendar, calptr);
  if (getMaxLateness() != TimePeriod::MAX)
    o->writeElement(Tags::tag_maxlateness, getMaxLateness());
  if (getMinShipment() != 1.0)
    o->writeElement(Tags::tag_minshipment, getMinShipment());

  // Write source field
  o->writeElement(Tags::tag_source, getSource());

  // Write the custom fields
  PythonDictionary::write(o, getDict());

  // Write fields specific to the forecast model
  if (!getDiscrete())
    o->writeElement(Tags::tag_discrete, false);
  if (getMethods() != METHOD_ALL)
    o->writeElement(Forecast::tag_methods, getMethods());
  if (!getPlanned())
    o->writeElement(Forecast::tag_planned, false);

  // Write all entries
  o->BeginList(Tags::tag_buckets);
  for (memberIterator i = beginMember(); i != end(); ++i)
  {
    ForecastBucket* f = dynamic_cast<ForecastBucket*>(&*i);
    o->BeginObject(Tags::tag_bucket, Tags::tag_start, f->getDue());
    o->writeElement(tag_total, f->getTotal());
    o->writeElement(Tags::tag_quantity, f->getQuantity());
    o->writeElement(tag_consumed, f->getConsumed());
    o->EndObject(Tags::tag_bucket);
  }
  o->EndList(Tags::tag_buckets);

  o->EndObject(tag);
}


void Forecast::endElement(DataInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  // While reading forecast buckets, we use the userarea field on the input
  // to cache the data. The temporary object is deleted when the bucket
  // tag is closed.
  if (pAttr.isA(Tags::tag_calendar))
  {
    Calendar *b = dynamic_cast<Calendar*>(pIn.getPreviousObject());
    if (b) setCalendar(b);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA(Tags::tag_discrete))
    setDiscrete(pElement.getBool());
  else if (pAttr.isA(Forecast::tag_planned))
    setPlanned(pElement.getBool());
  else if (pAttr.isA(Forecast::tag_methods))
    setMethods(pElement.getUnsignedLong());
  else if (pAttr.isA(Tags::tag_bucket))
  {
    pair<DateRange,double> *d =
      static_cast< pair<DateRange,double>* >(pIn.getUserArea());
    if (d)
    {
      // Update the forecast quantities
      setTotalQuantity(d->first, d->second);
      // Clear the read buffer
      d->first.setStart(Date());
      d->first.setEnd(Date());
      d->second = 0;
    }
  }
  else if (pIn.getParentElement().isA(Tags::tag_bucket))
  {
    pair<DateRange,double> *d =
      static_cast< pair<DateRange,double>* >(pIn.getUserArea());
    if (pAttr.isA(tag_total))
    {
      if (d) d->second = pElement.getDouble();
      else pIn.setUserArea(
          new pair<DateRange,double>(DateRange(),pElement.getDouble())
        );
    }
    else if (pAttr.isA(Tags::tag_start))
    {
      Date x = pElement.getDate();
      if (d)
      {
        if (!d->first.getStart()) d->first.setStartAndEnd(x,x);
        else d->first.setStart(x);
      }
      else pIn.setUserArea(new pair<DateRange,double>(DateRange(x,x),0));
    }
    else if (pAttr.isA(Tags::tag_end))
    {
      Date x = pElement.getDate();
      if (d)
      {
        if (!d->first.getStart()) d->first.setStartAndEnd(x,x);
        else d->first.setEnd(x);
      }
      else pIn.setUserArea(new pair<DateRange,double>(DateRange(x,x),0));
    }
  }
  else
    Demand::endElement(pIn, pAttr, pElement);

  if (pIn.isObjectEnd())
  {
    // Delete dynamically allocated temporary read object
    if (pIn.getUserArea())
      delete static_cast< pair<DateRange,double>* >(pIn.getUserArea());
  }
}


void Forecast::beginElement(DataInput& pIn, const Attribute& pAttr)
{
  if (pAttr.isA(Tags::tag_calendar))
    pIn.readto( Calendar::reader(Calendar::metadata, pIn.getAttributes()) );
  else
    Demand::beginElement(pIn, pAttr);
}


void Forecast::setCalendar(Calendar* c)
{
  if (isGroup())
    throw DataException(
      "Changing the calendar of an initialized forecast isn't allowed");
  calptr = c;
}


void Forecast::setItem(Item* i)
{
  // No change
  if (getItem() == i) return;

  // Update the dictionary
  for (MapOfForecasts::iterator x =
      ForecastDictionary.lower_bound(make_pair(
          &*getItem(),&*getCustomer()
          ));
      x != ForecastDictionary.end(); ++x)
    if (x->second == this)
    {
      ForecastDictionary.erase(x);
      break;
    }
  ForecastDictionary.insert(make_pair(make_pair(i,&*getCustomer()),this));

  // Update data field
  Demand::setItem(i);

  // Update the item for all buckets/subdemands
  for (memberIterator m = beginMember(); m!=end(); ++m)
    m->setItem(i);
}


void Forecast::setCustomer(Customer* i)
{
  // No change
  if (getCustomer() == i) return;

  // Update the dictionary
  for (MapOfForecasts::iterator x =
      ForecastDictionary.lower_bound(make_pair(
          getItem(), getCustomer()
          ));
      x != ForecastDictionary.end(); ++x)
    if (x->second == this)
    {
      ForecastDictionary.erase(x);
      break;
    }
  ForecastDictionary.insert(make_pair(make_pair(&*getItem(),i),this));

  // Update data field
  Demand::setCustomer(i);

  // Update the customer for all buckets/subdemands
  for (memberIterator m = beginMember(); m!=end(); ++m)
    m->setCustomer(i);
}


void Forecast::setMaxLateness(TimePeriod i)
{
  Demand::setMaxLateness(i);
  // Update the maximum lateness for all buckets/subdemands
  for (memberIterator m = beginMember(); m!=end(); ++m)
    m->setMaxLateness(i);
}


void Forecast::setMinShipment(double i)
{
  Demand::setMinShipment(i);
  // Update the minimum shipment for all buckets/subdemands
  for (memberIterator m = beginMember(); m!=end(); ++m)
    m->setMinShipment(i);
}


void Forecast::setPriority(int i)
{
  Demand::setPriority(i);
  // Update the priority for all buckets/subdemands
  for (memberIterator m = beginMember(); m!=end(); ++m)
    m->setPriority(i);
}


void Forecast::setOperation(Operation *o)
{
  Demand::setOperation(o);
  // Update the priority for all buckets/subdemands
  for (memberIterator m = beginMember(); m!=end(); ++m)
    m->setOperation(o);
}

}       // end namespace
