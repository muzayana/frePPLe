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

DECLARE_EXPORT const MetaCategory* SubOperation::metacategory;
DECLARE_EXPORT const MetaClass* SubOperation::metadata;


int SubOperation::initialize()
{
  // Initialize the metadata
  metacategory = MetaCategory::registerCategory<SubOperation>(
	  "suboperation", "suboperations", MetaCategory::ControllerDefault  // TODO Need controller to find suboperations. Currently can only add
	  );
  metadata = MetaClass::registerClass<SupplierItem>(
    "suboperation", "suboperation", Object::create<SubOperation>, true
  );
  registerFields<SubOperation>(const_cast<MetaCategory*>(metacategory));

  // Initialize the Python class
  PythonType& x = FreppleCategory<SubOperation>::getPythonType();
  x.setName("suboperation");
  x.setDoc("frePPLe suboperation");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaClass*>(SubOperation::metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


int SubOperationIterator::initialize()
{
  // Initialize the type
  PythonType& x = PythonExtension<SubOperationIterator>::getPythonType();
  x.setName("suboperationIterator");
  x.setDoc("frePPLe iterator for suboperations");
  x.supportiter();
  return x.typeReady();
}


DECLARE_EXPORT SubOperation::~SubOperation()
{
  if (owner)
    owner->getSubOperations().remove(this);
  if (oper)
    oper->superoplist.remove(owner);
}


DECLARE_EXPORT void SubOperation::setOwner(Operation* o)
{
  if (o == owner) return;

  // Remove from previous owner
  if (oper && owner)
    oper->removeSuperOperation(owner);
  if (owner)
    owner->getSubOperations().remove(this);

  // Update
  owner = o;

  // Insert at new owner
  if (oper && owner)
    oper->addSuperOperation(owner);
  if (owner)
  {
    Operation::Operationlist::iterator iter = owner->getSubOperations().begin();
    while (iter != owner->getSubOperations().end() && prio >= (*iter)->getPriority())
      ++iter;
    owner->getSubOperations().insert(iter, this);
  }
}


DECLARE_EXPORT void SubOperation::setOperation(Operation* o)
{
  if (o == oper) return;

  // Remove from previous oper
  if (oper && owner)
    oper->removeSuperOperation(owner);

  // Update
  oper = o;

  // Insert at new oper
  if (owner)
    oper->addSuperOperation(owner);
}


DECLARE_EXPORT void SubOperation::setPriority(int pr)
{
  if (prio == pr) return;

  if (pr < 0)
    throw DataException("Suboperation priority must be greater or equal to 0");

  prio = pr;

  if (owner)
  {
    // Maintain the list in order of priority
    owner->getSubOperations().remove(this);
    Operation::Operationlist::iterator iter = owner->getSubOperations().begin();
    while (iter != owner->getSubOperations().end() && prio >= (*iter)->getPriority())
      ++iter;
    owner->getSubOperations().insert(iter, this);
  }
}


PyObject *SubOperationIterator::iternext()
{
  if (iter == oplist.end())
    return NULL;
  PyObject* result = *(iter++);
  Py_INCREF(result);
  return result;
}


PyObject* SubOperation::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Pick up the operation
    PyObject* oper = PyDict_GetItemString(kwds, "operation");
    if (!PyObject_TypeCheck(oper, Operation::metadata->pythonClass))
      throw DataException("field 'operation' of suboperation must be of type operation");

    // Pick up the buffer
    PyObject* owner = PyDict_GetItemString(kwds, "owner");
    if (!PyObject_TypeCheck(owner, Operation::metadata->pythonClass))
      throw DataException("field 'operation' of suboperation must be of type operation");

    // Pick up the type and create the flow
    SubOperation *l = new SubOperation();
    if (oper) l->setOperation(static_cast<Operation*>(oper));
    if (owner) l->setOwner(static_cast<Operation*>(owner));

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
        if (!attr.isA(Tags::operation) && !attr.isA(Tags::owner))
        {
          const MetaFieldBase* fmeta = SubOperation::metacategory->findField(attr.getHash());
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


}