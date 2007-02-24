/***************************************************************************
  file : $URL$
  version : $LastChangedRevision$  $LastChangedBy$
  date : $LastChangedDate$
  email : jdetaeye@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007 by Johan De Taeye                                    *
 *                                                                         *
 * This library is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation; either version 2.1 of the License, or  *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This library is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with this library; if not, write to the Free Software     *
 * Foundation Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA *
 *                                                                         *
 ***************************************************************************/

#include "embeddedpython.h"

namespace module_python
{

// Type information of our Python extensions
PyTypeObject PythonProblem::InfoType;
PyTypeObject PythonOperationPlan::InfoType;
PyTypeObject PythonDemand::InfoType;
PyTypeObject PythonBuffer::InfoType;
PyTypeObject PythonResource::InfoType;


//
// INTERFACE FOR PROBLEM
//


extern "C" PyObject* PythonProblem::create(PyTypeObject* type, PyObject *args, PyObject *kwargs)
{
  // Allocate memory
  PythonProblem* obj = PyObject_New(PythonProblem, &PythonProblem::InfoType);

  // Initialize the problem iterator
  obj->iter.reset();
  obj->iter = Problem::begin();

  return reinterpret_cast<PyObject*>(obj);
}


extern "C" PyObject* PythonProblem::next(PythonProblem* obj) 
{
  if (obj->iter != Problem::end())
  {
    PyObject* result = Py_BuildValue("{s:s,s:s,s:N,s:N}",
      "DESCRIPTION", obj->iter->getDescription().c_str(),
      "TYPE", obj->iter->getType().type.c_str(),
      "START", PythonDateTime(obj->iter->getDateRange().getStart()),
      "END", PythonDateTime(obj->iter->getDateRange().getEnd())
      ); 

    ++(obj->iter);
    return result;
  }
  else 
    // Reached the end of the iteration
    return NULL;
}


// 
// INTERFACE FOR OPERATIONPLAN
//


extern "C" PyObject* PythonOperationPlan::create(PyTypeObject* type, PyObject *args, PyObject *kwargs)
{
  // Allocate memory
  PythonOperationPlan* obj = PyObject_New(PythonOperationPlan, &PythonOperationPlan::InfoType);

  // Initialize the iterator
  obj->iter = OperationPlan::begin();

  return reinterpret_cast<PyObject*>(obj);
}


extern "C" PyObject* PythonOperationPlan::next(PythonOperationPlan* obj) 
{
  if (obj->iter != OperationPlan::end())
  {
    PyObject* result = Py_BuildValue("{s:l,s:s,s:f,s:N,s:N}",
      "IDENTIFIER", obj->iter->getIdentifier(),
      "OPERATION", obj->iter->getOperation()->getName().c_str(),
      "QUANTITY", obj->iter->getQuantity(),
      "START", PythonDateTime(obj->iter->getDates().getStart()),
      "END", PythonDateTime(obj->iter->getDates().getEnd())
      );
    ++(obj->iter);
    return result;
  }
  // Reached the end of the iteration
  return NULL;
}


//
// INTERFACE FOR DEMAND
//

extern "C" PyObject* PythonDemand::create(PyTypeObject* type, PyObject *args, PyObject *kwargs)
{
  // Allocate memory
  PythonDemand* obj = PyObject_New(PythonDemand, &PythonDemand::InfoType);

  // Initialize the iterator
  obj->iter = Demand::begin();

  return reinterpret_cast<PyObject*>(obj);
}


extern "C" PyObject* PythonDemand::next(PythonDemand* obj) 
{
  if (obj->iter != Demand::end())
  {
    PyObject* result = Py_BuildValue("{s:s,s:f,s:N,s:d,s:z,s:z,s:z}",
      "NAME", obj->iter->getName().c_str(),
      "QUANTITY", obj->iter->getQuantity(),
			"DUE", PythonDateTime(obj->iter->getDue()),
      "PRIORITY", obj->iter->getPriority(),
      "ITEM", obj->iter->getItem() ? obj->iter->getItem()->getName().c_str() : NULL,
      "OPERATION", obj->iter->getOperation() ? obj->iter->getOperation()->getName().c_str() : NULL,
      "OWNER", obj->iter->getOwner() ? obj->iter->getOwner()->getName().c_str() : NULL
      //xxx "CUSTOMER", obj->iter->getCustomer() ? obj->iter->getCustomer()->getName().c_str() : NULL     
      );
    ++(obj->iter);
    return result;
  }
  // Reached the end of the iteration
  return NULL;
}


//
// INTERFACE FOR BUFFER
//


extern "C" PyObject* PythonBuffer::create(PyTypeObject* type, PyObject *args, PyObject *kwargs)
{
  // Allocate memory
  PythonBuffer* obj = PyObject_New(PythonBuffer, &PythonBuffer::InfoType);

  // Initialize the iterator
  obj->iter = Buffer::begin();

  return reinterpret_cast<PyObject*>(obj);
}


extern "C" PyObject* PythonBuffer::next(PythonBuffer* obj) 
{
  if (obj->iter != Buffer::end())
  {
    PyObject* result = Py_BuildValue("{s:s,s:s,s:s,s:s,s:f,s:z,s:z,s:z,s:z,s:z,s:z}",
      "NAME", obj->iter->getName().c_str(),
      "CATEGORY", obj->iter->getCategory().c_str(),
      "SUBCATEGORY", obj->iter->getSubCategory().c_str(),
      "DESCRIPTION", obj->iter->getDescription().c_str(),
      "ONHAND", obj->iter->getOnHand(),
      "LOCATION", obj->iter->getLocation() ? obj->iter->getLocation()->getName().c_str() : NULL,
      "ITEM", obj->iter->getItem() ? obj->iter->getItem()->getName().c_str() : NULL,
      "MINIMUM", obj->iter->getMinimum() ? obj->iter->getMinimum()->getName().c_str() : NULL,
      "MAXIMUM", obj->iter->getMaximum() ? obj->iter->getMaximum()->getName().c_str() : NULL,
      "CONSUMING", obj->iter->getConsumingOperation() ? obj->iter->getConsumingOperation()->getName().c_str() : NULL,
      "PRODUCING", obj->iter->getProducingOperation() ? obj->iter->getProducingOperation()->getName().c_str() : NULL
      );
    ++(obj->iter);
    return result;
  }
  // Reached the end of the iteration
  return NULL;
}


//
// INTERFACE FOR RESOURCE
//


extern "C" PyObject* PythonResource::create(PyTypeObject* type, PyObject *args, PyObject *kwargs)
{
  // Allocate memory
  PythonResource* obj = PyObject_New(PythonResource, &PythonResource::InfoType);

  // Initialize the iterator
  obj->iter = Resource::begin();

  return reinterpret_cast<PyObject*>(obj);
}


extern "C" PyObject* PythonResource::next(PythonResource* obj) 
{
  if (obj->iter != Resource::end())
  {
    PyObject* result = Py_BuildValue("{s:s,s:s,s:s,s:s,s:z,s:z,s:z}",
      "NAME", obj->iter->getName().c_str(),
      "CATEGORY", obj->iter->getCategory().c_str(),
      "SUBCATEGORY", obj->iter->getSubCategory().c_str(),
      "DESCRIPTION", obj->iter->getDescription().c_str(),
      "LOCATION", obj->iter->getLocation() ? obj->iter->getLocation()->getName().c_str() : NULL,
      "MAXIMUM", obj->iter->getMaximum() ? obj->iter->getMaximum()->getName().c_str() : NULL,
      "OWNER", obj->iter->getOwner() ? obj->iter->getOwner()->getName().c_str() : NULL
      );
    ++(obj->iter);
    return result;
  }
  // Reached the end of the iteration
  return NULL;
}


} // End namespace
