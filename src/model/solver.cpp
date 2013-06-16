/***************************************************************************
 *                                                                         *
 * Copyright (C) 2009 by Johan De Taeye, frePPLe bvba                                    *
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

template<class Solver> DECLARE_EXPORT Tree utils::HasName<Solver>::st;
DECLARE_EXPORT const MetaCategory* Solver::metadata;


int Solver::initialize()
{
  // Initialize the metadata
  metadata = new MetaCategory("solver", "solvers", reader, writer);

  // Initialize the Python class
  FreppleCategory<Solver>::getType().addMethod("solve", solve, METH_NOARGS, "run the solver");
  return FreppleCategory<Solver>::initialize();
}


DECLARE_EXPORT void Solver::writeElement
(XMLOutput *o, const Keyword &tag, mode m) const
{
  // The subclass should have written its own header
  assert(m == NOHEADER);

  // Fields
  if (loglevel) o->writeElement(Tags::tag_loglevel, loglevel);

  // End object
  o->EndObject(tag);
}


DECLARE_EXPORT void Solver::endElement(XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  if (pAttr.isA(Tags::tag_loglevel))
  {
    int i = pElement.getInt();
    if (i<0 || i>USHRT_MAX)
      throw DataException("Invalid log level" + pElement.getString());
    setLogLevel(i);
  }
}


DECLARE_EXPORT PyObject* Solver::getattro(const Attribute& attr)
{ 
  if (attr.isA(Tags::tag_name))
    return PythonObject(getName());
  if (attr.isA(Tags::tag_loglevel))
    return PythonObject(getLogLevel());
  return NULL;
}


DECLARE_EXPORT int Solver::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_name))
    setName(field.getString());
  else if (attr.isA(Tags::tag_loglevel))
    setLogLevel(field.getInt());
  else
    return -1;  // Error
  return 0;  // OK
}


DECLARE_EXPORT PyObject *Solver::solve(PyObject *self, PyObject *args)
{
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    static_cast<Solver*>(self)->solve();
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

} // end namespace
