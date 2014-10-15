/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba                 *
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

DECLARE_EXPORT const MetaCategory* FlowPlan::metadata;


int FlowPlan::initialize()
{
  // Initialize the metadata
  metadata = new MetaCategory("flowplan", "flowplans");

  // Initialize the Python type
  PythonType& x = FreppleCategory<FlowPlan>::getType();
  x.setName("flowplan");
  x.setDoc("frePPLe flowplan");
  x.supportgetattro();
  const_cast<MetaCategory*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT FlowPlan::FlowPlan (OperationPlan *opplan, const Flow *f)
{
  assert(opplan && f);
  fl = const_cast<Flow*>(f);

  // Initialize the Python type
  initType(metadata);

  // Link the flowplan to the operationplan
  oper = opplan;
  nextFlowPlan = NULL;
  if (opplan->firstflowplan)
  {
    // Append to the end
    FlowPlan *c = opplan->firstflowplan;
    while (c->nextFlowPlan) c = c->nextFlowPlan;
    c->nextFlowPlan = this;
  }
  else
    // First in the list
    opplan->firstflowplan = this;

  // Compute the flowplan quantity
  fl->getBuffer()->flowplans.insert(
    this,
    fl->getFlowplanQuantity(this),
    fl->getFlowplanDate(this)
  );

  // Mark the operation and buffer as having changed. This will trigger the
  // recomputation of their problems
  fl->getBuffer()->setChanged();
  fl->getOperation()->setChanged();
}


DECLARE_EXPORT void FlowPlan::update()
{
  // Update the timeline data structure
  fl->getBuffer()->flowplans.update(
    this,
    fl->getFlowplanQuantity(this),
    fl->getFlowplanDate(this)
  );

  // Mark the operation and buffer as having changed. This will trigger the
  // recomputation of their problems
  fl->getBuffer()->setChanged();
  fl->getOperation()->setChanged();
}


DECLARE_EXPORT void FlowPlan::setFlow(const Flow* newfl)
{
  // No change
  if (newfl == fl) return;

  // Verify the data
  if (!newfl) throw LogicException("Can't switch to NULL flow");

  // Remove from the old buffer, if there is one
  if (fl)
  {
    if (fl->getOperation() != newfl->getOperation())
      throw LogicException("Only switching to a flow on the same operation is allowed");
    fl->getBuffer()->flowplans.erase(this);
    fl->getBuffer()->setChanged();
  }

  // Insert in the new buffer
  fl = newfl;
  fl->getBuffer()->flowplans.insert(
    this,
    fl->getFlowplanQuantity(this),
    fl->getFlowplanDate(this)
  );
  fl->getBuffer()->setChanged();
  fl->getOperation()->setChanged();
}


// Remember that this method only superficially looks like a normal
// writeElement() method.
DECLARE_EXPORT void FlowPlan::writeElement(XMLOutput *o, const Keyword& tag, mode m) const
{
  o->BeginObject(tag);
  o->writeElement(Tags::tag_date, getDate());
  o->writeElement(Tags::tag_quantity, getQuantity());
  o->writeElement(Tags::tag_onhand, getOnhand());
  o->writeElement(Tags::tag_minimum, getMin());
  o->writeElement(Tags::tag_maximum, getMax());
  if (!dynamic_cast<OperationPlan*>(o->getCurrentObject()))
    o->writeElement(Tags::tag_operationplan, &*getOperationPlan());

  // Write pegging info.
  if (o->getContentType() == XMLOutput::PLANDETAIL)
  {
    // Write the upstream pegging
    PeggingIterator k(this, false);
    if (k) --k;
    for (; k; --k)
      o->writeElement(Tags::tag_pegging,
        Tags::tag_level, -k.getLevel(),
        Tags::tag_operationplan, k.getOperationPlan()->getIdentifier(),
        Tags::tag_quantity, k.getQuantity()
        );

    // Write the downstream pegging
    PeggingIterator l(this, true);
    if (l) ++l;
    for (; l; ++l)
      o->writeElement(Tags::tag_pegging,
        Tags::tag_level, l.getLevel(),
        Tags::tag_operationplan, l.getOperationPlan()->getIdentifier(),
        Tags::tag_quantity, l.getQuantity()
        );
  }

  o->EndObject(tag);
}


PyObject* FlowPlan::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_operationplan))
    return PythonObject(getOperationPlan());
  if (attr.isA(Tags::tag_quantity))
    return PythonObject(getQuantity());
  if (attr.isA(Tags::tag_flow))
    return PythonObject(getFlow());
  if (attr.isA(Tags::tag_date))
    return PythonObject(getDate());
  if (attr.isA(Tags::tag_onhand))
    return PythonObject(getOnhand());
  if (attr.isA(Tags::tag_buffer)) // Convenient shortcut
    return PythonObject(getFlow()->getBuffer());
  if (attr.isA(Tags::tag_operation)) // Convenient shortcut
    return PythonObject(getFlow()->getOperation());
  return NULL;
}


int FlowPlanIterator::initialize()
{
  // Initialize the type
  PythonType& x = PythonExtension<FlowPlanIterator>::getType();
  x.setName("flowplanIterator");
  x.setDoc("frePPLe iterator for flowplan");
  x.supportiter();
  return x.typeReady();
}


PyObject* FlowPlanIterator::iternext()
{
  FlowPlan* fl;
  if (buffer_or_opplan)
  {
    // Skip uninteresting entries
    while (*bufiter != buf->getFlowPlans().end() && (*bufiter)->getQuantity()==0.0)
      ++(*bufiter);
    if (*bufiter == buf->getFlowPlans().end()) return NULL;
    fl = const_cast<FlowPlan*>(static_cast<const FlowPlan*>(&*((*bufiter)++)));
  }
  else
  {
    // Skip uninteresting entries
    while (*opplaniter != opplan->endFlowPlans() && (*opplaniter)->getQuantity()==0.0)
      ++(*opplaniter);
    if (*opplaniter == opplan->endFlowPlans()) return NULL;
    fl = static_cast<FlowPlan*>(&*((*opplaniter)++));
  }
  Py_INCREF(fl);
  return const_cast<FlowPlan*>(fl);
}

} // end namespace
