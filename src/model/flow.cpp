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

DECLARE_EXPORT const MetaCategory* Flow::metadata;
DECLARE_EXPORT const MetaClass* FlowStart::metadata;
DECLARE_EXPORT const MetaClass* FlowEnd::metadata;
DECLARE_EXPORT const MetaClass* FlowFixedStart::metadata;
DECLARE_EXPORT const MetaClass* FlowFixedEnd::metadata;


int Flow::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<Flow>(
    "flow", "flows",
    Association<Operation,Buffer,Flow>::reader, finder
    );
  registerFields<Flow>(const_cast<MetaCategory*>(metadata));
  FlowStart::metadata = MetaClass::registerClass<FlowStart>(
    "flow", "flow_start", Object::create<FlowStart>, true
    );
  FlowEnd::metadata = MetaClass::registerClass<FlowEnd>(
    "flow", "flow_end", Object::create<FlowEnd>
    );
  FlowFixedStart::metadata = MetaClass::registerClass<FlowFixedStart>(
    "flow", "flow_fixed_start", Object::create<FlowFixedStart>
    );
  FlowFixedEnd::metadata = MetaClass::registerClass<FlowFixedEnd>(
    "flow", "flow_fixed_end", Object::create<FlowFixedEnd>
    );

  // Initialize the type
  PythonType& x = FreppleCategory<Flow>::getPythonType();
  x.setName("flow");
  x.setDoc("frePPLe flow");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaCategory*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT Flow::~Flow()
{
  // Set a flag to make sure the level computation is triggered again
  HasLevel::triggerLazyRecomputation();

  // Delete existing flowplans
  if (getOperation() && getBuffer())
  {
    // Loop over operationplans
    for(OperationPlan::iterator i(getOperation()); i != OperationPlan::end(); ++i)
      // Loop over flowplans
      for(OperationPlan::FlowPlanIterator j = i->beginFlowPlans(); j != i->endFlowPlans(); )
        if (j->getFlow() == this) j.deleteFlowPlan();
        else ++j;
  }

  // Delete the flow from the operation and the buffer
  if (getOperation())
    getOperation()->flowdata.erase(this);
  if (getBuffer())
    getBuffer()->flows.erase(this);
}


PyObject* Flow::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Pick up the operation
    PyObject* oper = PyDict_GetItemString(kwds, "operation");
    if (!oper)
      throw DataException("missing operation on Flow");
    if (!PyObject_TypeCheck(oper, Operation::metadata->pythonClass))
      throw DataException("flow operation must be of type operation");

    // Pick up the buffer
    PyObject* buf = PyDict_GetItemString(kwds, "buffer");
    if (!buf)
      throw DataException("missing buffer on Flow");
    if (!PyObject_TypeCheck(buf, Buffer::metadata->pythonClass))
      throw DataException("flow buffer must be of type buffer");

    // Pick up the quantity
    PyObject* q1 = PyDict_GetItemString(kwds, "quantity");
    double q2 = q1 ? PythonData(q1).getDouble() : 1.0;

    // Pick up the effectivity dates
    DateRange eff;
    PyObject* eff_start = PyDict_GetItemString(kwds, "effective_start");
    if (eff_start)
    {
      PythonData d(eff_start);
      eff.setStart(d.getDate());
    }
    PyObject* eff_end = PyDict_GetItemString(kwds, "effective_end");
    if (eff_end)
    {
      PythonData d(eff_end);
      eff.setEnd(d.getDate());
    }

    // Pick up the type and create the flow
    Flow *l;
    PyObject* t = PyDict_GetItemString(kwds, "type");
    if (t)
    {
      PythonData d(t);
      if (d.getString() == "flow_end")
        l = new FlowEnd(
          static_cast<Operation*>(oper),
          static_cast<Buffer*>(buf),
          q2
        );
      else if (d.getString() == "flow_fixed_end")
        l = new FlowFixedEnd(
          static_cast<Operation*>(oper),
          static_cast<Buffer*>(buf),
          q2
        );
      else if (d.getString() == "flow_fixed_start")
        l = new FlowFixedStart(
          static_cast<Operation*>(oper),
          static_cast<Buffer*>(buf),
          q2
        );
      else
        l = new FlowStart(
          static_cast<Operation*>(oper),
          static_cast<Buffer*>(buf),
          q2
        );
    }
    else
      l = new FlowStart(
        static_cast<Operation*>(oper),
        static_cast<Buffer*>(buf),
        q2
      );

    // Iterate over extra keywords, and set attributes.   @todo move this responsibility to the readers...
    if (l)
    {
      l->setEffective(eff);
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(kwds, &pos, &key, &value))
      {
        PythonData field(value);
        PyObject* key_utf8 = PyUnicode_AsUTF8String(key);
        DataKeyword attr(PyBytes_AsString(key_utf8));
        Py_DECREF(key_utf8);
        if (!attr.isA(Tags::effective_end) && !attr.isA(Tags::effective_start)
          && !attr.isA(Tags::operation) && !attr.isA(Tags::buffer)
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
            l->setProperty(attr.getName(), value);
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


DECLARE_EXPORT Object* Flow::finder(const DataValueDict& d)
{
  // Check operation
  const DataValue* tmp = d.get(Tags::operation);
  if (!tmp)
    return NULL;
  Operation* oper = static_cast<Operation*>(tmp->getObject());

  // Check buffer field
  tmp = d.get(Tags::buffer);
  if (!tmp)
    return NULL;
  Buffer* buf = static_cast<Buffer*>(tmp->getObject());

  // Walk over all flows of the operation, and return
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
  for (Operation::flowlist::const_iterator fl = oper->getFlows().begin();
    fl != oper->getFlows().end(); ++fl)
  {
    if (fl->getBuffer() != buf)
      continue;
    if (hasEffectiveStart && fl->getEffectiveStart() != effective_start)
      continue;
    if (hasEffectiveEnd && fl->getEffectiveEnd() != effective_end)
      continue;
    if (hasPriority && fl->getPriority() != priority)
      continue;
    if (hasName && fl->getName() != name)
      continue;
    return const_cast<Flow*>(&*fl);
  }
  return NULL;
}

} // end namespace
