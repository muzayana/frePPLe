/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba                 *
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
#include <math.h>

// This is the name used for the dummy operation used to represent the
// inventory.
#define INVENTORY_OPERATION "Inventory of buffer '" + string(getName()) + "'"

// This is the name used for the dummy operation used to represent procurements
#define PROCURE_OPERATION "Procure for buffer '" + string(getName()) + "'"

namespace frepple
{

template<class Buffer> DECLARE_EXPORT Tree utils::HasName<Buffer>::st;
DECLARE_EXPORT const MetaCategory* Buffer::metadata;
DECLARE_EXPORT const MetaClass* BufferDefault::metadata,
               *BufferInfinite::metadata,
               *BufferProcure::metadata;
DECLARE_EXPORT const double Buffer::default_max = 1e37;


int Buffer::initialize()
{
  // Initialize the metadata
  metadata = new MetaCategory("buffer", "buffers", reader, writer);

  // Initialize the Python class
  return FreppleCategory<Buffer>::initialize();
}


int BufferDefault::initialize()
{
  // Initialize the metadata
  BufferDefault::metadata = new MetaClass(
    "buffer",
    "buffer_default",
    Object::createString<BufferDefault>, true);

  // Initialize the Python class
  return FreppleClass<BufferDefault,Buffer>::initialize();
}


int BufferInfinite::initialize()
{
  // Initialize the metadata
  BufferInfinite::metadata = new MetaClass(
    "buffer",
    "buffer_infinite",
    Object::createString<BufferInfinite>);

  // Initialize the Python class
  return FreppleClass<BufferInfinite,Buffer>::initialize();
}


int BufferProcure::initialize()
{
  // Initialize the metadata
  BufferProcure::metadata = new MetaClass(
    "buffer",
    "buffer_procure",
    Object::createString<BufferProcure>);

  // Initialize the Python class
  return FreppleClass<BufferProcure,Buffer>::initialize();
}


DECLARE_EXPORT void Buffer::setOnHand(double f)
{
  // The dummy operation to model the inventory may need to be created
  Operation *o = Operation::find(INVENTORY_OPERATION);
  Flow *fl;
  if (!o)
  {
    // Create a fixed time operation with zero leadtime, hidden from the xml
    // output, hidden for the solver, and without problem detection.
    o = new OperationFixedTime(INVENTORY_OPERATION);
    Operation::add(o);  // No need to check again for existance
    o->setHidden(true);
    o->setDetectProblems(false);
    fl = new FlowEnd(o, this, 1);
  }
  else
    // Find the flow of this operation
    fl = const_cast<Flow*>(&*(o->getFlows().begin()));

  // Check valid pointers
  if (!fl || !o)
    throw LogicException("Failed creating inventory operation for '"
        + getName() + "'");

  // Make sure the sign of the flow is correct: +1 or -1.
  fl->setQuantity(f>=0.0 ? 1.0 : -1.0);

  // Create a dummy operationplan on the inventory operation
  OperationPlan::iterator i(o);
  if (i == OperationPlan::end())
  {
    // No operationplan exists yet
    OperationPlan *opplan = o->createOperationPlan(
        fabs(f), Date::infinitePast, Date::infinitePast);
    opplan->setLocked(true);
    // Note that we use the max counter for the onhand operationplans.
    opplan->activate(false);
  }
  else
  {
    // Update the existing operationplan
    i->setLocked(false);
    i->setQuantity(fabs(f));
    i->setLocked(true);
  }
  setChanged();
}


DECLARE_EXPORT double Buffer::getOnHand(Date d) const
{
  double tmp(0.0);
  for (flowplanlist::const_iterator oo=flowplans.begin();
      oo!=flowplans.end(); ++oo)
  {
    if (oo->getDate() > d)
      // Found a flowplan with a later date.
      // Return the onhand after the previous flowplan.
      return tmp;
    tmp = oo->getOnhand();
  }
  // Found no flowplan: either we have specified a date later than the
  // last flowplan, either there are no flowplans at all.
  return tmp;
}


DECLARE_EXPORT double Buffer::getOnHand(Date d1, Date d2, bool min) const
{
  // Swap parameters if required
  if (d2 < d1)
  {
    Date x(d1);
    d2 = d1;
    d2 = x;
  }

  // Loop through all flowplans
  double tmp(0.0), record(0.0);
  Date d, prev_Date;
  for (flowplanlist::const_iterator oo=flowplans.begin(); true; ++oo)
  {
    if (oo==flowplans.end() || oo->getDate() > d)
    {
      // Date has now changed or we have arrived at the end

      // New max?
      if (prev_Date < d1)
        // Not in active Date range: we simply follow the onhand profile
        record = tmp;
      else
      {
        // In the active range
        // New extreme?
        if (min) {if (tmp < record) record = tmp;}
        else {if (tmp > record) record = tmp;}
      }

      // Are we done now?
      if (prev_Date > d2 || oo==flowplans.end()) return record;

      // Set the variable with the new Date
      d = oo->getDate();
    }
    tmp = oo->getOnhand();
    prev_Date = oo->getDate();
  }
  // The above for-loop controls the exit. This line of code is never reached.
  throw LogicException("Unreachable code reached");
}


DECLARE_EXPORT void Buffer::writeElement(XMLOutput *o, const Keyword &tag, mode m) const
{
  // Writing a reference
  if (m == REFERENCE)
  {
    o->writeElement(tag, Tags::tag_name, getName());
    return;
  }

  // Write the complete object
  if (m != NOHEAD && m != NOHEADTAIL) 
    o->BeginObject(tag, Tags::tag_name, XMLEscape(getName()));

  // Write own fields
  HasDescription::writeElement(o, tag);
  HasHierarchy<Buffer>::writeElement(o, tag);
  o->writeElement(Tags::tag_producing, producing_operation);
  o->writeElement(Tags::tag_item, it);
  o->writeElement(Tags::tag_location, loc);
  Plannable::writeElement(o, tag);

  // Onhand
  flowplanlist::const_iterator i = flowplans.begin();
  // Loop through the flowplans at the start of the horizon
  for (; i!=flowplans.end() && i->getType()!=1 && !i->getDate(); ++i) ;
  if (i!=flowplans.end() && i->getType()==1)
  {
    // A flowplan has been found
    const FlowPlan *fp = dynamic_cast<const FlowPlan*>(&*i);
    if (fp
        && fp->getFlow()->getOperation()->getName() == string(INVENTORY_OPERATION)
        && fabs(fp->getQuantity()) > ROUNDING_ERROR)
      o->writeElement(Tags::tag_onhand, fp->getQuantity());
  }

  // Minimum and maximum inventory targets, carrying cost
  if (min_val != 0) o->writeElement(Tags::tag_minimum, min_val);
  o->writeElement(Tags::tag_minimum_calendar, min_cal);
  if (max_val != default_max) o->writeElement(Tags::tag_maximum, max_val);
  o->writeElement(Tags::tag_maximum_calendar, max_cal);
  if (getCarryingCost()!= 0.0)
    o->writeElement(Tags::tag_carrying_cost, getCarryingCost());

  // Write extra plan information
  i = flowplans.begin();
  if ((o->getContentType() == XMLOutput::PLAN
      || o->getContentType() == XMLOutput::PLANDETAIL) && i!=flowplans.end())
  {
    o->BeginObject(Tags::tag_flowplans);
    for (; i!=flowplans.end(); ++i)
      if (i->getType()==1)
        dynamic_cast<const FlowPlan*>(&*i)->writeElement(o, Tags::tag_flowplan);
    o->EndObject(Tags::tag_flowplans);
  }

  // Ending tag
  if (m != NOHEADTAIL && m != NOTAIL) o->EndObject(tag);
}


DECLARE_EXPORT void Buffer::beginElement(XMLInput& pIn, const Attribute& pAttr)
{
  if (pAttr.isA(Tags::tag_flow)
      && pIn.getParentElement().first.isA(Tags::tag_flows))
  {
    Flow *f =
      dynamic_cast<Flow*>(MetaCategory::ControllerDefault(Flow::metadata,pIn.getAttributes()));
    if (f) f->setBuffer(this);
    pIn.readto (f);
  }
  else if (pAttr.isA(Tags::tag_producing))
    pIn.readto( Operation::reader(Operation::metadata,pIn.getAttributes()) );
  else if (pAttr.isA(Tags::tag_item))
    pIn.readto( Item::reader(Item::metadata,pIn.getAttributes()) );
  else if (pAttr.isA(Tags::tag_minimum_calendar)
      || pAttr.isA(Tags::tag_maximum_calendar))
    pIn.readto( Calendar::reader(Calendar::metadata,pIn.getAttributes()) );
  else if (pAttr.isA(Tags::tag_location))
    pIn.readto( Location::reader(Location::metadata,pIn.getAttributes()) );
  else if (pAttr.isA(Tags::tag_flowplans))
    pIn.IgnoreElement();
  else
    HasHierarchy<Buffer>::beginElement(pIn, pAttr);
}


DECLARE_EXPORT void Buffer::endElement(XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  if (pAttr.isA(Tags::tag_producing))
  {
    Operation *b = dynamic_cast<Operation*>(pIn.getPreviousObject());
    if (b) setProducingOperation(b);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA(Tags::tag_item))
  {
    Item *a = dynamic_cast<Item*>(pIn.getPreviousObject());
    if (a) setItem(a);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA(Tags::tag_onhand))
    setOnHand(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_minimum))
    setMinimum(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_maximum))
    setMaximum(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_minimum_calendar))
  {
    CalendarDouble *mincal =
      dynamic_cast<CalendarDouble*>(pIn.getPreviousObject());
    if (mincal)
      setMinimumCalendar(mincal);
    else
    {
      Calendar *c = dynamic_cast<Calendar*>(pIn.getPreviousObject());
      if (!c)
        throw LogicException("Incorrect object type during read operation");
      throw DataException("Calendar '" + c->getName() +
          "' has invalid type for use as buffer min calendar");
    }
  }
  else if (pAttr.isA(Tags::tag_maximum_calendar))
  {
    CalendarDouble *maxcal =
      dynamic_cast<CalendarDouble*>(pIn.getPreviousObject());
    if (maxcal)
      setMaximumCalendar(maxcal);
    else
    {
      Calendar *c = dynamic_cast<Calendar*>(pIn.getPreviousObject());
      if (!c)
        throw LogicException("Incorrect object type during read operation");
      throw DataException("Calendar '" + c->getName() +
          "' has invalid type for use as buffer max calendar");
    }
  }
  else if (pAttr.isA(Tags::tag_location))
  {
    Location * d = dynamic_cast<Location*>(pIn.getPreviousObject());
    if (d) setLocation(d);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA(Tags::tag_carrying_cost))
    setCarryingCost(pElement.getDouble());
  else
  {
    Plannable::endElement(pIn, pAttr, pElement);
    HasDescription::endElement(pIn, pAttr, pElement);
    HasHierarchy<Buffer>::endElement(pIn, pAttr, pElement);
  }
}


