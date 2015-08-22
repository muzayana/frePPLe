/***************************************************************************
 *                                                                         *
 * Copyright (C) 2015 by frePPLe bvba                                      *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "inventoryplanning.h"

namespace module_inventoryplanning
{

const MetaClass *InventoryPlanningSolver::metadata;


MODULE_EXPORT const char* initialize(const Environment::ParameterList& z)
{
  // Initialize only once
  static bool init = false;
  static const char* name = "inventoryplanning";
  if (init)
  {
    logger << "Warning: Initializing module inventoryplanning more than once." << endl;
    return name;
  }
  init = true;

  // Verify you have an enterprise license.
  // This specific value of the flag is when the customer name is "Community Edition users".
  if (flags == 269264) return "";

  // Register the Python extensions
  PyGILState_STATE state = PyGILState_Ensure();
  try
  {
    // Register new Python data types
    if (InventoryPlanningSolver::initialize())
      throw RuntimeException("Error registering inventoryplanningsolver");
    PyGILState_Release(state);
  }
  catch (const exception &e)
  {
    PyGILState_Release(state);
    logger << "Error: " << e.what() << endl;
  }
  catch (...)
  {
    PyGILState_Release(state);
    logger << "Error: unknown exception" << endl;
  }

  // Return the name of the module
  return name;
}


int InventoryPlanningSolver::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<InventoryPlanningSolver>(
    "solver", "solver_inventoryplanning", Object::create<InventoryPlanningSolver>
    );
  registerFields<InventoryPlanningSolver>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  PythonType& x = FreppleClass<InventoryPlanningSolver, Solver>::getPythonType();
  x.setName("solver_inventoryplanning");
  x.setDoc("frePPLe solver_inventoryplanning");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("solve", Solver::solve, METH_NOARGS, "run the solver");
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}



PyObject* InventoryPlanningSolver::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Create the solver
    InventoryPlanningSolver *s = new InventoryPlanningSolver();

    // Iterate over extra keywords, and set attributes.   @todo move this responsibility to the readers...
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwds, &pos, &key, &value))
    {
      PythonData field(value);
      PyObject* key_utf8 = PyUnicode_AsUTF8String(key);
      DataKeyword attr(PyBytes_AsString(key_utf8));
      Py_DECREF(key_utf8);
      const MetaFieldBase* fmeta = metadata->findField(attr.getHash());
      if (!fmeta)
        fmeta = Solver::metadata->findField(attr.getHash());
      if (fmeta)
        // Update the attribute
        fmeta->setField(s, field);
      else
        PyErr_Format(PyExc_AttributeError,
            "attribute '%S' on '%s' can't be updated",
            key, Py_TYPE(s)->tp_name);
    };

    // Return the object. The reference count doesn't need to be increased
    // as we do with other objects, because we want this object to be available
    // for the garbage collector of Python.
    return static_cast<PyObject*>(s);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


void InventoryPlanningSolver::solve(void* v)
{
  if (getLogLevel()>0)
    logger << "Calling inventory planning solver" << endl;

  // For classic inventory planning, simply call the solve method for each
  // indivudual buffer.
  // For mulit-echelon optimization, this loop needs to be
  // replaced with something else.
  for (Buffer::iterator i = Buffer::begin(); i != Buffer::end(); ++i)
    solve(&*i);
}


void InventoryPlanningSolver::solve(const Buffer* b, void* v)
{
  logger << "Inventory planning solver on buffer " << b << endl;

  // Calculation below applies to a single period.
  // For SAIC I'll need to compute it for each period.

  // Get the lead time from the operation replenishing this buffer
  Duration leadtime;
  Operation *oper = b->getProducingOperation();
  if (!oper)
    logger << "     No replenishing operation defined" << endl;
  else if (oper->getType() != *OperationFixedTime::metadata)
    logger << "     Replenishing operation should be of type fixed_time" << endl;
  else
    leadtime = static_cast<OperationFixedTime*>(oper)->getDuration();
  logger << "     Lead time: " << leadtime << endl;

  // Get the demand.
  // We take the average demand seen on the buffer for the next 3 lead time periods.
  // To be thought through more carefully... MCA calculations are based on a single
  // point in time, while the distribution type of industries frePPLe is dealing with
  // have a time aspect as well in the forecast: a peak in a seasonal forecast must
  // have an influence on the safety stock upstream in periods that properly offset
  // with the lead time.
  double consumption = 0.0;
  Date fence = Plan::instance().getCurrent() + Duration(leadtime * 3);
  for (Buffer::flowplanlist::const_iterator i = b->getFlowPlans().begin();
    i != b->getFlowPlans().end(); ++i)
  {
    if (i->getQuantity() < 0)
      consumption -= i->getQuantity();
    if (i->getDate() > fence)
      break;
  }
  logger << "     Consumption over lead time: " << consumption / 3 << endl;

  // Final result will go here:
  //    b->setMinimum()
  //    b->setMinimumCalendar()

}

}       // end namespace
