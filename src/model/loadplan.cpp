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

DECLARE_EXPORT const MetaCategory* LoadPlan::metadata;


int LoadPlan::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<LoadPlan>("loadplan", "loadplans");
  registerFields<LoadPlan>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python type
  PythonType& x = FreppleCategory<LoadPlan>::getPythonType();
  x.setName("loadplan");
  x.setDoc("frePPLe loadplan");
  x.supportgetattro();
  x.supportsetattro();
  const_cast<MetaCategory*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT LoadPlan::LoadPlan(OperationPlan *o, const Load *r)
{
  assert(o);
  ld = const_cast<Load*>(r);
  oper = o;
  start_or_end = START;

  // Update the resource field
  res = r->getResource();

  // Add to the operationplan
  nextLoadPlan = NULL;
  if (o->firstloadplan)
  {
    // Append to the end
    LoadPlan *c = o->firstloadplan;
    while (c->nextLoadPlan) c = c->nextLoadPlan;
    c->nextLoadPlan = this;
  }
  else
    // First in the list
    o->firstloadplan = this;

  // Insert in the resource timeline
  getResource()->loadplans.insert(
    this,
    ld->getLoadplanQuantity(this),
    ld->getLoadplanDate(this)
  );

  // Initialize the Python type
  initType(metadata);

  // For continuous resources, create a loadplan to mark
  // the end of the operationplan.
  if (getResource()->getType() != *ResourceBuckets::metadata)
    new LoadPlan(o, r, this);

  // Mark the operation and resource as being changed. This will trigger
  // the recomputation of their problems
  getResource()->setChanged();
  r->getOperation()->setChanged();
}


DECLARE_EXPORT LoadPlan::LoadPlan(OperationPlan *o, const Load *r, LoadPlan *lp)
{
  ld = const_cast<Load*>(r);
  oper = o;
  start_or_end = END;

  // Update the resource field
  res = lp->getResource();

  // Add to the operationplan
  nextLoadPlan = NULL;
  if (o->firstloadplan)
  {
    // Append to the end
    LoadPlan *c = o->firstloadplan;
    while (c->nextLoadPlan) c = c->nextLoadPlan;
    c->nextLoadPlan = this;
  }
  else
    // First in the list
    o->firstloadplan = this;

  // Insert in the resource timeline
  getResource()->loadplans.insert(
    this,
    ld->getLoadplanQuantity(this),
    ld->getLoadplanDate(this)
  );

  // Initialize the Python type
  initType(metadata);
}