DECLARE_EXPORT void Buffer::setMinimum(double m)
{
  // There is already a minimum calendar.
  if (min_cal)
  {
    // We update the field, but don't use it yet.
    min_val = m;
    return;
  }

  // Mark as changed
  setChanged();

  // Set field
  min_val = m;

  // Create or update a single timeline min event
  for (flowplanlist::iterator oo=flowplans.begin(); oo!=flowplans.end(); oo++)
    if (oo->getType() == 3)
    {
      // Update existing event
      static_cast<flowplanlist::EventMinQuantity *>(&*oo)->setMin(min_val);
      return;
    }
  // Create new event
  flowplanlist::EventMinQuantity *newEvent =
    new flowplanlist::EventMinQuantity(Date::infinitePast, min_val);
  flowplans.insert(newEvent);
}


DECLARE_EXPORT void Buffer::setMinimumCalendar(CalendarDouble *cal)
{
  // Resetting the same calendar
  if (min_cal == cal) return;

  // Mark as changed
  setChanged();

  // Delete previous events.
  for (flowplanlist::iterator oo=flowplans.begin(); oo!=flowplans.end(); )
    if (oo->getType() == 3)
    {
      flowplans.erase(&(*oo));
      delete &(*(oo++));
    }
    else ++oo;

  // Null pointer passed. Change back to time independent min.
  if (!cal)
  {
    setMinimum(min_val);
    return;
  }

  // Create timeline structures for every event. A new entry is created only
  // when the value changes.
  min_cal = const_cast< CalendarDouble* >(cal);
  double curMin = 0.0;
  for (CalendarDouble::EventIterator x(min_cal); x.getDate()<Date::infiniteFuture; ++x)
    if (curMin != x.getValue())
    {
      curMin = x.getValue();
      flowplanlist::EventMinQuantity *newBucket =
        new flowplanlist::EventMinQuantity(x.getDate(), curMin);
      flowplans.insert(newBucket);
    }
}


