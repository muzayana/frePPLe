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

DECLARE_EXPORT const MetaCategory* Load::metadata;
DECLARE_EXPORT const MetaClass* LoadDefault::metadata;


int Load::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<Load>("load", "loads", MetaCategory::ControllerDefault);
  registerFields<Load>(const_cast<MetaCategory*>(metadata));
  LoadDefault::metadata = MetaClass::registerClass<LoadDefault>(
    "load", "load", Object::create<LoadDefault>, true
    );

  // Initialize the Python class
  PythonType& x = FreppleCategory<Load>::getPythonType();
  x.setName("load");
  x.setDoc("frePPLe load");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaCategory*>(Load::metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT void Load::validate(Action action)
{
  // Catch null operation and resource pointers
  Operation *oper = getOperation();
  Resource *res = getResource();
  if (!oper || !res)
  {
    // Invalid load model
    if (!oper && !res)
      throw DataException("Missing operation and resource on a load");
    else if (!oper)
      throw DataException("Missing operation on a load on resource '"
          + res->getName() + "'");
    else if (!res)
      throw DataException("Missing resource on a load on operation '"
          + oper->getName() + "'");
  }

  // Check if a load with 1) identical resource, 2) identical operation and
  // 3) overlapping effectivity dates already exists
  Operation::loadlist::const_iterator i = oper->getLoads().begin();
  for (; i != oper->getLoads().end(); ++i)
    if (i->getResource() == res
        && i->getEffective().overlap(getEffective())
        && &*i != this)
      break;

  // Apply the appropriate action
  switch (action)
  {
    case ADD:
      if (i != oper->getLoads().end())
      {
        throw DataException("Load of '" + oper->getName() + "' and '"
            + res->getName() + "' already exists");
      }
      break;
    case CHANGE:
      throw DataException("Can't update a load");
    case ADD_CHANGE:
      // ADD is handled in the code after the switch statement
      if (i == oper->getLoads().end()) break;
      throw DataException("Can't update a load");
    case REMOVE:
      // This load was only used temporarily during the reading process
      delete this;
      if (i == oper->getLoads().end())
        // Nothing to delete
        throw DataException("Can't remove nonexistent load of '"
            + oper->getName() + "' and '" + res->getName() + "'");
      delete &*i;
      // Set a flag to make sure the level computation is triggered again
      HasLevel::triggerLazyRecomputation();
      return;
  }

  // The statements below should be executed only when a new load is created.

  // Set a flag to make sure the level computation is triggered again
  HasLevel::triggerLazyRecomputation();
}


DECLARE_EXPORT Load::~Load()
{
  // Set a flag to make sure the level computation is triggered again
  HasLevel::triggerLazyRecomputation();

  // Delete existing loadplans
  if (getOperation() && getResource())
  {
    // Loop over operationplans
    for(OperationPlan::iterator i(getOperation()); i != OperationPlan::end(); ++i)
      // Loop over loadplans
      for(OperationPlan::LoadPlanIterator j = i->beginLoadPlans(); j != i->endLoadPlans(); )
        if (j->getLoad() == this) j.deleteLoadPlan();
        else ++j;
  }

  // Delete the load from the operation and resource
  if (getOperation()) getOperation()->loaddata.erase(this);
  if (getResource()) getResource()->loads.erase(this);

  // Clean up alternate loads
  if (hasAlts)
  {
    // The load has alternates.
    // Make a new load the leading one. Or if there is only one alternate
    // present it is not marked as an alternate any more.
    unsigned short cnt = 0;
    int minprio = INT_MAX;
    Load* newLeader = NULL;
    for (Operation::loadlist::iterator i = getOperation()->loaddata.begin();
        i != getOperation()->loaddata.end(); ++i)
      if (i->altLoad == this)
      {
        cnt++;
        if (i->priority < minprio)
        {
          newLeader = &*i;
          minprio = i->priority;
        }
      }
    if (cnt < 1)
      throw LogicException("Alternate loads update failure");
    else if (cnt == 1)
      // No longer an alternate any more
      newLeader->altLoad = NULL;
    else
    {
      // Mark a new leader load
      newLeader->hasAlts = true;
      newLeader->altLoad = NULL;
      for (Operation::loadlist::iterator i = getOperation()->loaddata.begin();
          i != getOperation()->loaddata.end(); ++i)
        if (i->altLoad == this) i->altLoad = newLeader;
    }
  }
  if (altLoad)
  {
    // The load is an alternate of another one.
    // If it was the only alternate, then the hasAlts flag on the parent
    // load needs to be set back to false
    bool only_one = true;
    for (Operation::loadlist::iterator i = getOperation()->loaddata.begin();
        i != getOperation()->loaddata.end(); ++i)
      if (i->altLoad == altLoad)
      {
        only_one = false;
        break;
      }
    if (only_one) altLoad->hasAlts = false;
  }
}


DECLARE_EXPORT void Load::setAlternate(Load *f)
{
  // Can't be an alternate to oneself.
  // No need to flag as an exception.
  if (f == this) return;

  // Validate the argument
  if (!f)
    throw DataException("Setting NULL alternate load");
  if (hasAlts || f->altLoad)
    throw DataException("Nested alternate loads are not allowed");

  // Update both flows
  f->hasAlts = true;
  altLoad = f;
}


DECLARE_EXPORT void Load::setAlternateName(const string& n)
{
  if (!getOperation())
    throw LogicException("Can't set an alternate load before setting the operation");
  Load *x = getOperation()->loaddata.find(n);
  if (!x) throw DataException("Can't find load with name '" + n + "'");
  setAlternate(x);
}


DECLARE_EXPORT void Load::setOperation(Operation* o)
{
  // Validate the input
  if (!setup.empty() && o)
  {
    // Guarantuee that only a single load has a setup.
    // Alternates of that load can have a setup as well.
    for (Operation::loadlist::iterator i = o->loaddata.begin();
        i != o->loaddata.end(); ++i)
      if (&*i != this && !i->setup.empty()
          && i->getAlternate() != this && getAlternate() != &*i
          && i->getAlternate() != getAlternate())
        throw DataException("Only a single load of an operation can specify a setup");
  }

  // Update the field
  if (o)
    setPtrA(o,o->getLoads());
}


DECLARE_EXPORT void Load::setSetup(const string& n)
{
  // Validate the input
  if (!n.empty() && getOperation())
  {
    // Guarantuee that only a single load has a setup.
    // Alternates of that load can have a setup as well.
    for (Operation::loadlist::iterator i = getOperation()->loaddata.begin();
        i != getOperation()->loaddata.end(); ++i)
      if (&*i != this && !i->setup.empty()
          && i->getAlternate() != this && getAlternate() != &*i
          && i->getAlternate() != getAlternate())
        throw DataException("Only a single load of an operation can specify a setup");
  }

  // Update the field
  setup = n;
}


PyObject* Load::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Pick up the operation
    PyObject* oper = PyDict_GetItemString(kwds,"operation");
    if (!PyObject_TypeCheck(oper, Operation::metadata->pythonClass))
      throw DataException("load operation must be of type operation");

    // Pick up the resource
    PyObject* res = PyDict_GetItemString(kwds,"resource");
    if (!PyObject_TypeCheck(res, Resource::metadata->pythonClass))
      throw DataException("load resource must be of type resource");

    // Pick up the quantity
    PyObject* q1 = PyDict_GetItemString(kwds,"quantity");
    double q2 = q1 ? PythonData(q1).getDouble() : 1.0;

    // Pick up the effective dates
    DateRange eff;
    PyObject* eff_start = PyDict_GetItemString(kwds,"effective_start");
    if (eff_start)
    {
      PythonData d(eff_start);
      eff.setStart(d.getDate());
    }
    PyObject* eff_end = PyDict_GetItemString(kwds,"effective_end");
    if (eff_end)
    {
      PythonData d(eff_end);
      eff.setEnd(d.getDate());
    }

    // Create the load
    Load *l = new LoadDefault(
      static_cast<Operation*>(oper),
      static_cast<Resource*>(res),
      q2, eff
    );

    // Iterate over extra keywords, and set attributes.   @todo move this responsibility to the readers...
    if (l)
    {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(kwds, &pos, &key, &value))
      {
        PythonData field(value);
        PyObject* key_utf8 = PyUnicode_AsUTF8String(key);
        DataKeyword attr(PyBytes_AsString(key_utf8));
        Py_DECREF(key_utf8);
        if (!attr.isA(Tags::effective_end) && !attr.isA(Tags::effective_start)
          && !attr.isA(Tags::operation) && !attr.isA(Tags::resource)
          && !attr.isA(Tags::quantity) && !attr.isA(Tags::type)
          && !attr.isA(Tags::action))
        {
          const MetaFieldBase* fmeta = l->getType().findField(attr.getHash());
          if (!fmeta && l->getType().category)
            fmeta = l->getType().category->findField(attr.getHash());
          if (fmeta)
            // Update the attribute
            fmeta->setField(l, field);
          else
            PyErr_Format(PyExc_AttributeError,
                "attribute '%S' on '%s' can't be updated",
                key, Py_TYPE(l)->tp_name);
        }
      };
    }

    // Return the object
    Py_INCREF(l);
    return static_cast<PyObject*>(l);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


} // end namespace
