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
  metadata = MetaCategory::registerCategory<Load>(
    "load", "loads",
    Association<Operation,Resource,Load>::reader, finder
    );
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
  if (getOperation())
    getOperation()->loaddata.erase(this);
  if (getResource())
    getResource()->loads.erase(this);
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
      if (&*i != this && !i->setup.empty() && i->getName() != getName())
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
      if (&*i != this && !i->setup.empty() && i->getName() != getName())
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
    if (!oper)
      throw DataException("missing operation on Load");
    if (!PyObject_TypeCheck(oper, Operation::metadata->pythonClass))
      throw DataException("load operation must be of type operation");

    // Pick up the resource
    PyObject* res = PyDict_GetItemString(kwds,"resource");
    if (!res)
      throw DataException("missing resource on Load");
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
            l->setProperty(attr.getName(), value);;
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


DECLARE_EXPORT Object* Load::finder(const DataValueDict& d)
{
  // Check operation
  const DataValue* tmp = d.get(Tags::operation);
  if (!tmp)
    return NULL;
  Operation* oper = static_cast<Operation*>(tmp->getObject());

  // Check resource field
  tmp = d.get(Tags::resource);
  if (!tmp)
    return NULL;
  Resource* res = static_cast<Resource*>(tmp->getObject());

  // Walk over all loads of the operation, and return
  // the first one with matching
  const DataValue* hasEffectiveStart = d.get(Tags::effective_start);
  Date effective_start;
  if (hasEffectiveStart)
    effective_start = hasEffectiveStart->getDate();
  const DataValue* hasEffectiveEnd = d.get(Tags::effective_end);
  Date effective_end;
  if (hasEffectiveEnd)
    effective_end = hasEffectiveEnd->getDate();
  const DataValue* hasPriority = d.get(Tags::priority);
  int priority;
  if (hasPriority)
    priority = hasPriority->getInt();
  const DataValue* hasName = d.get(Tags::name);
  string name;
  if (hasName)
    name = hasName->getString();
  for (Operation::loadlist::const_iterator fl = oper->getLoads().begin();
    fl != oper->getLoads().end(); ++fl)
  {
    if (fl->getResource() != res)
      continue;
    if (hasEffectiveStart && fl->getEffectiveStart() != effective_start)
      continue;
    if (hasEffectiveEnd && fl->getEffectiveEnd() != effective_end)
      continue;
    if (hasPriority && fl->getPriority() != priority)
      continue;
    if (hasName && fl->getName() != name)
      continue;
    return const_cast<Load*>(&*fl);
  }
  return NULL;
}

} // end namespace