DECLARE_EXPORT void Buffer::setMaximum(double m)
{
  // There is already a maximum calendar.
  if (max_cal)
  {
    // We update the field, but don't use it yet.
    max_val = m;
    return;
  }

  // Mark as changed
  setChanged();

  // Set field
  max_val = m;

  // Create or update a single timeline max event
  for (flowplanlist::iterator oo=flowplans.begin(); oo!=flowplans.end(); oo++)
    if (oo->getType() == 4)
    {
      // Update existing event
      static_cast<flowplanlist::EventMaxQuantity *>(&*oo)->setMax(max_val);
      return;
    }
  // Create new event
  flowplanlist::EventMaxQuantity *newEvent =
    new flowplanlist::EventMaxQuantity(Date::infinitePast, max_val);
  flowplans.insert(newEvent);
}


DECLARE_EXPORT void Buffer::setMaximumCalendar(CalendarDouble *cal)
{
  // Resetting the same calendar
  if (max_cal == cal) return;

  // Mark as changed
  setChanged();

  // Delete previous events.
  for (flowplanlist::iterator oo=flowplans.begin(); oo!=flowplans.end(); )
    if (oo->getType() == 4)
    {
      flowplans.erase(&(*oo));
      delete &(*(oo++));
    }
    else ++oo;

  // Null pointer passed. Change back to time independent max.
  if (!cal)
  {
    setMaximum(max_val);
    return;
  }

  // Create timeline structures for every bucket. A new entry is created only
  // when the value changes.
  max_cal = const_cast<CalendarDouble*>(cal);
  double curMax = 0.0;
  for (CalendarDouble::EventIterator x(max_cal); x.getDate()<Date::infiniteFuture; ++x)
    if (curMax != x.getValue())
    {
      curMax = x.getValue();
      flowplanlist::EventMaxQuantity *newBucket =
        new flowplanlist::EventMaxQuantity(x.getDate(), curMax);
      flowplans.insert(newBucket);
    }
}


