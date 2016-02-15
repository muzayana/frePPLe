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
#include <math.h>

// This is the name used for the dummy operation used to represent the
// inventory.
#define INVENTORY_OPERATION "Inventory " + string(getName())

// This is the name used for the dummy operation used to represent procurements
#define PURCHASE_OPERATION "Purchase " + string(getName())

namespace frepple
{

template<class Buffer> DECLARE_EXPORT Tree utils::HasName<Buffer>::st;
DECLARE_EXPORT const MetaCategory* Buffer::metadata;
DECLARE_EXPORT const MetaClass* BufferDefault::metadata,
               *BufferInfinite::metadata,
               *BufferProcure::metadata;
DECLARE_EXPORT const double Buffer::default_max = 1e37;
DECLARE_EXPORT OperationFixedTime *Buffer::uninitializedProducing = NULL;


int Buffer::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<Buffer>("buffer", "buffers", reader, finder);
  registerFields<Buffer>(const_cast<MetaCategory*>(metadata));

  uninitializedProducing = new OperationFixedTime();

  // Initialize the Python class
  return FreppleCategory<Buffer>::initialize();
}


int BufferDefault::initialize()
{
  // Initialize the metadata
  BufferDefault::metadata = MetaClass::registerClass<BufferDefault>(
    "buffer",
    "buffer_default",
    Object::create<BufferDefault>, true);

  // Initialize the Python class
  return FreppleClass<BufferDefault,Buffer>::initialize();
}


int BufferInfinite::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<BufferInfinite>(
    "buffer",
    "buffer_infinite",
    Object::create<BufferInfinite>);

  // Initialize the Python class
  return FreppleClass<BufferInfinite,Buffer>::initialize();
}


int BufferProcure::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<BufferProcure>(
    "buffer",
    "buffer_procure",
    Object::create<BufferProcure>);
  registerFields<BufferProcure>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  return FreppleClass<BufferProcure,Buffer>::initialize();
}


DECLARE_EXPORT void Buffer::inspect(const string msg) const
{
  logger << "Inspecting buffer " << getName() << ": ";
  if (!msg.empty()) logger  << msg;
  logger << endl;

  double curmin = 0.0;
  for (flowplanlist::const_iterator oo = getFlowPlans().begin();
    oo != getFlowPlans().end();
    ++oo)
  {
    if (oo->getEventType() == 3)
      curmin = oo->getMin();
    logger << "  " << oo->getDate()
      << " qty:" << oo->getQuantity()
      << ", oh:" << oo->getOnhand()
      << ", min:" << curmin;
    if (oo->getEventType() == 1)
      logger <<  ", oper:" << static_cast<const FlowPlan*>(&*oo)->getOperationPlan()->getOperation();
    logger << endl;
  }
}


DECLARE_EXPORT void Buffer::setItem(Item* i)
{
  if (it == i)
    // No change
    return;

  // Unlink from previous item
  if (it)
  {
    if (it->firstItemBuffer == this)
      it->firstItemBuffer = nextItemBuffer;
    else
    {
      Buffer* buf = it->firstItemBuffer;
      while (buf && buf->nextItemBuffer != this)
        buf = buf->nextItemBuffer;
      if (!buf)
        throw LogicException("corrupted buffer list for an item");
      buf->nextItemBuffer = nextItemBuffer;
    }
  }

  // Link at new item
  it = i;
  if (it)
  {
    nextItemBuffer = it->firstItemBuffer;
    it->firstItemBuffer = this;
  }

  // Mark changed
  setChanged();
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
    o = new OperationFixedTime();
    o->setName(INVENTORY_OPERATION);
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
    opplan->setConfirmed(true);
    opplan->activate();
  }
  else
  {
    // Update the existing operationplan
    i->setConfirmed(false);
    i->setQuantity(fabs(f));
    i->setConfirmed(true);
  }
  setChanged();
}


DECLARE_EXPORT double Buffer::getOnHand() const
{
  for (flowplanlist::const_iterator i = flowplans.begin(); i!=flowplans.end(); ++i)
  {
    if(i->getDate()) return 0.0; // Inventory event is always at start of horizon
    if(i->getEventType() != 1) continue;
    const FlowPlan *fp = static_cast<const FlowPlan*>(&*i);
    if (fp->getFlow()->getOperation()->getName() == string(INVENTORY_OPERATION)
      && fabs(fp->getQuantity()) > ROUNDING_ERROR)
        return fp->getQuantity();
  }
  return 0.0;
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
    if (oo->getEventType() == 3)
    {
      // Update existing event
      static_cast<flowplanlist::EventMinQuantity *>(&*oo)->setMin(min_val);
      return;
    }
  // Create new event
  flowplanlist::EventMinQuantity *newEvent =
    new flowplanlist::EventMinQuantity(Date::infinitePast, &flowplans, min_val);
  flowplans.insert(newEvent);
}


