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

DECLARE_EXPORT bool Plannable::anyChange = false;
DECLARE_EXPORT bool Plannable::computationBusy = false;
DECLARE_EXPORT const MetaCategory* Problem::metadata;
DECLARE_EXPORT const MetaClass* ProblemMaterialExcess::metadata,
               *ProblemMaterialShortage::metadata,
               *ProblemExcess::metadata,
               *ProblemShort::metadata,
               *ProblemEarly::metadata,
               *ProblemLate::metadata,
               *ProblemInvalidData::metadata,
               *ProblemDemandNotPlanned::metadata,
               *ProblemPrecedence::metadata,
               *ProblemBeforeFence::metadata,
               *ProblemBeforeCurrent::metadata,
               *ProblemCapacityUnderload::metadata,
               *ProblemCapacityOverload::metadata;


int Problem::initialize()
{
  // Initialize the problem metadata.
  metadata = MetaCategory::registerCategory<Problem>("problem", "problems");
  registerFields<Problem>(const_cast<MetaCategory*>(metadata));

  // Register classes.
  // We register them as default to avoid saving an xsi:type header. This
  // has no further impact as there is no factory method.
  ProblemMaterialExcess::metadata = MetaClass::registerClass<ProblemMaterialExcess>("problem", "material excess", true);
  ProblemMaterialShortage::metadata = MetaClass::registerClass<ProblemMaterialShortage>("problem", "material shortage", true);
  ProblemExcess::metadata = MetaClass::registerClass<ProblemExcess>("problem", "excess", true);
  ProblemShort::metadata = MetaClass::registerClass<ProblemShort>("problem", "short", true);
  ProblemEarly::metadata = MetaClass::registerClass<ProblemEarly>("problem", "early", true);
  ProblemLate::metadata = MetaClass::registerClass<ProblemLate>("problem", "late", true);
  ProblemInvalidData::metadata = MetaClass::registerClass<ProblemInvalidData>("problem", "invalid data", true);
  ProblemDemandNotPlanned::metadata = MetaClass::registerClass<ProblemDemandNotPlanned>("problem", "unplanned", true);
  ProblemPrecedence::metadata = MetaClass::registerClass<ProblemPrecedence>("problem", "precedence", true);
  ProblemBeforeFence::metadata = MetaClass::registerClass<ProblemBeforeFence>("problem", "before fence", true);
  ProblemBeforeCurrent::metadata = MetaClass::registerClass<ProblemBeforeCurrent>("problem", "before current", true);
  ProblemCapacityUnderload::metadata = MetaClass::registerClass<ProblemCapacityUnderload>("problem", "underload", true);
  ProblemCapacityOverload::metadata = MetaClass::registerClass<ProblemCapacityOverload>("problem", "overload", true);

  // Initialize the Python type
  PythonType& x = PythonExtension<Problem>::getPythonType();
  x.setName("problem");
  x.setDoc("frePPLe problem");
  x.supportgetattro();
  x.supportstr();
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaCategory*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT bool Problem::operator < (const Problem& a) const
{
  // 1. Sort based on entity
  assert(owner == a.owner);

  // 2. Sort based on type
  if (getType() != a.getType()) return getType() < a.getType();

  // 3. Sort based on start date
  return getDates().getStart() < a.getDates().getStart();
}


DECLARE_EXPORT void Problem::addProblem()
{
  assert(owner);
  if ((owner->firstProblem && *this < *(owner->firstProblem))
      || !owner->firstProblem)
  {
    // Insert as the first problem in the list
    nextProblem = owner->firstProblem;
    owner->firstProblem = this;
  }
  else
  {
    // Insert in the middle or at the end of the list
    Problem* curProblem = owner->firstProblem->nextProblem;
    Problem* prevProblem = owner->firstProblem;
    while (curProblem && !(*this < *curProblem))
    {
      prevProblem = curProblem;
      curProblem = curProblem->nextProblem;
    }
    nextProblem = curProblem;
    prevProblem->nextProblem = this;
  }
}


DECLARE_EXPORT void Problem::removeProblem()
{
  // Fast delete method: the code triggering this method is responsible of
  // maintaining the problem container
  if (!owner) return;

  if (owner->firstProblem == this)
    // Removal from the head of the list
    owner->firstProblem = nextProblem;
  else
  {
    // Removal from the middle of the list
    Problem *prev = owner->firstProblem;
    for (Problem* cur = owner->firstProblem; cur; cur=cur->nextProblem)
    {
      if (cur == this)
      {
        // Found it!
        prev->nextProblem = nextProblem;
        return;
      }
      prev = cur;
    }
    // The problem wasn't found in the list. This shouldn't happen...
    throw LogicException("Corrupted problem list");
  }
}


DECLARE_EXPORT void Plannable::setDetectProblems(bool b)
{
  if (useProblemDetection && !b)
    // We are switching from 'yes' to 'no': delete all existing problems
    Problem::clearProblems(*this);
  else if (!useProblemDetection && b)
    // We are switching from 'no' to 'yes': mark as changed for the next
    // problem detection call
    setChanged();
  // Update the flag
  useProblemDetection=b;
}


DECLARE_EXPORT void Plannable::computeProblems()
{
  // Exit immediately if the list is up to date
  if (!anyChange && !computationBusy) return;

  computationBusy = true;
  // Get exclusive access to this function in a multi-threaded environment.
  static Mutex computationbusy;
  {
    ScopeMutexLock l(computationbusy);

    // Another thread may already have computed it while this thread was
    // waiting for the lock
    while (anyChange)
    {
      // Reset to change flag. Note that during the computation the flag
      // could be switched on again by some model change in a different thread.
      anyChange = false;

      // Loop through all entities
      for (HasProblems::EntityIterator i; i!=HasProblems::endEntity(); ++i)
      {
        Plannable *e = i->getEntity();
        if (e->getChanged() && e->getDetectProblems()) i->updateProblems();
      }

      // Mark the entities as unchanged
      for (HasProblems::EntityIterator j; j!=HasProblems::endEntity(); ++j)
      {
        Plannable *e = j->getEntity();
        if (e->getChanged() && e->getDetectProblems()) e->setChanged(false);
      }
    }

    // Unlock the exclusive access to this function
    computationBusy = false;
  }
}


DECLARE_EXPORT void Problem::clearProblems()
{
  // Loop through all entities, and call clearProblems(i)
  for (HasProblems::EntityIterator i = HasProblems::beginEntity();
      i != HasProblems::endEntity(); ++i)
  {
    clearProblems(*i);
    i->getEntity()->setChanged(true);
  }
}


DECLARE_EXPORT void Problem::clearProblems(HasProblems& p, bool setchanged)
{
  // Nothing to do
  if (!p.firstProblem) return;

  // Delete all problems in the list
  for (Problem *cur=p.firstProblem; cur; )
  {
    Problem *del = cur;
    cur = cur->nextProblem;
    del->owner = NULL;
    delete del;
  }
  p.firstProblem = NULL;

  // Mark as changed
  if (setchanged) p.getEntity()->setChanged();
}


DECLARE_EXPORT Problem::iterator HasProblems::getProblems() const
{
  return Problem::iterator(firstProblem);
}


DECLARE_EXPORT HasProblems::EntityIterator::EntityIterator() : type(0)
{
  // Buffer
  bufIter = new Buffer::iterator(Buffer::begin());
  if (*bufIter != Buffer::end()) return;

  // Move on to resource if there are no buffers
  delete bufIter;
  type = 1;
  resIter = new Resource::iterator(Resource::begin());
  if (*resIter != Resource::end()) return;

  // Move on to operationplans if there are no resources either
  delete resIter;
  type = 2;
  operIter = new OperationPlan::iterator(OperationPlan::begin());
  if (*operIter != OperationPlan::end()) return;

  // Move on to demands if there are no operationplans either
  delete operIter;
  type = 3;
  demIter = new Demand::iterator(Demand::begin());
  if (*demIter == Demand::end())
  {
    // There is nothing at all in this model
    delete demIter;
    type = 4;
  }
}


DECLARE_EXPORT HasProblems::EntityIterator& HasProblems::EntityIterator::operator++()
{
  switch (type)
  {
    case 0:
      // Buffer
      if (*bufIter != Buffer::end())
        if (++(*bufIter) != Buffer::end()) return *this;
      ++type;
      delete bufIter;
      resIter = new Resource::iterator(Resource::begin());
      if (*resIter != Resource::end()) return *this;
      // Note: no break statement
    case 1:
      // Resource
      if (*resIter != Resource::end())
        if (++(*resIter) != Resource::end()) return *this;
      ++type;
      delete resIter;
      operIter = new OperationPlan::iterator(OperationPlan::begin());
      if (*operIter != OperationPlan::end()) return *this;
      // Note: no break statement
    case 2:
      // Operationplan
      if (*operIter != OperationPlan::end())
        if (++(*operIter) != OperationPlan::end()) return *this;
      ++type;
      delete operIter;
      demIter = new Demand::iterator(Demand::begin());
      if (*demIter != Demand::end()) return *this;
      // Note: no break statement
    case 3:
      // Demand
      if (*demIter != Demand::end())
        if (++(*demIter) != Demand::end()) return *this;
      // Ended recursing of all entities
      ++type;
      delete demIter;
      demIter = NULL;
      return *this;
  }
  throw LogicException("Unreachable code reached");
}


DECLARE_EXPORT HasProblems::EntityIterator::~EntityIterator()
{
  switch (type)
  {
      // Buffer
    case 0:
      delete bufIter;
      return;
      // Resource
    case 1:
      delete resIter;
      return;
      // Operation
    case 2:
      delete operIter;
      return;
      // Demand
    case 3:
      delete demIter;
      return;
  }
}


DECLARE_EXPORT HasProblems::EntityIterator::EntityIterator(const EntityIterator& o)
{
  // Delete old iterator
  this->~EntityIterator();
  // Populate new values
  type = o.type;
  if (type==0)
    bufIter = new Buffer::iterator(*(o.bufIter));
  else if (type==1)
    resIter = new Resource::iterator(*(o.resIter));
  else if (type==2)
    operIter = new OperationPlan::iterator(*(o.operIter));
  else if (type==3)
    demIter = new Demand::iterator(*(o.demIter));
}


DECLARE_EXPORT HasProblems::EntityIterator&
HasProblems::EntityIterator::operator=(const EntityIterator& o)
{
  // Gracefully handle self assignment
  if (this == &o) return *this;
  // Delete old iterator
  this->~EntityIterator();
  // Populate new values
  type = o.type;
  if (type==0)
    bufIter = new Buffer::iterator(*(o.bufIter));
  else if (type==1)
    resIter = new Resource::iterator(*(o.resIter));
  else if (type==2)
    operIter = new OperationPlan::iterator(*(o.operIter));
  else if (type==3)
    demIter = new Demand::iterator(*(o.demIter));
  return *this;
}


DECLARE_EXPORT bool
HasProblems::EntityIterator::operator != (const EntityIterator& t) const
{
  // Different iterator type, thus always different and return false
  if (type != t.type) return true;

  // Same iterator type, more granular comparison required
  switch (type)
  {
    case 0:
      // Buffer
      return *bufIter != *(t.bufIter);
    case 1:
      // Resource
      return *resIter != *(t.resIter);
    case 2:
      // Operation
      return *operIter != *(t.operIter);
    case 3:
      // Demand
      return *demIter != *(t.demIter);
    default:
      // Always return true for higher type numbers. This should happen only
      // when comparing with the end of list element.
      return false;
  }
}


DECLARE_EXPORT HasProblems& HasProblems::EntityIterator::operator*() const
{
  switch (type)
  {
    case 0:
      // Buffer
      return **bufIter;
    case 1:
      // Resource
      return **resIter;
    case 2:
      // Operation
      return **operIter;
    case 3:
      // Demand
      return **demIter;
    default:
      throw LogicException("Unknown problem entity found");
  }
}


DECLARE_EXPORT HasProblems* HasProblems::EntityIterator::operator->() const
{
  switch (type)
  {
    case 0:
      // Buffer
      return &**bufIter;
    case 1:
      // Resource
      return &**resIter;
    case 2:
      // Operation
      return &**operIter;
    case 3:
      // Demand
      return &**demIter;
    default:
      throw LogicException("Unknown problem entity found");
  }
}


DECLARE_EXPORT HasProblems::EntityIterator HasProblems::beginEntity()
{
  return EntityIterator();
}


DECLARE_EXPORT HasProblems::EntityIterator HasProblems::endEntity()
{
  // Note that we give call a constructor with type 4, in order to allow
  // a fast comparison.
  return EntityIterator(4);
}


DECLARE_EXPORT Problem::iterator& Problem::iterator::operator++()
{
  // Incrementing beyond the end
  if (!iter) return *this;

  // Move to the next problem
  iter = iter->nextProblem;

  // Move to the next entity
  // We need a while loop here because some entities can be without problems
  while (!iter && !owner && eiter!=HasProblems::endEntity())
  {
    ++eiter;
    if (eiter!=HasProblems::endEntity())
      iter = eiter->firstProblem;
  }
  return *this;
}


DECLARE_EXPORT Problem::iterator Problem::begin()
{
  return iterator();
}


DECLARE_EXPORT Problem::iterator Problem::begin(HasProblems* i, bool refresh)
{
  // Null pointer passed, loop through the full list anyway
  if (!i) return begin();

  // Return an iterator for a single entity
  if (refresh)
    i->updateProblems();
  return iterator(i);
}


DECLARE_EXPORT const Problem::iterator Problem::end()
{
  return iterator(static_cast<Problem*>(NULL));
}


DECLARE_EXPORT void Problem::List::clear(Problem *c)
{
  // Unchain the predecessor
  if (c)
  {
    for (Problem *x = first; x; x = x->nextProblem)
      if (x->nextProblem == c)
      {
        x->nextProblem = NULL;
        break;
      }
  }

  // Delete each constraint in the list
  for (Problem *cur = c ? c : first; cur; )
  {
    Problem *del = cur;
    cur = cur->nextProblem;
    del->owner = NULL;
    delete del;
  }

  // Set the header to NULL
  if (!c) first = NULL;
}


DECLARE_EXPORT Problem* Problem::List::push(const MetaClass* m,
    const Object* o, Date st, Date nd, double w)
{
  // Find the end of the list
  Problem* cur = first;
  while (cur && cur->nextProblem && cur->getOwner() != o)
    cur = cur->nextProblem;
  if (cur && cur->getOwner() == o)
    // Duplicate problem: stop here.
    return cur;

  // Create a new problem
  Problem *p;
  if (m == ProblemCapacityOverload::metadata)
    p = new ProblemCapacityOverload(const_cast<Resource*>(dynamic_cast<const Resource*>(o)), st, nd, w, false);
  else if (m == ProblemMaterialShortage::metadata)
    p = new ProblemMaterialShortage(const_cast<Buffer*>(dynamic_cast<const Buffer*>(o)), st, nd, w, false);
  else if (m == ProblemBeforeCurrent::metadata)
    p = new ProblemBeforeCurrent(const_cast<Operation*>(dynamic_cast<const Operation*>(o)), st, nd, w);
  else if (m == ProblemBeforeFence::metadata)
    p = new ProblemBeforeFence(const_cast<Operation*>(dynamic_cast<const Operation*>(o)), st, nd, w);
  else
    throw LogicException("Problem factory can't create this type of problem");

  // Link the problem in the list
  if (cur)
    cur->nextProblem = p;
  else
    first = p;
  return p;
}


DECLARE_EXPORT void Problem::List::pop(Problem *p)
{
  Problem *q = NULL;
  if (p)
  {
    // Skip the problem that was passed as argument
    q = p->nextProblem;
    p->nextProblem = NULL;
  }
  else
  {
    // NULL argument: delete all
    q = first;
    first = NULL;
  }

  // Delete each constraint after the marked one
  while (q)
  {
    Problem *del = q;
    q = q->nextProblem;
    del->owner = NULL;
    delete del;
  }
}


DECLARE_EXPORT Problem* Problem::List::top() const
{
  for (Problem *p = first; p; p = p->nextProblem)
    if (!p->nextProblem) return p;
  return NULL;
}


} // End namespace