DECLARE_EXPORT void LoadPlan::setResource(Resource* newres, bool check)
{
  // Nothing to do
  if (res == newres) return;

  // Validate the argument
  if (!newres) throw DataException("Can't switch to NULL resource");
  if (check)
  {
    // New resource must be a subresource of the load's resource.
    bool ok = false;
    for (const Resource* i = newres; i && !ok; i = i->getOwner())
      if (i == getLoad()->getResource()) ok = true;
    if (!ok)
      throw DataException("Resource isn't matching the resource specified on the load");

    // New resource must have the required skill
    if (getLoad()->getSkill())
    {
      ok = false;
      for(Resource::skilllist::const_iterator s = newres->getSkills().begin();
        s != newres->getSkills().end() && !ok; s++)
        if (s->getSkill() == getLoad()->getSkill()) ok = true;
      if (!ok)
        throw DataException("Resource misses the skill specified on the load");
    }
  }

  // Mark entities as changed
  if (oper) oper->getOperation()->setChanged();
  if (res && res!=newres) res->setChanged();
  newres->setChanged();

  // Update also the setup operationplans
  if (oper && oper->getOperation() != OperationSetup::setupoperation)
  {
    bool oldHasSetup = ld && !ld->getSetup().empty()  // TODO not fully correct. If the load is changed, it is still possible that the old load had a setup, while ld doesn't have one any more...
        && res && res->getSetupMatrix();
    bool newHasSetup = ld && !ld->getSetup().empty()
        && newres->getSetupMatrix();
    OperationPlan *setupOpplan = NULL;
    if (oldHasSetup)
    {
      for (OperationPlan::iterator i(oper); i != oper->end(); ++i)
        if (i->getOperation() == OperationSetup::setupoperation)
        {
          setupOpplan = &*i;
          break;
        }
      if (!setupOpplan) oldHasSetup = false;
    }
    if (oldHasSetup)
    {
      if (newHasSetup)
      {
        // Case 1: Both the old and new load require a setup
        LoadPlan *setupLdplan = NULL;
        for (OperationPlan::LoadPlanIterator j = setupOpplan->beginLoadPlans();
            j != setupOpplan->endLoadPlans(); ++j)
          if (j->getLoad() == ld)
          {
            setupLdplan = &*j;
            break;
          }
        if (!setupLdplan)
          throw LogicException("Can't find loadplan on setup operationplan");
        // Update the loadplan
        setupOpplan->setEnd(setupOpplan->getDates().getEnd());
      }
      else
      {
        // Case 2: Delete the old setup which is not required any more
        oper->eraseSubOperationPlan(setupOpplan);
      }
    }
    else
    {
      if (newHasSetup)
      {
        // Case 3: Create a new setup operationplan
        OperationSetup::setupoperation->createOperationPlan(
          1, Date::infinitePast, oper->getDates().getEnd(), NULL, oper);
      }
      //else:
      // Case 4: No setup for the old or new load
    }
  }

  // Find the loadplan before the setup
  LoadPlan *prevldplan = NULL;
  if (getOperationPlan()->getOperation() == OperationSetup::setupoperation)
  {
    for (TimeLine<LoadPlan>::const_iterator i = getResource()->getLoadPlans().begin(isStart() ? getOtherLoadPlan() : this);
        i != getResource()->getLoadPlans().end(); --i)
    {
      const LoadPlan *l = dynamic_cast<const LoadPlan*>(&*i);
      if (l && l->getOperationPlan() != getOperationPlan()
          && l->getOperationPlan() != getOperationPlan()->getOwner()
          && !l->isStart())
      {
        prevldplan = const_cast<LoadPlan*>(l);
        break;
      }
    }
    if (!prevldplan)
    {
      for (TimeLine<LoadPlan>::const_iterator i = getResource()->getLoadPlans().begin(isStart() ? getOtherLoadPlan() : this);
          i != getResource()->getLoadPlans().end(); ++i)
      {
        const LoadPlan *l = dynamic_cast<const LoadPlan*>(&*i);
        if (l && l->getOperationPlan() != getOperationPlan()
            && l->getOperationPlan() != getOperationPlan()->getOwner()
            && !l->isStart())
        {
          prevldplan = const_cast<LoadPlan*>(l);
          break;
        }
      }
    }
  }

  // Change this loadplan and its brother
  for (LoadPlan *ldplan = getOtherLoadPlan(); true; )
  {
    // Remove from the old resource, if there is one
    if (res)
    {
      res->loadplans.erase(ldplan);
      res->setChanged();
    }

    // Insert in the new resource.
    // This code assumes the date and quantity of the loadplan don't change
    // when a new resource is assigned.
    ldplan->res = newres;
    newres->loadplans.insert(
      ldplan,
      ld->getLoadplanQuantity(ldplan),
      ld->getLoadplanDate(ldplan)
    );

    // Repeat for the brother loadplan or exit
    if (ldplan != this) ldplan = this;
    else break;
  }

  // Update the setups on the old resource
  if (prevldplan) prevldplan->res->updateSetups(prevldplan);

  // Change the resource
  newres->setChanged();
}


DECLARE_EXPORT LoadPlan* LoadPlan::getOtherLoadPlan() const
{
  for (LoadPlan *i = oper->firstloadplan; i; i = i->nextLoadPlan)
    if (i->ld == ld && i != this) return i;
  throw LogicException("No matching loadplan found");
}


DECLARE_EXPORT void LoadPlan::update()
{
  // Update the timeline data structure
  getResource()->getLoadPlans().update(
    this,
    ld->getLoadplanQuantity(this),
    ld->getLoadplanDate(this)
  );

  // Review adjacent setups
  if (!isStart()) getResource()->updateSetups(this);

  // Mark the operation and resource as being changed. This will trigger
  // the recomputation of their problems
  getResource()->setChanged();
  ld->getOperation()->setChanged();
}