DECLARE_EXPORT void Buffer::setMinimumCalendar(Calendar *cal)
{
  // Resetting the same calendar
  if (min_cal == cal) return;

  // Mark as changed
  setChanged();

  // Delete previous events.
  for (flowplanlist::iterator oo=flowplans.begin(); oo!=flowplans.end(); )
    if (oo->getEventType() == 3)
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
  min_cal = cal;
  double curMin = 0.0;
  for (Calendar::EventIterator x(min_cal); x.getDate()<Date::infiniteFuture; ++x)
    if (curMin != x.getValue())
    {
      curMin = x.getValue();
      flowplanlist::EventMinQuantity *newBucket =
        new flowplanlist::EventMinQuantity(x.getDate(), &flowplans, curMin);
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
    if (oo->getEventType() == 4)
    {
      // Update existing event
      static_cast<flowplanlist::EventMaxQuantity *>(&*oo)->setMax(max_val);
      return;
    }
  // Create new event
  flowplanlist::EventMaxQuantity *newEvent =
    new flowplanlist::EventMaxQuantity(Date::infinitePast, &flowplans, max_val);
  flowplans.insert(newEvent);
}


DECLARE_EXPORT void Buffer::setMaximumCalendar(Calendar *cal)
{
  // Resetting the same calendar
  if (max_cal == cal) return;

  // Mark as changed
  setChanged();

  // Delete previous events.
  for (flowplanlist::iterator oo=flowplans.begin(); oo!=flowplans.end(); )
    if (oo->getEventType() == 4)
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
  max_cal = cal;
  double curMax = 0.0;
  for (Calendar::EventIterator x(max_cal); x.getDate()<Date::infiniteFuture; ++x)
    if (curMax != x.getValue())
    {
      curMax = x.getValue();
      flowplanlist::EventMaxQuantity *newBucket =
        new flowplanlist::EventMaxQuantity(x.getDate(), &flowplans, curMax);
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

  // Unlink from the item
  if (it)
  {
    if (it->firstItemBuffer == this)
      it->firstItemBuffer = nextItemBuffer;
    else
    {
      Buffer* buf = it->firstItemBuffer;
      while (buf && buf->nextItemBuffer != this)
        buf = buf->nextItemBuffer;
      if (!buf)
        logger << "Error: Corrupted buffer list for an item";
      buf->nextItemBuffer = nextItemBuffer;
    }
  }

  // Remove the inventory operation
  Operation *invoper = Operation::find(INVENTORY_OPERATION);
  if (invoper) delete invoper;
}


DECLARE_EXPORT void Buffer::followPegging
(PeggingIterator& iter, FlowPlan* curflowplan, double qty, double offset, short lvl)
{
  if (!curflowplan->getOperationPlan()->getQuantity() || curflowplan->getBuffer()->getTool())
    // Flowplans with quantity 0 have no pegging.
    // Flowplans for buffers representing tools have no pegging either.
    return;

  Buffer::flowplanlist::iterator f = getFlowPlans().begin(curflowplan);
  if (curflowplan->getQuantity() < -ROUNDING_ERROR && !iter.isDownstream())
  {
    // CASE 1:
    // This is a flowplan consuming from a buffer. Navigating upstream means
    // finding the flowplans producing this consumed material.
    double scale = - curflowplan->getQuantity() / curflowplan->getOperationPlan()->getQuantity();
    double startQty = f->getCumulativeConsumed() + f->getQuantity() + offset * scale;
    double endQty = startQty + qty * scale;
    if (f->getCumulativeProduced() <= startQty + ROUNDING_ERROR)
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
          double newoffset = 0.0;
          if (f->getCumulativeProduced()-f->getQuantity() < startQty)
          {
            newoffset = startQty - (f->getCumulativeProduced()-f->getQuantity());
            newqty -= newoffset;
          }
          if (f->getCumulativeProduced() > endQty)
            newqty -= f->getCumulativeProduced() - endQty;
          OperationPlan *opplan = dynamic_cast<const FlowPlan*>(&(*f))->getOperationPlan();
          OperationPlan *topopplan = opplan->getTopOwner();
          if (topopplan->getOperation()->getType() == *OperationSplit::metadata)
            topopplan = opplan;
          iter.updateStack(
            topopplan,
            topopplan->getQuantity() * newqty / f->getQuantity(),
            topopplan->getQuantity() * newoffset / f->getQuantity(),
            lvl
            );
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
          double newoffset = 0.0;
          if (f->getCumulativeProduced()-f->getQuantity() < startQty)
          {
            newoffset = startQty - (f->getCumulativeProduced()-f->getQuantity());
            newqty -= newoffset;
          }
          if (f->getCumulativeProduced() > endQty)
            newqty -= f->getCumulativeProduced() - endQty;
          OperationPlan *opplan = dynamic_cast<FlowPlan*>(&(*f))->getOperationPlan();
          OperationPlan *topopplan = opplan->getTopOwner();
          if (topopplan->getOperation()->getType() == *OperationSplit::metadata)
            topopplan = opplan;
          iter.updateStack(
            topopplan,
            topopplan->getQuantity() * newqty / f->getQuantity(),
            topopplan->getQuantity() * newoffset / f->getQuantity(),
            lvl
            );
        }
        --f;
      }
    }
    return;
  }

  if (curflowplan->getQuantity() > ROUNDING_ERROR && iter.isDownstream())
  {
    // CASE 2:
    // This is a flowplan producing in a buffer. Navigating downstream means
    // finding the flowplans consuming this produced material.
    double scale = curflowplan->getQuantity() / curflowplan->getOperationPlan()->getQuantity();
    double startQty = f->getCumulativeProduced() - f->getQuantity() + offset * scale;
    double endQty = startQty + qty * scale;
    if (f->getCumulativeConsumed() <= startQty + ROUNDING_ERROR)
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
          double newoffset = 0.0;
          if (f->getCumulativeConsumed()+f->getQuantity() < startQty)
          {
            newoffset = startQty - (f->getCumulativeConsumed()+f->getQuantity());
            newqty -= newoffset;
          }
          if (f->getCumulativeConsumed() > endQty)
            newqty -= f->getCumulativeConsumed() - endQty;
          OperationPlan *opplan = dynamic_cast<FlowPlan*>(&(*f))->getOperationPlan();
          OperationPlan *topopplan = opplan->getTopOwner();
          if (topopplan->getOperation()->getType() == *OperationSplit::metadata)
            topopplan = opplan;
          iter.updateStack(
            topopplan,
            - topopplan->getQuantity() * newqty / f->getQuantity(),
            - topopplan->getQuantity() * newoffset / f->getQuantity(),
            lvl
            );
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
          double newoffset = 0.0;
          if (f->getCumulativeConsumed()+f->getQuantity() < startQty)
            newqty -= startQty - (f->getCumulativeConsumed()+f->getQuantity());
          if (f->getCumulativeConsumed() > endQty)
            newqty -= f->getCumulativeConsumed() - endQty;
          OperationPlan *opplan = dynamic_cast<FlowPlan*>(&(*f))->getOperationPlan();
          OperationPlan *topopplan = opplan->getTopOwner();
          if (topopplan->getOperation()->getType() == *OperationSplit::metadata)
            topopplan = opplan;
          iter.updateStack(
            topopplan,
            - topopplan->getQuantity() * newqty / f->getQuantity(),
            - topopplan->getQuantity() * newoffset / f->getQuantity(),
            lvl
            );
        }
        --f;
      }
    }
  }
}