DECLARE_EXPORT void Buffer::deleteOperationPlans(bool deleteLocked)
{
  // Delete the operationplans
  for (flowlist::iterator i=flows.begin(); i!=flows.end(); ++i)
    OperationPlan::deleteOperationPlans(i->getOperation(),deleteLocked);

  // Mark to recompute the problems
  setChanged();
}


DECLARE_EXPORT Buffer::~Buffer()
{
  // Delete all operationplans.
  // An alternative logic would be to delete only the flowplans for this
  // buffer and leave the rest of the plan untouched. The currently
  // implemented method is way more drastic...
  deleteOperationPlans(true);

  // The Flow objects are automatically deleted by the destructor of the
  // Association list class.

  // Remove the inventory operation
  Operation *invoper = Operation::find(INVENTORY_OPERATION);
  if (invoper) delete invoper;
}


DECLARE_EXPORT void Buffer::followPegging
(PeggingIterator& iter, FlowPlan* curflowplan, short nextlevel, double curqty, double curfactor)
{

  double peggedQty(0);
  Buffer::flowplanlist::const_iterator f = getFlowPlans().begin(curflowplan);

  if (curflowplan->getQuantity() < -ROUNDING_ERROR && !iter.isDownstream())
  {
    // CASE 1:
    // This is a flowplan consuming from a buffer. Navigating upstream means
    // finding the flowplans producing this consumed material.
    double endQty = f->getCumulativeConsumed();
    double startQty = endQty + f->getQuantity();
    if (f->getCumulativeProduced() <= startQty)
    {
      // CASE 1A: Not produced enough yet: move forward
      while (f!=getFlowPlans().end()
          && f->getCumulativeProduced() <= startQty) ++f;
      while (f!=getFlowPlans().end()
          && ( (f->getQuantity()<=0 && f->getCumulativeProduced() < endQty)
              || (f->getQuantity()>0
                  && f->getCumulativeProduced()-f->getQuantity() < endQty))
            )
      {
        if (f->getQuantity() > ROUNDING_ERROR)
        {
          double newqty = f->getQuantity();
          if (f->getCumulativeProduced()-f->getQuantity() < startQty)
            newqty -= startQty - (f->getCumulativeProduced()-f->getQuantity());
          if (f->getCumulativeProduced() > endQty)
            newqty -= f->getCumulativeProduced() - endQty;
          peggedQty += newqty;
          const FlowPlan *x = dynamic_cast<const FlowPlan*>(&(*f));
          iter.updateStack(nextlevel,
              -curqty*newqty/curflowplan->getQuantity(),
              curfactor*newqty/f->getQuantity(),
              curflowplan, x);
        }
        ++f;
      }
    }
    else
    {
      // CASE 1B: Produced too much already: move backward
      while ( f!=getFlowPlans().end()
          && ((f->getQuantity()<=0 && f->getCumulativeProduced() > endQty)
              || (f->getQuantity()>0
                  && f->getCumulativeProduced()-f->getQuantity() > endQty))) --f;
      while (f!=getFlowPlans().end() && f->getCumulativeProduced() > startQty)
      {
        if (f->getQuantity() > ROUNDING_ERROR)
        {
          double newqty = f->getQuantity();
          if (f->getCumulativeProduced()-f->getQuantity() < startQty)
            newqty -= startQty - (f->getCumulativeProduced()-f->getQuantity());
          if (f->getCumulativeProduced() > endQty)
            newqty -= f->getCumulativeProduced() - endQty;
          peggedQty += newqty;
          const FlowPlan *x = dynamic_cast<const FlowPlan*>(&(*f));
          iter.updateStack(nextlevel,
              -curqty*newqty/curflowplan->getQuantity(),
              curfactor*newqty/f->getQuantity(),
              curflowplan, x);
        }
        --f;
      }
    }
    if (peggedQty < endQty - startQty - ROUNDING_ERROR)
      // Unproduced material (i.e. material that is consumed but never
      // produced) is handled with a special entry on the stack.
      iter.updateStack(nextlevel,
          curqty*(peggedQty - endQty + startQty)/curflowplan->getQuantity(),
          curfactor,
          curflowplan,
          NULL,
          false);
    return;
  }

  if (curflowplan->getQuantity() > ROUNDING_ERROR && iter.isDownstream())
  {
    // CASE 2:
    // This is a flowplan producing in a buffer. Navigating downstream means
    // finding the flowplans consuming this produced material.
    double endQty = f->getCumulativeProduced();
    double startQty = endQty - f->getQuantity();
    if (f->getCumulativeConsumed() <= startQty)
    {
      // CASE 2A: Not consumed enough yet: move forward
      while (f!=getFlowPlans().end()
          && f->getCumulativeConsumed() <= startQty) ++f;
      while (f!=getFlowPlans().end()
          && ( (f->getQuantity()<=0
              && f->getCumulativeConsumed()+f->getQuantity() < endQty)
              || (f->getQuantity()>0 && f->getCumulativeConsumed() < endQty))
            )
      {
        if (f->getQuantity() < -ROUNDING_ERROR)
        {
          double newqty = - f->getQuantity();
          if (f->getCumulativeConsumed()+f->getQuantity() < startQty)
            newqty -= startQty - (f->getCumulativeConsumed()+f->getQuantity());
          if (f->getCumulativeConsumed() > endQty)
            newqty -= f->getCumulativeConsumed() - endQty;
          peggedQty += newqty;
          const FlowPlan *x = dynamic_cast<const FlowPlan*>(&(*f));
          iter.updateStack(nextlevel,
              curqty*newqty/curflowplan->getQuantity(),
              -curfactor*newqty/f->getQuantity(),
              x, curflowplan);
        }
        ++f;
      }
    }
    else
    {
      // CASE 2B: Consumed too much already: move backward
      while ( f!=getFlowPlans().end()
          && ((f->getQuantity()<=0 && f->getCumulativeConsumed()+f->getQuantity() < endQty)
              || (f->getQuantity()>0 && f->getCumulativeConsumed() < endQty))) --f;
      while (f!=getFlowPlans().end() && f->getCumulativeConsumed() > startQty)
      {
        if (f->getQuantity() < -ROUNDING_ERROR)
        {
          double newqty = - f->getQuantity();
          if (f->getCumulativeConsumed()+f->getQuantity() < startQty)
            newqty -= startQty - (f->getCumulativeConsumed()+f->getQuantity());
          if (f->getCumulativeConsumed() > endQty)
            newqty -= f->getCumulativeConsumed() - endQty;
          peggedQty += newqty;
          const FlowPlan *x = dynamic_cast<const FlowPlan*>(&(*f));
          iter.updateStack(nextlevel,
              curqty*newqty/curflowplan->getQuantity(),
              -curfactor*newqty/f->getQuantity(),
              x, curflowplan);
        }
        --f;
      }
    }
    if (peggedQty < endQty - startQty)
      // Unpegged material (i.e. material that is produced but never consumed)
      // is handled with a special entry on the stack.
      iter.updateStack(nextlevel,
          curqty*(endQty - startQty - peggedQty)/curflowplan->getQuantity(),
          curfactor,
          NULL, curflowplan,
          false);
    return;
  }
}