DECLARE_EXPORT string LoadPlan::getSetup(bool current) const
{
  // This resource has no setupmatrix
  static string nosetup;
  assert(ld);
  if (!getResource()->getSetupMatrix()) return nosetup;

  // Current load has a setup
  if (!ld->getSetup().empty() && current) return ld->getSetup();

  // Scan earlier setups
  for (Resource::loadplanlist::const_iterator i(this);
      i != getResource()->getLoadPlans().end(); --i)
  {
    const LoadPlan* j = dynamic_cast<const LoadPlan*>(&*i);
    if (j && !j->getLoad()->getSetup().empty() && (current || j != this))
      return j->getLoad()->getSetup();
  }

  // No conversions found - return the original setup
  return getResource()->getSetup();
}


DECLARE_EXPORT LoadPlan::~LoadPlan()
{
  getResource()->setChanged();
  LoadPlan *prevldplan = NULL;
  if (!isStart() && oper->getOperation() == OperationSetup::setupoperation)
  {
    for (TimeLine<LoadPlan>::const_iterator i = getResource()->getLoadPlans().begin(isStart() ? getOtherLoadPlan() : this);
        i != getResource()->getLoadPlans().end(); --i)
    {
      const LoadPlan *l = dynamic_cast<const LoadPlan*>(&*i);
      if (l && l->getOperationPlan() != getOperationPlan()
          && l->getOperationPlan() != getOperationPlan()->getOwner()
          && !l->isStart())
      {
        prevldplan = const_cast<LoadPlan*>(l);
        break;
      }
    }
    if (!prevldplan)
    {
      for (TimeLine<LoadPlan>::const_iterator i = getResource()->getLoadPlans().begin(isStart() ? getOtherLoadPlan() : this);
          i != getResource()->getLoadPlans().end(); ++i)
      {
        const LoadPlan *l = dynamic_cast<const LoadPlan*>(&*i);
        if (l && l->getOperationPlan() != getOperationPlan()
            && l->getOperationPlan() != getOperationPlan()->getOwner()
            && !l->isStart())
        {
          prevldplan = const_cast<LoadPlan*>(l);
          break;
        }
      }
    }
  }
  getResource()->loadplans.erase(this);
  if (prevldplan) getResource()->updateSetups(prevldplan);
}


DECLARE_EXPORT void LoadPlan::setLoad(Load* newld)
{
  // No change
  if (newld == ld) return;

  // Verify the data
  if (!newld) throw DataException("Can't switch to NULL load");
  if (ld && ld->getOperation() != newld->getOperation())
    throw DataException("Only switching to a load on the same operation is allowed");

  // Update the load and resource fields
  LoadPlan* o = getOtherLoadPlan();
  if (o) o->ld = newld;
  ld = newld;
  setResource(newld->getResource());
}


int LoadPlanIterator::initialize()
{
  // Initialize the type
  PythonType& x = PythonExtension<LoadPlanIterator>::getPythonType();
  x.setName("loadplanIterator");
  x.setDoc("frePPLe iterator for loadplan");
  x.supportiter();
  return x.typeReady();
}


PyObject* LoadPlanIterator::iternext()
{
  LoadPlan* ld;
  if (resource_or_opplan)
  {
    // Skip zero quantity loadplans
    while (*resiter != res->getLoadPlans().end() && (*resiter)->getQuantity()==0.0)
      ++(*resiter);
    if (*resiter == res->getLoadPlans().end()) return NULL;

    // Return result
    ld = const_cast<LoadPlan*>(static_cast<const LoadPlan*>(&*((*resiter)++)));
  }
  else
  {
    while (*opplaniter != opplan->endLoadPlans() && (*opplaniter)->getQuantity()==0.0)
      ++(*opplaniter);
    if (*opplaniter == opplan->endLoadPlans()) return NULL;
    ld = static_cast<LoadPlan*>(&*((*opplaniter)++));
  }
  Py_INCREF(ld);
  return const_cast<LoadPlan*>(ld);
}

} // end namespace
