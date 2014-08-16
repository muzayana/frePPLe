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
#include "frepple/solver.h"

namespace frepple
{


const MetaClass* OperatorDelete::metadata;


int OperatorDelete::initialize()
{
  // Initialize the metadata
  metadata = new MetaClass(
    "solver", "solver_delete", Object::createString<OperatorDelete>, true
    );

  // Initialize the Python class
  FreppleClass<OperatorDelete, Solver>::getType().addMethod(
    "solve", solve, METH_VARARGS, "run the solver"
    );
  return FreppleClass<OperatorDelete, Solver>::initialize();
}


void OperatorDelete::solve(void *v)
{
   // Loop over all buffers
   // Push to stack, in order of level

  // Clean up all buffers in the list
  while(!buffersToScan.empty())
  {
      Buffer* curbuf = buffersToScan.back();
      buffersToScan.pop_back();
      solve(curbuf);
  }
}


void OperatorDelete::solve(const Resource* r, void* v)
{
  // Loop over all operationplans on the resource
  for (Resource::loadplanlist::const_iterator i = r->getLoadPlans().begin();
    i != r->getLoadPlans().end(); ++i)
  {
    const LoadPlan* lp = dynamic_cast<const LoadPlan*>(&*i);
    if (lp)
      // Add all buffers into which material is produced to the stack
      pushBuffers(lp->getOperationPlan(), false);
  }

  // Process all buffers found, and their upstream colleagues
  while(!buffersToScan.empty())
  {
      Buffer* curbuf = buffersToScan.back();
      buffersToScan.pop_back();
      solve(curbuf);
  }
}


void OperatorDelete::solve(const Demand* d, void* v)
{
  logger << "delete for " << d << endl;

  // Delete all delivery operationplans.
  // Note that an extra loop is used to assure that our iterator doesn't get
  // invalidated during the deletion.
  while (true)
  {
    // Find a candidate operationplan to delete
    OperationPlan *candidate = NULL;
    const Demand::OperationPlan_list& deli = d->getDelivery();
    for (Demand::OperationPlan_list::const_iterator i = deli.begin(); i != deli.end(); ++i)
      if (!(*i)->getLocked())
      {
        candidate = *i;
        break;
      }
      if (!candidate) break;

      // Push the buffer on the stack in which the deletion creates excess inventory
      pushBuffers(candidate, true);

      // Delete only the delivery, immediately or through a delete command
      if (cmds)
        cmds->add(new CommandDeleteOperationPlan(candidate));
      else
        delete candidate;
  }

  // Propagate to all upstream buffers
  while(!buffersToScan.empty())
  {
    Buffer* curbuf = buffersToScan.back();
    buffersToScan.pop_back();
    solve(curbuf);
  }
}


void OperatorDelete::pushBuffers(OperationPlan* o, bool consuming)
{
  // Loop over all flowplans
  for (OperationPlan::FlowPlanIterator i = o->beginFlowPlans(); i != o->endFlowPlans(); ++i)
  {
    // Skip flowplans we're not interested in
    if ((consuming && i->getQuantity() >= 0)
      || (!consuming && i->getQuantity() <= 0))
      continue;

    // Check if the buffer is already found on the stack
    bool found = false;
    for (int j = buffersToScan.size()-1; j>=0 && !found; --j)
      if (buffersToScan[j] == i->getBuffer())
        found = true;

    // Add the buffer to the stack
    if (!found) buffersToScan.push_back(const_cast<Buffer*>(i->getBuffer()));
  }

  // Recursive call for all suboperationplans
  for (OperationPlan::iterator subopplan(o); subopplan != OperationPlan::end(); ++subopplan)
    pushBuffers(&*subopplan, consuming);
}


void OperatorDelete::solve(const Buffer* b, void* v)
{
  Buffer::flowplanlist::const_iterator fiter = b->getFlowPlans().rbegin();
  Buffer::flowplanlist::const_iterator fend = b->getFlowPlans().end();
  if (fiter == fend)
    return; // There isn't a single flowplan in the buffer
  double excess = fiter->getOnhand() - fiter->getMin();

  // Find the earliest occurence of the excess
  fiter = b->getFlowPlans().begin();
  while (excess > ROUNDING_ERROR && fiter != fend)
  {
    if (fiter->getQuantity() <= 0)
    {
      // Not a producer
      ++fiter;
      continue;
    }
    FlowPlan* fp = const_cast<FlowPlan*>(dynamic_cast<const FlowPlan*>(&*fiter));
    double cur_excess = b->getFlowPlans().getExcess(&*fiter);
    if (!fp || fp->getOperationPlan()->getLocked() || cur_excess < ROUNDING_ERROR)
    {
      // No excess producer, or it's locked
      ++fiter;
      continue;
    }
    assert(fp);
    ++fiter;  // Increment the iterator here, because it can get invalidated later on
    if (cur_excess >= fp->getQuantity() - ROUNDING_ERROR)
    {
      // The complete operationplan is excess.
      // Reduce the excess
      excess -= fp->getQuantity();
      // Add upstream buffers to the stack
      pushBuffers(fp->getOperationPlan(), true);
      // Delete operationplan
      if (cmds)
      cmds->add(new CommandDeleteOperationPlan(fp->getOperationPlan()));
      else
      delete fp->getOperationPlan();
    }
    else
    {
      // Reduce the operationplan
      double newsize = fp->setQuantity(fp->getQuantity() - cur_excess, false, false);
      if (newsize == fp->getQuantity())
        // No resizing is feasible
        continue;
      // Add upstream buffers to the stack
      pushBuffers(fp->getOperationPlan(), true);
      // Reduce the excess
      excess -= fp->getQuantity() - newsize;
      // Resize operationplan
      if (cmds)
      cmds->add(new CommandMoveOperationPlan(
        fp->getOperationPlan(), Date::infinitePast,
        fp->getOperationPlan()->getDates().getEnd(), newsize)
          );
      else
      fp->getOperationPlan()->setQuantity(newsize);
    }
  }
}


PyObject* OperatorDelete::solve(PyObject *self, PyObject *args)
{
  // Parse the argument
  PyObject *dem = NULL;
  if (args && !PyArg_ParseTuple(args, "|O:solve", &dem)) return NULL;
  if (dem && !PyObject_TypeCheck(dem, Demand::metadata->pythonClass))
  {
    PyErr_SetString(PythonDataException, "solve(d) argument must be a demand");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    OperatorDelete* sol = static_cast<OperatorDelete*>(self); 
    if (dem)
      // Delete upstream of a single demand
      sol->solve(dem);
    else
      // Remove all excess
      sol->solve();
  }
  catch(...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}

}