DECLARE_EXPORT void BufferInfinite::writeElement
(XMLOutput *o, const Keyword &tag, mode m) const
{
  // Writing a reference
  if (m == REFERENCE)
  {
    o->writeElement
    (tag, Tags::tag_name, getName(), Tags::tag_type, getType().type);
    return;
  }

  // Write the complete object
  if (m != NOHEAD && m != NOHEADTAIL) o->BeginObject
    (tag, Tags::tag_name, XMLEscape(getName()), Tags::tag_type, getType().type);

  // Write the fields and an ending tag
  Buffer::writeElement(o, tag, NOHEAD);
}


DECLARE_EXPORT void BufferProcure::endElement(XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  if (pAttr.isA(Tags::tag_leadtime))
    setLeadtime(pElement.getTimeperiod());
  else if (pAttr.isA(Tags::tag_fence))
    setFence(pElement.getTimeperiod());
  else if (pAttr.isA(Tags::tag_size_maximum))
    setSizeMaximum(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_size_minimum))
    setSizeMinimum(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_size_multiple))
    setSizeMultiple(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_mininterval))
    setMinimumInterval(pElement.getTimeperiod());
  else if (pAttr.isA(Tags::tag_maxinterval))
    setMaximumInterval(pElement.getTimeperiod());
  else if (pAttr.isA(Tags::tag_mininventory))
    setMinimumInventory(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_maxinventory))
    setMaximumInventory(pElement.getDouble());
  else
    Buffer::endElement(pIn, pAttr, pElement);
}