DECLARE_EXPORT Operation* BufferProcure::getOperation() const
{
  if (!oper)
  {
    Operation *o = Operation::find(PURCHASE_OPERATION);
    if (!o)
    {
      // Create a new purchase operation
      o = new OperationFixedTime();
      o->setName(PURCHASE_OPERATION);
      static_cast<OperationFixedTime*>(o)->setDuration(leadtime);
      new FlowEnd(o, const_cast<BufferProcure*>(this), 1);
    }
    // Copy procurement parameters to the existing operation
    if (o->getType() == *OperationFixedTime::metadata)
      static_cast<OperationFixedTime*>(o)->setDuration(leadtime);
    const_cast<BufferProcure*>(this)->oper = o;
    o->setFence(getFence());
    o->setSizeMaximum(getSizeMaximum());
    o->setSizeMinimum(getSizeMinimum());
    o->setSizeMultiple(getSizeMultiple());
    if (!o->getLocation()) o->setLocation(getLocation());
    o->setSource(getSource());
  }
  return oper;
}


DECLARE_EXPORT void Buffer::buildProducingOperation()
{
  if (producing_operation
    && producing_operation != uninitializedProducing
    && !producing_operation->getHidden())
    // Leave manually specified producing operations alone
    return;

  // Loop over this item and all its parent items
  Item* item = getItem();
  while (item)
  {
    // Loop over all suppliers of this item+location combination
    Item::supplierlist::const_iterator supitem_iter = item->getSupplierIterator();
    while (ItemSupplier *supitem = supitem_iter.next())
    {
      // Verify whether the ItemSupplier is applicable to the buffer location
      // We need to reject the following 2 mismatches:
      //   - buffer location is not null, and is not a member of the
      //     ItemSupplier location
      //   - buffer location is null, and the ItemSupplier location isn't
      if (supitem->getLocation())
      {
        if ((getLocation() && !getLocation()->isMemberOf(supitem->getLocation()))
          || !getLocation())
          continue;
      }

      // Check if there is already a producing operation referencing this ItemSupplier
      if (producing_operation && producing_operation != uninitializedProducing)
      {
        if (producing_operation->getType() == *OperationItemSupplier::metadata)
        {
          OperationItemSupplier* o = static_cast<OperationItemSupplier*>(producing_operation);
          if (o->getItemSupplier() == supitem)
            // Already exists
            continue;
        }
        else
        {
          SubOperation::iterator subiter(producing_operation->getSubOperations());
          while (SubOperation *o = subiter.next())
            if (o->getOperation()->getType() == *OperationItemSupplier::metadata)
            {
              OperationItemSupplier* s = static_cast<OperationItemSupplier*>(o->getOperation());
              if (s->getItemSupplier() == supitem)
                // Already exists
                continue;
            }
        }
      }

      // New operation needs to be created
      OperationItemSupplier *oper = OperationItemSupplier::findOrCreate(supitem, this);

      // Merge the new operation in an alternate operation if required
      if (producing_operation && producing_operation != uninitializedProducing)
      {
        // We're not the first
        SubOperation* subop = new SubOperation();
        subop->setOperation(oper);
        subop->setPriority(supitem->getPriority());
        subop->setEffective(supitem->getEffective());
        if (producing_operation->getType() != *OperationAlternate::metadata)
        {
          // We are the second: create an alternate and add 2 suboperations
          OperationAlternate *superop = new OperationAlternate();
          stringstream o;
          o << "Replenish " << getName();
          superop->setName(o.str());
          superop->setHidden(true);
          superop->setSearch("PRIORITY");
          SubOperation* subop2 = new SubOperation();
          subop2->setOperation(producing_operation);
          // Note that priority and effectivity are at default values.
          // If not, the alternate would already have been created.
          subop2->setOwner(superop);
          producing_operation = superop;
          subop->setOwner(producing_operation);
        }
        else
          // We are third or later: just add a suboperation
          subop->setOwner(producing_operation);
      }
      else
      {
        // We are the first: only create an operationItemSupplier instance
        if (supitem->getEffective() == DateRange() && supitem->getPriority() == 1)
          // Use a single operation. If an alternate is required
          // later on, we know it has the default priority and effectivity.
          producing_operation = oper;
        else
        {
          // Already create an alternate now
          OperationAlternate *superop = new OperationAlternate();
          producing_operation = superop;
          stringstream o;
          o << "Replenish " << getName();
          superop->setName(o.str());
          superop->setHidden(true);
          superop->setSearch("PRIORITY");
          SubOperation* subop = new SubOperation();
          subop->setOperation(oper);
          subop->setPriority(supitem->getPriority());
          subop->setEffective(supitem->getEffective());
          subop->setOwner(superop);
        }
      }
    } // End loop over itemsuppliers

    // Loop over all item distributions to replenish this item+location combination
    Item::distributionIterator itemdist_iter = item->getDistributionIterator();
    while (ItemDistribution *itemdist = itemdist_iter.next())
    {
      // Verify whether the ItemDistribution is applicable to the buffer location
      // We need to reject the following 2 mismatches:
      //   - buffer location is not null, and is not a member of the
      //     ItemDistribution destination location
      //   - buffer location is null, and the ItemDistribution destination location isn't
      if (itemdist->getDestination())
      {
        if ((getLocation() && !getLocation()->isMemberOf(itemdist->getDestination()))
          || !getLocation())
          continue;
      }

      // Check if there is already a producing operation referencing this ItemDistribution
      if (producing_operation && producing_operation != uninitializedProducing)
      {
        if (producing_operation->getType() == *OperationItemDistribution::metadata)
        {
          OperationItemDistribution* o = static_cast<OperationItemDistribution*>(producing_operation);
          if (o->getItemDistribution() == itemdist)
            // Already exists
            continue;
        }
        else
        {
          SubOperation::iterator subiter(producing_operation->getSubOperations());
          while (SubOperation *o = subiter.next())
            if (o->getOperation()->getType() == *OperationItemDistribution::metadata)
            {
              OperationItemDistribution* s = static_cast<OperationItemDistribution*>(o->getOperation());
              if (s->getItemDistribution() == itemdist)
                // Already exists
                continue;
            }
        }
      }

      // New operation needs to be created for each allowed source location
      for (Location::iterator loc = Location::begin(); loc != Location::end(); ++loc)
      {
        // Skip unqualified locations
        if (itemdist->getOrigin() && !loc->isMemberOf(itemdist->getOrigin()))
          continue;

        // Find or create the source buffer
        Buffer* originbuf = NULL;
        for (Buffer::iterator buf = Buffer::begin(); buf != Buffer::end(); ++buf)
          if (buf->getLocation() == &*loc && buf->getItem() == getItem())
          {
            originbuf = &*buf;
            break;
          }
        if (!originbuf)
        {
          originbuf = new BufferDefault();
          originbuf->setItem(getItem());
          originbuf->setLocation(&*loc);
          stringstream o;
          o << getItem() << " @ " << loc->getName();
          // We assume there is no buffer with that name yet. If there is,
          // we'll get an error here.
          originbuf->setName(o.str());
        }

        // Create new operation
        OperationItemDistribution *oper = new OperationItemDistribution(itemdist, originbuf, this);

        // Merge the new operation in an alternate operation if required
        if (producing_operation && producing_operation != uninitializedProducing)
        {
          // We're not the first
          SubOperation* subop = new SubOperation();
          subop->setOperation(oper);
          subop->setPriority(itemdist->getPriority());
          subop->setEffective(itemdist->getEffective());
          if (producing_operation->getType() != *OperationAlternate::metadata)
          {
            // We are the second: create an alternate and add 2 suboperations
            OperationAlternate *superop = new OperationAlternate();
            stringstream o;
            o << "Replenish " << getName();
            superop->setName(o.str());
            superop->setHidden(true);
            superop->setSearch("PRIORITY");
            SubOperation* subop2 = new SubOperation();
            subop2->setOperation(producing_operation);
            // Note that priority and effectivity are at default values.
            // If not, the alternate would already have been created.
            subop2->setOwner(superop);
            producing_operation = superop;
            subop->setOwner(producing_operation);
          }
          else
            // We are third or later: just add a suboperation
            subop->setOwner(producing_operation);
        }
        else
        {
          // We are the first: only create an operationItemSupplier instance
          if (itemdist->getEffective() == DateRange() && itemdist->getPriority() == 1)
            // Use a single operation. If an alternate is required
            // later on, we know it has the default priority and effectivity.
            producing_operation = oper;
          else
          {
            // Already create an alternate now
            OperationAlternate *superop = new OperationAlternate();
            producing_operation = superop;
            stringstream o;
            o << "Replenish " << getName();
            superop->setName(o.str());
            superop->setHidden(true);
            superop->setSearch("PRIORITY");
            SubOperation* subop = new SubOperation();
            subop->setOperation(oper);
            subop->setPriority(itemdist->getPriority());
            subop->setEffective(itemdist->getEffective());
            subop->setOwner(superop);
          }
        }
      } // End loop over origin locations

    } // End loop over itemdistributions

    // While-loop to add suppliers defined at parent items
    item = item->getOwner();
  }

  if (producing_operation == uninitializedProducing)
  {
    // No producer could be generated. No replenishment will be possible.
    new ProblemInvalidData(this, "no replenishment defined", "buffer",
      Date::infinitePast, Date::infiniteFuture, 1);
    logger << "Warning: Can't replenish buffer '" << this << "'" << endl;
    producing_operation = NULL;
  }
}


} // end namespace
