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

DECLARE_EXPORT const MetaCategory* PeggingIterator::metadata;


int PeggingIterator::initialize()
{
  // Initialize the pegging metadata
  PeggingIterator::metadata = MetaCategory::registerCategory<PeggingIterator>("pegging","peggings");
  registerFields<PeggingIterator>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python type
  PythonType& x = PythonExtension<PeggingIterator>::getPythonType();
  x.setName("peggingIterator");
  x.setDoc("frePPLe iterator for demand pegging");
  x.supportgetattro();
  x.supportiter();
  const_cast<MetaCategory*>(PeggingIterator::metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT PeggingIterator::PeggingIterator(const Demand* d)
  : downstream(false), firstIteration(true), first(false)
{
  initType(metadata);
  const Demand::OperationPlanList &deli = d->getDelivery();
  for (Demand::OperationPlanList::const_iterator opplaniter = deli.begin();
      opplaniter != deli.end(); ++opplaniter)
  {
    OperationPlan *t = (*opplaniter)->getTopOwner();
    updateStack(t, t->getQuantity(), 0.0, 0);
  }
}


DECLARE_EXPORT PeggingIterator::PeggingIterator(const OperationPlan* opplan, bool b)
  : downstream(b), firstIteration(true), first(false)
{
  initType(metadata);
  if (!opplan) return;
  if (opplan->getTopOwner()->getOperation()->getType() == *OperationSplit::metadata)
    updateStack(
      opplan,
      opplan->getQuantity(),
      0.0,
      0
      );
  else
    updateStack(
      opplan->getTopOwner(),
      opplan->getTopOwner()->getQuantity(),
      0.0,
      0
      );
}


DECLARE_EXPORT PeggingIterator::PeggingIterator(FlowPlan* fp, bool b)
  : downstream(b), firstIteration(true), first(false)
{
  initType(metadata);
  if (!fp) return;
  updateStack(
    fp->getOperationPlan()->getTopOwner(),
    fp->getOperationPlan()->getQuantity(),
    0.0,
    0
    );
}


DECLARE_EXPORT PeggingIterator::PeggingIterator(LoadPlan* lp, bool b)
  : downstream(b), firstIteration(true), first(false)
{
  initType(metadata);
  if (!lp) return;
  updateStack(
    lp->getOperationPlan()->getTopOwner(),
    lp->getOperationPlan()->getQuantity(),
    0.0,
    0
    );
}


DECLARE_EXPORT PeggingIterator& PeggingIterator::operator--()
{
  // Validate
  if (states.empty())
    throw LogicException("Incrementing the iterator beyond it's end");
  if (downstream)
    throw LogicException("Decrementing a downstream iterator");

  // Mark the top entry in the stack as invalid, so it can be reused.
  first = true;

  // Find other operationplans to add to the stack
  state t = states.back(); // Copy the top element
  followPegging(t.opplan, t.quantity, t.offset, t.level);

  // Pop invalid top entry from the stack.
  // This will happen if we didn't find an operationplan to replace the
  // top entry.
  if (first) states.pop_back();

  return *this;
}


DECLARE_EXPORT PeggingIterator& PeggingIterator::operator++()
{
  // Validate
  if (states.empty())
    throw LogicException("Incrementing the iterator beyond it's end");
  if (!downstream)
    throw LogicException("Incrementing an upstream iterator");

  // Mark the top entry in the stack as invalid, so it can be reused.
  first = true;

  // Find other operationplans to add to the stack
  state t = states.back(); // Copy the top element
  followPegging(t.opplan, t.quantity, t.offset, t.level);

  // Pop invalid top entry from the stack.
  // This will happen if we didn't find an operationplan to replace the
  // top entry.
  if (first) states.pop_back();

  return *this;
}


DECLARE_EXPORT void PeggingIterator::followPegging
(const OperationPlan* op, double qty, double offset, short lvl)
{
  // Zero quantity operationplans don't have further pegging
  if (!op->getQuantity()) return;

  // For each flowplan ask the buffer to find the pegged operationplans.
  if (downstream)
    for (OperationPlan::FlowPlanIterator i = op->beginFlowPlans();
        i != op->endFlowPlans(); ++i)
    {
      if (i->getQuantity() > ROUNDING_ERROR) // Producing flowplan
        i->getFlow()->getBuffer()->followPegging(*this, &*i, qty, offset, lvl+1);
    }
  else
    for (OperationPlan::FlowPlanIterator i = op->beginFlowPlans();
        i != op->endFlowPlans(); ++i)
    {
      if (i->getQuantity() < -ROUNDING_ERROR) // Consuming flowplan
        i->getFlow()->getBuffer()->followPegging(*this, &*i, qty, offset, lvl+1);
    }

  // Push child operationplans on the stack.
  // The pegged quantity is equal to the ratio of the quantities of the
  // parent and child operationplan.
  for (OperationPlan::iterator j(op); j != OperationPlan::end(); ++j)
    updateStack(
      &*j,
      qty * j->getQuantity() / op->getQuantity(),
      offset * j->getQuantity() / op->getQuantity(),
      lvl+1
      );
}


DECLARE_EXPORT PeggingIterator* PeggingIterator::next()
{
  if (firstIteration)
    firstIteration = false;
  else if (downstream)
    operator++();
  else
    operator--();
  if (!operator bool())
    return NULL;
  else
    return this;
}


DECLARE_EXPORT void PeggingIterator::updateStack
(const OperationPlan* op, double qty, double o, short lvl)
{
  // Avoid very small pegging quantities
  if (qty < ROUNDING_ERROR) return;

  if (first)
  {
    // Update the current top element of the stack
    state& t = states.back();
    t.opplan = op;
    t.quantity = qty;
    t.offset = o;
    t.level = lvl;
    first = false;
  }
  else
    // We need to create a new element on the stack
    states.push_back( state(op, qty, o, lvl) );
}


} // End namespace