DECLARE_EXPORT void BufferProcure::writeElement(XMLOutput *o, const Keyword &tag, mode m) const
{
  // Writing a reference
  if (m == REFERENCE)
  {
    o->writeElement
    (tag, Tags::tag_name, getName(), Tags::tag_type, getType().type);
    return;
  }

  // Write the complete object
  if (m != NOHEAD && m != NOHEADTAIL) o->BeginObject
    (tag, Tags::tag_name, XMLEscape(getName()), Tags::tag_type, getType().type);

  // Write the standard buffer fields
  Buffer::writeElement(o, tag, NOHEADTAIL);

  // Write the extra fields
  if (leadtime) o->writeElement(Tags::tag_leadtime, leadtime);
  if (fence) o->writeElement(Tags::tag_fence, fence);
  if (size_maximum != DBL_MAX) o->writeElement(Tags::tag_size_maximum, size_maximum);
  if (size_minimum) o->writeElement(Tags::tag_size_minimum, size_minimum);
  if (size_multiple) o->writeElement(Tags::tag_size_multiple, size_multiple);
  if (min_interval) o->writeElement(Tags::tag_mininterval, min_interval);
  if (max_interval) o->writeElement(Tags::tag_maxinterval, max_interval);
  if (getMinimumInventory()) o->writeElement(Tags::tag_mininventory, getMinimumInventory());
  if (getMaximumInventory()) o->writeElement(Tags::tag_maxinventory, getMaximumInventory());

  // Write the tail
  if (m != NOHEADTAIL && m != NOTAIL) o->EndObject(tag);
}


