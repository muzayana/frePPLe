/***************************************************************************
 *                                                                         *
 * Copyright (C) 2016 by frePPLe bvba                                      *
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


DECLARE_EXPORT const MetaClass* OperatorMoveOut::metadata;


int OperatorMoveOut::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<OperatorMoveOut>(
    "solver", "solver_moveout", Object::create<OperatorMoveOut>
    );
  registerFields<OperatorMoveOut>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  PythonType& x = FreppleClass<OperatorMoveOut, Solver>::getPythonType();
  x.setName("solver_moveout");
  x.setDoc("frePPLe solver_moveout");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("solve", solve, METH_NOARGS, "run the solver");
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


PyObject* OperatorMoveOut::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Create the solver
    OperatorMoveOut *s = new OperatorMoveOut();

    // Iterate over extra keywords, and set attributes.   @todo move this responsibility to the readers...
    if (kwds)
    {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(kwds, &pos, &key, &value))
      {
        PythonData field(value);
        PyObject* key_utf8 = PyUnicode_AsUTF8String(key);
        DataKeyword attr(PyBytes_AsString(key_utf8));
        Py_DECREF(key_utf8);
        const MetaFieldBase* fmeta = OperatorMoveOut::metadata->findField(attr.getHash());
        if (!fmeta)
          fmeta = Solver::metadata->findField(attr.getHash());
        if (fmeta)
          // Update the attribute
          fmeta->setField(s, field);
        else
          s->setProperty(attr.getName(), value);
      };
    }

    // Return the object
    //Py_INCREF(s);  // XXX TODO SHOULD the ref count be set to one? Or do we prevent the opbject from being garbage collected
    return static_cast<PyObject*>(s);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


DECLARE_EXPORT void OperatorMoveOut::solve(void *v)
{
  // Solve in parallel threads, unless we run in debug mode
  // or unless there is only a single cluster.
  int cl = HasLevel::getNumberOfClusters();
  ThreadGroup threads;
  if (getLogLevel()>0 || cl == 1)
    threads.setMaxParallel(1);

  // Register all clusters to be solved
  for (int j = 0; j < cl; ++j)
    threads.add(
      OperatorMoveOutData::runme,
      new OperatorMoveOutData(this, j)
      );

  // Run the planning command threads and wait for them to exit
  threads.execute();
}


PyObject* OperatorMoveOut::solve(PyObject *self, PyObject *args)
{
  // Parse the argument
  PyObject *obj = NULL;
  short objtype = 0;
  if (args && !PyArg_ParseTuple(args, "|O:solve", &obj)) return NULL;
  if (obj)
  {
    if (PyObject_TypeCheck(obj, Buffer::metadata->pythonClass))
      objtype = 1;
    else
    {
      PyErr_SetString(
        PythonDataException,
        "solve(d) argument must be a buffer"
        );
      return NULL;
    }
  }

  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    OperatorMoveOut* sol = static_cast<OperatorMoveOut*>(self);
    switch (objtype)
    {
      case 0:
        // Solve all buffers
        sol->solve();
        break;
      case 1:
        // Solve a single buffer
        sol->solve(static_cast<Buffer*>(obj));
        break;
    }
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


DECLARE_EXPORT void OperatorMoveOut::OperatorMoveOutData::commit()
{
  if (sol->getLogLevel()>0)
    logger << "Start solving cluster " << cluster << " at " << Date::now() << endl;

  // Move out all operationplans beyond the fence
  if (sol->isFenceConstrained() || sol->isLeadTimeConstrained())
  {
    for (Operation::iterator o = Operation::begin(); o != Operation::end(); ++o)
    {
      if (o->getCluster() == cluster)
      {
        try
        {
          sol->solve(&*o, this);
        }
        catch(...)
        {
          logger << "Error: Caught an exception while solving operation '"
              << o->getName() << "':" << endl;
          try {throw;}
          catch (const bad_exception&) {logger << "  bad exception" << endl;}
          catch (const exception& e) {logger << "  " << e.what() << endl;}
          catch (...) {logger << "  Unknown type" << endl;}
        }
      }
    }
  }

  // Propagate the shortage across all buffers, starting from the deepest level
  for (short lvl = HasLevel::getNumberOfLevels(); lvl; --lvl)
    for (Buffer::iterator b = Buffer::begin(); b != Buffer::end(); ++b)
      if (b->getCluster() == cluster && b->getLevel() == lvl)
      {
        try
        {
          sol->solve(&*b, this);
        }
        catch(...)
        {
          logger << "Error: Caught an exception while solving buffer '"
              << b->getName() << "':" << endl;
          try {throw;}
          catch (const bad_exception&) {logger << "  bad exception" << endl;}
          catch (const exception& e) {logger << "  " << e.what() << endl;}
          catch (...) {logger << "  Unknown type" << endl;}
        }
      }

  if (sol->getLogLevel()>0)
    logger << "End solving cluster " << cluster << " at " << Date::now() << endl;
}


DECLARE_EXPORT void OperatorMoveOut::solve(const Operation* oper, void* v)
{
  bool firstmsg = true;

  // Compute the threshold date
  Date earliest = Plan::instance().getCurrent();
  if (isFenceConstrained() && oper->getFence() > 0L)
    earliest += oper->getFence();

  // Loop over operationplans
  OperationPlan::iterator opplan_iter(oper);
  while (OperationPlan *opplan = opplan_iter.next())
  {
    if (opplan->getStart() < earliest && !opplan->getLocked())
    {
      DateRange orig = opplan->getDates();
      opplan->setStart(earliest);  // TODO use move command
      if (getLogLevel() > 0 && firstmsg)
      {
        logger << "  Solving operation " << oper << endl;
        firstmsg = false;
      }
      if (getLogLevel() > 2)
        logger << "    Moving from " << orig << " to " << opplan->getDates() << endl; 
    }
  }
}


DECLARE_EXPORT void OperatorMoveOut::solve(const Resource* res, void* v)
{
  if (getLogLevel() > 0)
    logger << "  Solving resource " << res << endl;
}


DECLARE_EXPORT void OperatorMoveOut::solve(const Buffer* buf, void* v)
{
  if (getLogLevel() > 0)
    logger << "  Solving buffer " << buf << endl;

  // Check out the onhand at the end of the horizon, because this solver can
  // only solve temporal problems.
  if (buf->getOnHand(Date::infiniteFuture) < 0)
  {
    if (getLogLevel() > 0)
      logger << "  Can't solve the shortage by moving operationplans" << endl;
    return;
  }

  // Loop over all consumers in the buffer and scan for shortages
  // Assume all producers are already moved to their best possible date.
  //   for all shortages:
  //     loop over unlocked consumers with a start date before the end of the shortage
  //       walk over the pegging, to find demand with lowest end priority and highest due date & qty pegged
  //     select best candidate & move to consume after the next producer date/shortage end, eventually splitting the operationplan
  //   repeat till complete shortage is resolved
  bool shortage = false;
  double shortage_qty = 0.0;
  do
  {
    // Scan for shortage
    shortage = false;
    shortage_qty = 0.0;
    Date prevdate;
    for (Buffer::flowplanlist::const_iterator cur=buf->getFlowPlans().begin(); 
      cur != buf->getFlowPlans().end(); ++cur)
    {
      // Only evaluate date change
      if (cur->getDate() == prevdate)
        continue;
      prevdate = cur->getDate();

      if (cur->getOnhand() < -ROUNDING_ERROR)
      {
        // Shortage starting here
        shortage = true;

        // Scroll forward till the first producer after the shortage start
        while (cur != buf->getFlowPlans().end() && cur->getQuantity() <= 0)
        {
          if (cur->getDate() > prevdate)
          {
            prevdate = cur->getDate();
            if (cur->getOnhand() < shortage_qty) 
              shortage_qty = cur->getOnhand();
          }
          ++cur;
        }
        prevdate = cur->getDate();
        const Buffer::flowplanlist::Event *reference = &*cur;
        while (cur != buf->getFlowPlans().end() && cur->getDate() == prevdate)
        {
          reference = &*cur;
          ++cur;
        }
        // At this point the variable "reference" is pointing to the last 
        // event at the time of the new producer.

        // Evaluate all consumers before this producer.
        const FlowPlan* candidate = NULL;
        Demand* candidate_score = NULL;
        for (Buffer::flowplanlist::const_iterator cur2=buf->getFlowPlans().begin(); 
          cur2 != buf->getFlowPlans().end(); ++cur2)
        {
          if (cur2->getDate() >= reference->getDate())
            break;
          if (cur2->getEventType() == 1 && cur2->getQuantity() < 0)
          {
            const FlowPlan* flplan = static_cast<const FlowPlan*>(&*cur2);
            if (!flplan->getOperationPlan()->getLocked())
            {
              Demand* score = evaluateCandidate(flplan->getOperationPlan());
              if (score && candidate && !candidate_score)
                // This flowplan goes to a demand, and we already have a candidate
                // who doesn't. -> Stick with existing candidate
                continue;
              if (score && candidate_score 
                && SolverMRP::demand_comparison(score, candidate_score))
                // Previous candidate has lower priority
                continue;
              // A better candidate is found 
              candidate = flplan;
              candidate_score = score;
            }
          }
        }
        
        // Move the new candidate
        if (candidate)
        {
          // Evaluate total shortage
          if (reference->getOnhand() < 0)
            // Shortage is only partially resolved by this producer
            shortage_qty -= reference->getOnhand();
          bool move = true;
          /*
          if (
            shortage_qty > candidate->getQuantity()
            && (candidate->getFlow()->getType() == *FlowStart::metadata 
               || candidate->getFlow()->getType() == *FlowEnd::metadata)
            )
          {
            // The shortage is smaller than the flowplan we want to move.
            // We resize the orginal candidate and create another operationplan.
            logger << "before split " << candidate->getOperationPlan()->getQuantity() << endl;
            OperationPlan* newcandidate = new OperationPlan(*(candidate->getOperationPlan()), true);
            newcandidate->setQuantity(newcandidate->getQuantity() * shortage_qty / candidate->getQuantity());
            double orig_qty = candidate->getQuantity();
            const_cast<FlowPlan*>(candidate)->setQuantity(candidate->getQuantity() - shortage_qty);
            if (newcandidate->getQuantity() && candidate->getQuantity())
              logger << "ok" << endl;
            else
            {
              // Undo the split, as things didn't work out
              const_cast<FlowPlan*>(candidate)->setQuantity(orig_qty);
              delete newcandidate;
              move = false;
            }
            logger << "after split " << candidate->getOperationPlan()->getQuantity() << endl;            
          }
          */
          if (move)
          {
            // Move the complete consuming flowplan
            DateRange orig = candidate->getOperationPlan()->getDates();
            if (candidate->getFlow()->getType() == *FlowStart::metadata
              || candidate->getFlow()->getType() == *FlowFixedStart::metadata)
              candidate->getOperationPlan()->setStart(reference->getDate());
            else
              candidate->getOperationPlan()->setEnd(reference->getDate());
            if (getLogLevel() > 2)
               logger << "    Moving " << candidate->getOperationPlan()->getOperation() 
                 << " (quantity: " << candidate->getOperationPlan()->getQuantity() 
                 << ") from " << orig 
                 << " to " << candidate->getOperationPlan()->getDates() << endl;
          }
        }
        else
        {
          shortage = false;
          logger << "  No candidate found to resolve the problem" << endl;
        }
      }
    }
  }
  while (shortage);
}


DECLARE_EXPORT Demand* OperatorMoveOut::evaluateCandidate(const OperationPlan* opplan)
{
  Demand *curMostUrgent = NULL;
  for (PeggingIterator p(const_cast<OperationPlan*>(opplan)); p; ++p)
  {
    const OperationPlan* m = p.getOperationPlan();
    Demand* dmd = m ? m->getTopOwner()->getDemand() : NULL;
    if (dmd && (!curMostUrgent || SolverMRP::demand_comparison(dmd, curMostUrgent)))
      curMostUrgent = dmd;
  }
  return curMostUrgent;
}

}