DECLARE_EXPORT Operation* BufferProcure::getOperation() const
{
  if (!oper)
  {
    Operation *o = Operation::find(PROCURE_OPERATION);
    if (!o)
    {
      // Create the operation if it didn't exist yet
      o = new OperationFixedTime(PROCURE_OPERATION);
      static_cast<OperationFixedTime*>(o)->setDuration(leadtime);
      o->setFence(getFence());
      o->setSizeMaximum(getSizeMaximum());
      o->setSizeMinimum(getSizeMinimum());
      o->setSizeMultiple(getSizeMultiple());
      Operation::add(o);  // No need to check again for existence
      new FlowEnd(o, const_cast<BufferProcure*>(this), 1);
    }
    const_cast<BufferProcure*>(this)->oper = o;
  }
  return oper;
}


DECLARE_EXPORT PyObject* Buffer::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_name))
    return PythonObject(getName());
  if (attr.isA(Tags::tag_description))
    return PythonObject(getDescription());
  if (attr.isA(Tags::tag_category))
    return PythonObject(getCategory());
  if (attr.isA(Tags::tag_subcategory))
    return PythonObject(getSubCategory());
  if (attr.isA(Tags::tag_owner))
    return PythonObject(getOwner());
  if (attr.isA(Tags::tag_location))
    return PythonObject(getLocation());
  if (attr.isA(Tags::tag_producing))
    return PythonObject(getProducingOperation());
  if (attr.isA(Tags::tag_item))
    return PythonObject(getItem());
  if (attr.isA(Tags::tag_onhand))
    return PythonObject(getOnHand());
  if (attr.isA(Tags::tag_flowplans))
    return new FlowPlanIterator(this);
  if (attr.isA(Tags::tag_maximum))
    return PythonObject(getMaximum());
  if (attr.isA(Tags::tag_minimum))
    return PythonObject(getMinimum());
  if (attr.isA(Tags::tag_maximum_calendar))
    return PythonObject(getMaximumCalendar());
  if (attr.isA(Tags::tag_minimum_calendar))
    return PythonObject(getMinimumCalendar());
  if (attr.isA(Tags::tag_carrying_cost))
    return PythonObject(getCarryingCost());
  if (attr.isA(Tags::tag_hidden))
    return PythonObject(getHidden());
  if (attr.isA(Tags::tag_flows))
    return new FlowIterator(this);
  if (attr.isA(Tags::tag_level))
    return PythonObject(getLevel());
  if (attr.isA(Tags::tag_cluster))
    return PythonObject(getCluster());
  if (attr.isA(Tags::tag_members))
    return new BufferIterator(this);
  return NULL;
}


DECLARE_EXPORT int Buffer::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_name))
    setName(field.getString());
  else if (attr.isA(Tags::tag_description))
    setDescription(field.getString());
  else if (attr.isA(Tags::tag_category))
    setCategory(field.getString());
  else if (attr.isA(Tags::tag_subcategory))
    setSubCategory(field.getString());
  else if (attr.isA(Tags::tag_owner))
  {
    if (!field.check(Buffer::metadata))
    {
      PyErr_SetString(PythonDataException, "buffer owner must be of type buffer");
      return -1;
    }
    Buffer* y = static_cast<Buffer*>(static_cast<PyObject*>(field));
    setOwner(y);
  }
  else if (attr.isA(Tags::tag_location))
  {
    if (!field.check(Location::metadata))
    {
      PyErr_SetString(PythonDataException, "buffer location must be of type location");
      return -1;
    }
    Location* y = static_cast<Location*>(static_cast<PyObject*>(field));
    setLocation(y);
  }
  else if (attr.isA(Tags::tag_item))
  {
    if (!field.check(Item::metadata))
    {
      PyErr_SetString(PythonDataException, "buffer item must be of type item");
      return -1;
    }
    Item* y = static_cast<Item*>(static_cast<PyObject*>(field));
    setItem(y);
  }
  else if (attr.isA(Tags::tag_minimum))
    setMinimum(field.getDouble());
  else if (attr.isA(Tags::tag_maximum))
    setMaximum(field.getDouble());
  else if (attr.isA(Tags::tag_maximum_calendar))
  {
    if (!field.check(CalendarDouble::metadata))
    {
      PyErr_SetString(PythonDataException, "buffer maximum must be of type calendar_double");
      return -1;
    }
    CalendarDouble* y = static_cast<CalendarDouble*>(static_cast<PyObject*>(field));
    setMaximumCalendar(y);
  }
  else if (attr.isA(Tags::tag_minimum_calendar))
  {
    if (!field.check(CalendarDouble::metadata))
    {
      PyErr_SetString(PythonDataException, "buffer minimum must be of type calendar_double");
      return -1;
    }
    CalendarDouble* y = static_cast<CalendarDouble*>(static_cast<PyObject*>(field));
    setMinimumCalendar(y);
  }
  else if (attr.isA(Tags::tag_onhand))
    setOnHand(field.getDouble());
  else if (attr.isA(Tags::tag_carrying_cost))
    setCarryingCost(field.getDouble());
  else if (attr.isA(Tags::tag_producing))
  {
    if (!field.check(Operation::metadata))
    {
      PyErr_SetString(PythonDataException, "buffer producing must be of type operation");
      return -1;
    }
    Operation* y = static_cast<Operation*>(static_cast<PyObject*>(field));
    setProducingOperation(y);
  }
  else if (attr.isA(Tags::tag_hidden))
    setHidden(field.getBool());
  else
    return -1;  // Error
  return 0;  // OK
}


DECLARE_EXPORT PyObject* BufferProcure::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_leadtime))
    return PythonObject(getLeadtime());
  if (attr.isA(Tags::tag_mininventory))
    return PythonObject(getMinimumInventory());
  if (attr.isA(Tags::tag_maxinventory))
    return PythonObject(getMaximumInventory());
  if (attr.isA(Tags::tag_mininterval))
    return PythonObject(getMinimumInterval());
  if (attr.isA(Tags::tag_maxinterval))
    return PythonObject(getMaximumInterval());
  if (attr.isA(Tags::tag_fence))
    return PythonObject(getFence());
  if (attr.isA(Tags::tag_size_minimum))
    return PythonObject(getSizeMinimum());
  if (attr.isA(Tags::tag_size_multiple))
    return PythonObject(getSizeMultiple());
  if (attr.isA(Tags::tag_size_maximum))
    return PythonObject(getSizeMaximum());
  if (attr.isA(Tags::tag_producing))
    return PythonObject(getOperation());
  return Buffer::getattro(attr);
}


DECLARE_EXPORT int BufferProcure::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_leadtime))
    setLeadtime(field.getTimeperiod());
  else if (attr.isA(Tags::tag_mininventory))
    setMinimumInventory(field.getDouble());
  else if (attr.isA(Tags::tag_maxinventory))
    setMaximumInventory(field.getDouble());
  else if (attr.isA(Tags::tag_mininterval))
    setMinimumInterval(field.getTimeperiod());
  else if (attr.isA(Tags::tag_maxinterval))
    setMaximumInterval(field.getTimeperiod());
  else if (attr.isA(Tags::tag_size_minimum))
    setSizeMinimum(field.getDouble());
  else if (attr.isA(Tags::tag_size_multiple))
    setSizeMultiple(field.getDouble());
  else if (attr.isA(Tags::tag_size_maximum))
    setSizeMaximum(field.getDouble());
  else if (attr.isA(Tags::tag_fence))
    setFence(field.getTimeperiod());
  else if (attr.isA(Tags::tag_producing))
    return 0;
  else
    return Buffer::setattro(attr, field);
  return 0;
}

} // end namespace
