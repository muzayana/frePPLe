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

/** @file pythonutils.cpp
  * @brief Reusable functions for python functionality.
  *
  * The structure of the C++ wrappers around the C Python API is heavily
  * inspired on the design of PyCXX.<br>
  * More information can be found on http://cxx.sourceforge.net
  */

#define FREPPLE_CORE
#include "frepple/utils.h"
#include "frepple/xml.h"

namespace frepple
{
namespace utils
{

DECLARE_EXPORT PyObject* PythonLogicException = NULL;
DECLARE_EXPORT PyObject* PythonDataException = NULL;
DECLARE_EXPORT PyObject* PythonRuntimeException = NULL;

DECLARE_EXPORT PyObject *PythonInterpreter::module = NULL;
DECLARE_EXPORT PyThreadState* PythonInterpreter::mainThreadState = NULL;


DECLARE_EXPORT void Object::writeElement(
  Serializer* o, const Keyword& tag, FieldCategory m
  ) const
{
  // Don't serialize hidden objects
  if (getHidden() && !o->getWriteHidden())
    return;

  const MetaClass& meta = getType();

  // Write the head
  if (o->getSkipHead())
    o->skipHead(false);
  else
  {
    if (meta.isDefault)
      o->BeginObject(tag);
    else
      o->BeginObject(tag, Tags::type, getType().type);
  }

  // Write the content
  if (m == MANDATORY)
  {
    // Write references only
    if (meta.category)
      for (MetaClass::fieldlist::const_iterator i = meta.category->getFields().begin(); i != meta.category->getFields().end(); ++i)
        if ((*i)->getFlag(MANDATORY))
          (*i)->writeField(*o);
    for (MetaClass::fieldlist::const_iterator i = meta.getFields().begin(); i != meta.getFields().end(); ++i)
      if ((*i)->getFlag(MANDATORY))
        (*i)->writeField(*o);
  }
  else if (m == BASE)
  {
    // Write only the fields required to successfully save&restore the object
    if (meta.category)
      for (MetaClass::fieldlist::const_iterator i = meta.category->getFields().begin(); i != meta.category->getFields().end(); ++i)
        if (!(*i)->getFlag(DETAIL) && !(*i)->getFlag(PLAN))
          (*i)->writeField(*o);
    for (MetaClass::fieldlist::const_iterator i = meta.getFields().begin(); i != meta.getFields().end(); ++i)
      if (!(*i)->getFlag(DETAIL) && !(*i)->getFlag(PLAN))
        (*i)->writeField(*o);
    writeProperties(*o);
  }
  else
  {
    // Write the full info on the object
    if (meta.category)
      for (MetaClass::fieldlist::const_iterator i = meta.category->getFields().begin(); i != meta.category->getFields().end(); ++i)
        (*i)->writeField(*o);
    for (MetaClass::fieldlist::const_iterator i = meta.getFields().begin(); i != meta.getFields().end(); ++i)
      (*i)->writeField(*o);
    writeProperties(*o);
  }

  // Write the tail
  if (o->getSkipTail())
    o->skipTail(false);
  else
    o->EndObject(tag);
}


DECLARE_EXPORT size_t Object::getSize() const
{
  const MetaClass& meta = getType();
  size_t tmp = meta.size;
  if (meta.category)
    for (MetaClass::fieldlist::const_iterator i = meta.category->getFields().begin(); i != meta.category->getFields().end(); ++i)
      if (!(*i)->getFlag(COMPUTED))
        tmp += (*i)->getSize(this);
  for (MetaClass::fieldlist::const_iterator i = meta.getFields().begin(); i != meta.getFields().end(); ++i)
    if (!(*i)->getFlag(COMPUTED))
      tmp += (*i)->getSize(this);
  return tmp;
}


PyObject* PythonInterpreter::createModule()
{
  static PyMethodDef freppleMethods[] = {
    {NULL, NULL, 0, NULL}
  };
  static struct PyModuleDef frepplemodule = {
    PyModuleDef_HEAD_INIT,
    "frepple",
    "Access to the frePPLe library",
    -1, freppleMethods,
    NULL, NULL, NULL, NULL
  };
  module = PyModule_Create(&frepplemodule);
  return module;
}


DECLARE_EXPORT void PythonInterpreter::initialize()
{
  // Initialize the Python interpreter in case we are embedding it in frePPLe.
  if(!Py_IsInitialized())
  {
    // This method needs to be called before the initialization
    PyImport_AppendInittab("frepple", &PythonInterpreter::createModule);
    // The arg 0 indicates that the interpreter doesn't
    // implement its own signal handler
    Py_InitializeEx(0);
    // Initializes threads
    PyEval_InitThreads();
    mainThreadState = PyEval_SaveThread();
  }

  // Capture global lock
  PyGILState_STATE state = PyGILState_Ensure();

  // Create the logging function.
  // In Python3 this also creates the frepple module, by calling the createModule callback.
  PyRun_SimpleString(
    "import frepple, sys\n"
    "class redirect:\n"
    "\tdef write(self,str):\n"
    "\t\tfrepple.log(str)\n"
    "sys.stdout = redirect()\n"
    "sys.stderr = redirect()"
  );

  if (!module)
  {
    PyGILState_Release(state);
    throw RuntimeException("Can't initialize Python interpreter");
  }

  // Make the datetime types available
  PyDateTime_IMPORT;

  // Create python exception types
  int nok = 0;
  PythonLogicException = PyErr_NewException((char*)"frepple.LogicException", NULL, NULL);
  Py_IncRef(PythonLogicException);
  nok += PyModule_AddObject(module, "LogicException", PythonLogicException);
  PythonDataException = PyErr_NewException((char*)"frepple.DataException", NULL, NULL);
  Py_IncRef(PythonDataException);
  nok += PyModule_AddObject(module, "DataException", PythonDataException);
  PythonRuntimeException = PyErr_NewException((char*)"frepple.RuntimeException", NULL, NULL);
  Py_IncRef(PythonRuntimeException);
  nok += PyModule_AddObject(module, "RuntimeException", PythonRuntimeException);

  // Add a string constant for the version
  nok += PyModule_AddStringConstant(module, "version", PACKAGE_VERSION);

  // Redirect the stderr and stdout streams of Python
  registerGlobalMethod("log", python_log, METH_VARARGS,
      "Prints a string to the frePPLe log file.", false);

  // Release the lock
  PyGILState_Release(state);

  // A final check...
  if (nok) throw RuntimeException("Can't initialize Python interpreter");
}


DECLARE_EXPORT void PythonInterpreter::finalize()
{
  // Only valid if this is an embedded interpreter
  if (!mainThreadState) return;

  // Swap to the main thread and exit
  PyEval_AcquireLock();
  PyEval_RestoreThread(mainThreadState);
  Py_Finalize();
}


DECLARE_EXPORT void PythonInterpreter::addThread()
{
  // Check whether the thread already has a Python state
  PyThreadState * myThreadState = PyGILState_GetThisThreadState();
  if (myThreadState) return;

  // Create a new state
  PyThreadState *tcur = PyThreadState_New(PyInterpreterState_Head());
  if (!tcur) throw RuntimeException("Can't create new thread state");

  // Make the new state current
  PyEval_RestoreThread(tcur);
  PyEval_ReleaseLock();
}


DECLARE_EXPORT void PythonInterpreter::deleteThread()
{
  // Check whether the thread already has a Python state
  PyThreadState * tcur = PyGILState_GetThisThreadState();
  if (!tcur) return;

  // Delete the current Python thread state
  PyEval_RestoreThread(tcur);
  PyThreadState_Clear(tcur);
  PyThreadState_DeleteCurrent(); // This releases the GIL too!
}


DECLARE_EXPORT void PythonInterpreter::execute(const char* cmd)
{
  // Capture global lock
  PyGILState_STATE state = PyGILState_Ensure();

  // Execute the command
  PyObject *m = PyImport_AddModule("__main__");
  if (!m)
  {
    // Release the global Python lock
    PyGILState_Release(state);
    throw RuntimeException("Can't initialize Python interpreter");
  }
  PyObject *d = PyModule_GetDict(m);
  if (!d)
  {
    // Release the global Python lock
    PyGILState_Release(state);
    throw RuntimeException("Can't initialize Python interpreter");
  }

  // Execute the Python code. Note that during the call the Python lock can be
  // temporarily released.
  PyObject *v = PyRun_String(cmd, Py_file_input, d, d);
  if (!v)
  {
    // Print the error message
    PyErr_Print();
    // Release the global Python lock
    PyGILState_Release(state);
    throw RuntimeException("Error executing Python command");
  }
  Py_DECREF(v);
  PyErr_Clear();
  // Release the global Python lock
  PyGILState_Release(state);
}


DECLARE_EXPORT void PythonInterpreter::executeFile(string filename)
{
  // Replacing ' with \' to escape the quotes in the Python command
  for (string::size_type pos = filename.find_first_of("'", 0);
      pos < string::npos;
      pos = filename.find_first_of("'", pos))
  {
    filename.replace(pos,1,"\\'",2);
    pos+=2;
  }

  // Execute the Python script
  string cmd = "with open(r'" + filename
    + "', 'rb') as f: exec(compile(f.read(), r'" + filename
    + "', 'exec'), globals(), locals())";
  execute(cmd.c_str());
}


DECLARE_EXPORT void PythonInterpreter::registerGlobalMethod(
  const char* name, PyCFunction method, int flags, const char* doc, bool lock
)
{
  // Define a new method object.
  // We need are leaking the memory allocated for it to assure the data
  // are available at all times to Python.
  string *leakingName = new string(name);
  string *leakingDoc = new string(doc);
  PyMethodDef *newMethod = new PyMethodDef;
  newMethod->ml_name = leakingName->c_str();
  newMethod->ml_meth = method;
  newMethod->ml_flags = flags;
  newMethod->ml_doc = leakingDoc->c_str();

  // Lock the interpreter
  PyGILState_STATE state;
  if (lock) state = PyGILState_Ensure();

  // Register a new C function in Python
  PyObject* mod = PyUnicode_FromString("frepple");
  if (!mod)
  {
    if (lock) PyGILState_Release(state);;
    throw RuntimeException("Error registering a new Python method");
  }
  PyObject* func = PyCFunction_NewEx(newMethod, NULL, mod);
  Py_DECREF(mod);
  if (!func)
  {
    if (lock) PyGILState_Release(state);
    throw RuntimeException("Error registering a new Python method");
  }

  // Add the method to the module dictionary
  PyObject* moduledict = PyModule_GetDict(module);
  if (!moduledict)
  {
    Py_DECREF(func);
    if (lock) PyGILState_Release(state);
    throw RuntimeException("Error registering a new Python method");
  }
  if (PyDict_SetItemString(moduledict ,leakingName->c_str(), func) < 0)
  {
    Py_DECREF(func);
    if (lock) PyGILState_Release(state);
    throw RuntimeException("Error registering a new Python method");
  }
  Py_DECREF(func);

  // Release the interpeter
  if (lock) PyGILState_Release(state);
}


DECLARE_EXPORT void PythonInterpreter::registerGlobalMethod
(const char* c, PyCFunctionWithKeywords f, int i, const char* d, bool b)
{
  registerGlobalMethod(c, reinterpret_cast<PyCFunction>(f), i | METH_KEYWORDS, d, b);
}


DECLARE_EXPORT void PythonInterpreter::registerGlobalObject
(const char* name, PyObject *obj, bool lock)
{
  PyGILState_STATE state;
  if (lock) state = PyGILState_Ensure();
  PyModule_AddObject(module, name, obj);
  if (lock) PyGILState_Release(state);
}


PyObject* PythonInterpreter::python_log(PyObject *self, PyObject *args)
{
  // Pick up arguments
  char *data;
  int ok = PyArg_ParseTuple(args, "s:log", &data);
  if (!ok) return NULL;

  // Print and flush the output stream
  logger << data;
  logger.flush();

  // Return code
  return Py_BuildValue("");  // Safer than using Py_None, which is not
  // portable across compilers
}


const PyTypeObject PythonType::PyTypeObjectTemplate =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  "frepple.unspecified",  /* WILL BE UPDATED tp_name */
  0,  /* WILL BE UPDATED tp_basicsize */
  0,  /* tp_itemsize */
  0,  /* CAN BE UPDATED tp_dealloc */
  0,  /* tp_print */
  0,  /* tp_getattr */
  0,  /* tp_setattr */
  0,  /* tp_compare */
  0,  /* tp_repr */
  0,  /* tp_as_number */
  0,  /* tp_as_sequence */
  0,  /* tp_as_mapping */
  reinterpret_cast<hashfunc>(_Py_HashPointer),  /* tp_hash */
  0,  /* CAN BE UPDATED tp_call */
  0,  /* CAN BE UPDATED tp_str */
  0,  /* CAN BE UPDATED tp_getattro */
  0,  /* CAN BE UPDATED tp_setattro */
  0,  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  "std doc", /* CAN BE UPDATED  tp_doc */
  0,  /* tp_traverse */
  0,  /* tp_clear */
  0,  /* CAN BE UPDATED tp_richcompare */
  0,  /* tp_weaklistoffset */
  0,  /* CAN BE UPDATED tp_iter */
  0,  /* CAN BE UPDATED tp_iternext */
  0,  /* tp_methods */
  0,  /* tp_members */
  0,  /* tp_getset */
  0,  /* tp_base */
  0,  /* tp_dict */
  0,  /* tp_descr_get */
  0,  /* tp_descr_set */
  0,  /* tp_dictoffset */
  0,  /* tp_init */
  0,  /* tp_alloc */
  0,  /* CAN BE UPDATED tp_new */
  0,  /* tp_free */
  0, /* tp_is_gc */
  0, /* tp_bases */
  0, /* tp_mro */
  0, /* tp_cache */
  0, /* tp_subclasses */
  0, /* tp_weaklist */
  0, /* tp_del */
  0  /* tp_version_tag */
};


DECLARE_EXPORT void PythonData::setDate(const Date d)
{
  if (obj) Py_DECREF(obj);
  PyDateTime_IMPORT;
  // The standard library function localtime() is not re-entrant: the same
  // static structure is used for all calls. In a multi-threaded environment
  // the function is not to be used.
  // The POSIX standard defines a re-entrant version of the function:
  // localtime_r.
  // Visual C++ 6.0 and Borland 5.5 are missing it, but provide a thread-safe
  // variant without changing the function semantics.
  time_t ticks = d.getTicks();
#ifdef HAVE_LOCALTIME_R
  struct tm t;
  localtime_r(&ticks, &t);
#else
  struct tm t = *localtime(&ticks);
#endif
  obj = PyDateTime_FromDateAndTime(t.tm_year+1900, t.tm_mon+1, t.tm_mday,
      t.tm_hour, t.tm_min, t.tm_sec, 0);
}


DECLARE_EXPORT Date PythonData::getDate() const
{
  PyDateTime_IMPORT;
  if (PyDateTime_Check(obj))
    return Date(
        PyDateTime_GET_YEAR(obj),
        PyDateTime_GET_MONTH(obj),
        PyDateTime_GET_DAY(obj),
        PyDateTime_DATE_GET_HOUR(obj),
        PyDateTime_DATE_GET_MINUTE(obj),
        PyDateTime_DATE_GET_SECOND(obj)
        );
  else if (PyDate_Check(obj))
    return Date(
        PyDateTime_GET_YEAR(obj),
        PyDateTime_GET_MONTH(obj),
        PyDateTime_GET_DAY(obj)
        );
  else if (!PyUnicode_Check(obj))
  {
    if (PyUnicode_Check(obj))
    {
      // Replace the unicode object with a string encoded in UTF-8.
      const_cast<PyObject*&>(obj) =
        PyUnicode_AsEncodedString(obj, "UTF-8", "ignore");
    }
    return Date(PyBytes_AsString(PyObject_Str(obj)));
  }
  else
    throw DataException(
      "Invalid data type. Expecting datetime.date, datetime.datetime or string"
    );
}


DECLARE_EXPORT PythonData::PythonData(Object* p)
{
  obj = p ? static_cast<PyObject*>(p) : Py_None;
  Py_INCREF(obj);
}


DECLARE_EXPORT void PythonData::setObject(Object* val)
{
  if (obj) Py_DECREF(obj);
  obj = val ? static_cast<PyObject*>(val) : Py_None;
  Py_INCREF(obj);
}


inline Object* PythonData::getObject() const
{
  if (obj && (obj->ob_type->tp_getattro == getattro_handler ||
    (obj->ob_type->tp_base && obj->ob_type->tp_base->tp_getattro == getattro_handler)))
    // This objects are owned by us!
    return static_cast<Object*>(const_cast<PyObject*>(obj));
  else
    return NULL;
}


DECLARE_EXPORT PythonType::PythonType(size_t base_size, const type_info* tp)
  : cppClass(tp)
{
  // Allocate a new type object if it doesn't exist yet.
  // We copy from a template type definition.
  table = new PyTypeObject(PyTypeObjectTemplate);
  table->tp_basicsize = base_size;
}


DECLARE_EXPORT PythonType* Object::registerPythonType(int size, const type_info *t)
{
  // Scan the types already registered
  for (vector<PythonType*>::const_iterator i = table.begin(); i != table.end(); ++i)
    if (**i==*t) return *i;

  // Not found in the vector, so create a new one
  PythonType *cachedTypePtr = new PythonType(size, t);
  table.push_back(cachedTypePtr);
  return cachedTypePtr;
}


DECLARE_EXPORT void Object::writeProperties(Serializer& o) const
{
  if (!dict) return; // No custom fields here

  // Create a sorted list of all keys
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* key_iterator = PyObject_CallMethod(dict, "keys", NULL); // new ref
  PyObject* keylist = PySequence_Fast(key_iterator, ""); // new ref
  PyList_Sort(keylist);

  // Iterate over all keys
  PyObject *py_key, *py_value;
  Py_ssize_t len = PySequence_Size(keylist);
  for (Py_ssize_t i = 0; i < len; i++)
  {
    py_key = PySequence_Fast_GET_ITEM(keylist, i); // borrowed ref
    PythonData key(py_key);
    py_value = PyDict_GetItem(dict, py_key); // borrowed ref
    PythonData value(py_value);
    if (PyBool_Check(py_value))
      o.writeElement(Tags::booleanproperty,
        Tags::name, key.getString(),
        Tags::value, value.getBool() ? "1" : "0"
        );
    else if (PyFloat_Check(py_value))
      o.writeElement(Tags::doubleproperty,
        Tags::name, key.getString(),
        Tags::value, value.getString()
        );
    else if (PyDateTime_Check(py_value) || PyDate_Check(py_value))
      o.writeElement(Tags::dateproperty,
        Tags::name, key.getString(),
        Tags::value, string(value.getDate())
        );
    else
      o.writeElement(Tags::stringproperty,
        Tags::name, key.getString(),
        Tags::value, value.getString()
        );
  }

  // Clean up
  Py_DECREF(key_iterator);
  Py_DECREF(keylist);
  PyGILState_Release(pythonstate);
}


DECLARE_EXPORT void Object::setProperty(const string& name, const DataValue& value, short type)
{
  // Adding the new key-value pair to the dictionary.
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  if (!dict)
  {
    dict = PyDict_New();
    Py_INCREF(dict);
  }
  switch (type)
  {
    case 1: // Boolean
      {
      PythonData val(value.getBool());
      PyDict_SetItemString(dict, name.c_str(), static_cast<PyObject*>(val));
      break;
      }
    case 2: // Date
      {
      PythonData val(value.getDate());
      PyDict_SetItemString(dict, name.c_str(), static_cast<PyObject*>(val));
      break;
      }
    case 3: // Double
      {
      PythonData val(value.getDouble());
      PyDict_SetItemString(dict, name.c_str(), static_cast<PyObject*>(val));
      break;
      }
    default: // String
      {
      PythonData val(value.getString());
      PyDict_SetItemString(dict, name.c_str(), static_cast<PyObject*>(val));
      }
  }
  PyGILState_Release(pythonstate);
}


DECLARE_EXPORT bool Object::getBoolProperty(const string& name, bool def) const
{
  if (!dict)
    // Not a single property has been defined
    return def;
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* lkp = PyDict_GetItemString(dict, name.c_str());
  if (!lkp)
  {
    // Value not found in the dictionary
    PyGILState_Release(pythonstate);
    return def;
  }
  PythonData val(lkp);
  bool result = val.getBool();
  PyGILState_Release(pythonstate);
  return result;
}


DECLARE_EXPORT Date Object::getDateProperty(const string& name, Date def) const
{
  if (!dict)
    // Not a single property has been defined
    return def;
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* lkp = PyDict_GetItemString(dict, name.c_str());
  if (!lkp)
  {
    // Value not found in the dictionary
    PyGILState_Release(pythonstate);
    return def;
  }
  PythonData val(lkp);
  Date result = val.getDate();
  PyGILState_Release(pythonstate);
  return result;
}


DECLARE_EXPORT double Object::getDoubleProperty(const string& name, double def) const
{
  if (!dict)
    // Not a single property has been defined
    return def;
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* lkp = PyDict_GetItemString(dict, name.c_str());
  if (!lkp)
  {
    // Value not found in the dictionary
    PyGILState_Release(pythonstate);
    return def;
  }
  PythonData val(lkp);
  double result = val.getDouble();
  PyGILState_Release(pythonstate);
  return result;
}


DECLARE_EXPORT string Object::getStringProperty(const string& name, const string& def) const
{
  if (!dict)
    // Not a single property has been defined
    return def;
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* lkp = PyDict_GetItemString(dict, name.c_str());
  if (!lkp)
  {
    // Value not found in the dictionary
    PyGILState_Release(pythonstate);
    return def;
  }
  PythonData val(lkp);
  string result = val.getString();
  PyGILState_Release(pythonstate);
  return result;
}


DECLARE_EXPORT PyObject* Object::toXML(PyObject* self, PyObject* args)
{
  try
  {
    // Parse the argument
    PyObject *filearg = NULL;
    char *mode = NULL;
    if (!PyArg_ParseTuple(args, "|sO:toXML", &mode, &filearg))
      return NULL;

    // Create the XML string.
    ostringstream ch;
    XMLSerializer x(ch);
    x.setReferencesOnly(true);
    if (!mode || mode[0] == 'S')
      x.setContentType(BASE);
    else if (mode[0] == 'P')
      x.setContentType(PLAN);
    else if (mode[0] == 'D')
      x.setContentType(DETAIL);
    else
      throw DataException("Invalid output mode");

    // The next call assumes the self argument is an instance of the Object
    // base class. We don't need to check this explicitly since we expose
    // this method only on subclasses.
    x.pushCurrentObject(static_cast<Object*>(self));
    static_cast<Object*>(self)->writeElement
    (&x, *(static_cast<Object*>(self)->getType().category->typetag));
    // Write the output...
    if (filearg)
    {
      _Py_IDENTIFIER(write);
      PyObject *writer = _PyObject_GetAttrId(filearg, &PyId_write);
      if (writer)
      {
        // ... to a file
        Py_DECREF(writer);
        return PyFile_WriteString(ch.str().c_str(), filearg) ?
            NULL : // Error writing to the file
            Py_BuildValue("");
      }
      else
        // The argument is not a file
        throw LogicException("Expecting a file argument");
    }
    else
      // ... to a string
      return PythonData(ch.str());
  }
  catch(...)
  {
    PythonType::evalException();
    return NULL;
  }
  throw LogicException("Unreachable code reached");
}


DECLARE_EXPORT void PythonType::addMethod
(const char* method_name, PyCFunction f, int flags, const char* doc )
{
  unsigned short i = 0;

  // Create a method table array
  if (!table->tp_methods)
    // Allocate a first block
    table->tp_methods = new PyMethodDef[methodArraySize];
  else
  {
    // Find the first non-empty method record
    while (table->tp_methods[i].ml_name) i++;
    if (i % methodArraySize == methodArraySize - 1)
    {
      // Allocation of a bigger buffer is required
      PyMethodDef* tmp = new PyMethodDef[i + 1 + methodArraySize];
      for(unsigned short j = 0; j < i; j++)
        tmp[j] = table->tp_methods[j];
      delete [] table->tp_methods;
      table->tp_methods = tmp;
    }
  }

  // Populate a method definition struct
  table->tp_methods[i].ml_name = method_name;
  table->tp_methods[i].ml_meth = f;
  table->tp_methods[i].ml_flags = flags;
  table->tp_methods[i].ml_doc = doc;

  // Append an empty terminator record
  table->tp_methods[++i].ml_name = NULL;
  table->tp_methods[i].ml_meth = NULL;
  table->tp_methods[i].ml_flags = 0;
  table->tp_methods[i].ml_doc = NULL;
}


DECLARE_EXPORT void PythonType::addMethod
(const char* c, PyCFunctionWithKeywords f, int i, const char* d)
{
  addMethod(c, reinterpret_cast<PyCFunction>(f), i | METH_KEYWORDS, d);
}


DECLARE_EXPORT int PythonType::typeReady()
{
  // Register the new type in the module
  PyGILState_STATE state = PyGILState_Ensure();
  if (PyType_Ready(table) < 0)
  {
    PyGILState_Release(state);
    throw RuntimeException(string("Can't register python type ") + table->tp_name);
  }
  Py_INCREF(table);
  int result = PyModule_AddObject(
      PythonInterpreter::getModule(),
      table->tp_name + 8, // Note: +8 is to skip the "frepple." characters in the name
      reinterpret_cast<PyObject*>(table)
      );
  PyGILState_Release(state);
  return result;
}


DECLARE_EXPORT void PythonType::evalException()
{
  // Rethrowing the exception to catch its type better
  try {
    throw;
  }
  catch (const DataException& e)
  {
    PyErr_SetString(PythonDataException, e.what());
  }
  catch (const LogicException& e)
  {
    PyErr_SetString(PythonLogicException, e.what());
  }
  catch (const RuntimeException& e)
  {
    PyErr_SetString(PythonRuntimeException, e.what());
  }
  catch (const exception& e)
  {
    PyErr_SetString(PyExc_Exception, e.what());
  }
  catch (...)
  {
    PyErr_SetString(PyExc_Exception, "Unidentified exception");
  }
}


DECLARE_EXPORT PythonFunction::PythonFunction(const string& n)
{
  if (n.empty())
  {
    // Resetting to NULL when the string is empty
    func = NULL;
    return;
  }

  // Find the Python function
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  func = PyRun_String(n.c_str(), Py_eval_input,
      PyEval_GetGlobals(), PyEval_GetLocals() );
  if (!func)
  {
    PyGILState_Release(pythonstate);
    throw DataException("Python function '" + n + "' not defined");
  }
  if (!PyCallable_Check(func))
  {
    PyGILState_Release(pythonstate);
    throw DataException("Python object '" + n + "' is not a function");
  }
  Py_INCREF(func);

  // Store the Python function
  PyGILState_Release(pythonstate);
}


DECLARE_EXPORT PythonFunction::PythonFunction(PyObject* p)
{
  if (!p || p == Py_None)
  {
    // Resetting to null
    func = NULL;
    return;
  }

  if (!PyCallable_Check(p))
  {
    // It's not a callable object. Interprete it as a function name and
    // look it up.
    PyGILState_STATE pythonstate = PyGILState_Ensure();
    string n = PythonData(p).getString();
    p = PyRun_String(n.c_str(), Py_eval_input,
        PyEval_GetGlobals(), PyEval_GetLocals() );
    if (!p)
    {
      PyGILState_Release(pythonstate);
      throw DataException("Python function '" + n + "' not defined");
    }
    if (!PyCallable_Check(p))
    {
      PyGILState_Release(pythonstate);
      throw DataException("Python object '" + n + "' is not a function");
    }
    PyGILState_Release(pythonstate);
  }

  // Store the Python function
  func = p;
  Py_INCREF(func);
}


DECLARE_EXPORT PythonData PythonFunction::call() const
{
  if (!func) return PythonData();
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* result = PyEval_CallFunction(func, "()");
  if (!result)
  {
    logger << "Error: Exception caught when calling Python function '"
        << (func ? PyEval_GetFuncName(func) : "NULL") << "'" << endl;
    if (PyErr_Occurred()) PyErr_PrintEx(0);
  }
  PyGILState_Release(pythonstate);
  return PythonData(result);
}


DECLARE_EXPORT PythonData PythonFunction::call(const PyObject* p) const
{
  if (!func) return PythonData();
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* result = PyEval_CallFunction(func, "(O)", p);
  if (!result)
  {
    logger << "Error: Exception caught when calling Python function '"
        << (func ? PyEval_GetFuncName(func) : "NULL") << "'" << endl;
    if (PyErr_Occurred()) PyErr_PrintEx(0);
  }
  PyGILState_Release(pythonstate);
  return PythonData(result);
}


DECLARE_EXPORT PythonData PythonFunction::call(const PyObject* p, const PyObject* q) const
{
  if (!func) return PythonData();
  PyGILState_STATE pythonstate = PyGILState_Ensure();
  PyObject* result = PyEval_CallFunction(func, "(OO)", p, q);
  if (!result)
  {
    logger << "Error: Exception caught when calling Python function '"
        << (func ? PyEval_GetFuncName(func) : "NULL") << "'" << endl;
    if (PyErr_Occurred()) PyErr_PrintEx(0);
  }
  PyGILState_Release(pythonstate);
  return PythonData(result);
}


extern "C" DECLARE_EXPORT PyObject* getattro_handler(PyObject *self, PyObject *name)
{
  try
  {
    if (!PyUnicode_Check(name))
    {
      PyErr_Format(PyExc_TypeError,
          "attribute name must be string, not '%S'",
          Py_TYPE(name)->tp_name);
      return NULL;
    }

    // Find the field
    Object* cpp_self = static_cast<Object*>(self);
    PyObject* name_utf8 = PyUnicode_AsUTF8String(name);
    char* fname = PyBytes_AsString(name_utf8);
    const MetaFieldBase* fmeta = cpp_self->getType().findField(Keyword::hash(fname));
    if (!fmeta && cpp_self->getType().category)
      fmeta = cpp_self->getType().category->findField(Keyword::hash(fname));
    Py_DECREF(name_utf8);
    PythonData result;
    result.setNull();
    if (fmeta)
    {
      // Retrieve the attribute
      fmeta->getField(cpp_self, result);

      // Exit 1: Normal
      if (static_cast<PyObject*>(result))
        return result;

      // Exit 2: Exception occurred
      if (PyErr_Occurred())
        return NULL;
    }

    // Exit 3: Look up in our custom dictionary
    if (cpp_self->dict)
    {
      PyObject* item = PyDict_GetItem(cpp_self->dict, name);
      if (item)
      {
        Py_INCREF(item);
        return item;
      }
    }

    // Exit 4: No error occurred but the attribute was not found.
    // Use the standard generic function to pick up  standard attributes
    // (such as __class__, __doc__, ...)
    // Note that this function also picks up attributes from base classes, but
    // we can't rely on that: any C++ exceptions are lost along the way...
    return PyObject_GenericGetAttr(self,name);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


extern "C" DECLARE_EXPORT int setattro_handler(PyObject *self, PyObject *name, PyObject *value)
{
  try
  {
    // Pick up the field name
    if (!PyUnicode_Check(name))
    {
      PyErr_Format(PyExc_TypeError,
          "attribute name must be string, not '%S'",
          Py_TYPE(name)->tp_name);
      return -1;
    }
    PythonData field(value);

    // Find the field
    Object* cpp_self = static_cast<Object*>(self);
    PyObject* name_utf8 = PyUnicode_AsUTF8String(name);
    char* fname = PyBytes_AsString(name_utf8);
    const MetaFieldBase* fmeta = cpp_self->getType().findField(Keyword::hash(fname));
    if (!fmeta && cpp_self->getType().category)
      fmeta = cpp_self->getType().category->findField(Keyword::hash(fname));
    Py_DECREF(name_utf8);
    if (fmeta)
    {
      // Update the attribute
      fmeta->setField(cpp_self, field);
      return 0;
    }
    else
    {
      // Add to our custom extension dictionary
      if (value)
      {
        if (!cpp_self->dict)
        {
          cpp_self->dict = PyDict_New();
          Py_INCREF(cpp_self->dict);
        }
        if (!PyDict_SetItem(cpp_self->dict, name, value))
          return 0;
      }

      // Process 'not OK' result - set python error string if it isn't set yet
      if (!PyErr_Occurred())
        PyErr_Format(PyExc_AttributeError,
            "attribute '%S' on '%s' can't be updated",
            name, Py_TYPE(self)->tp_name);
      return -1;
    }
  }
  catch (...)
  {
    PythonType::evalException();
    return -1;
  }
}


extern "C" DECLARE_EXPORT PyObject* compare_handler(PyObject *self, PyObject *other, int op)
{
  try
  {
    if (Py_TYPE(self) != Py_TYPE(other)
        && Py_TYPE(self)->tp_base != Py_TYPE(other)->tp_base)
    {
      // Can't compare these objects.
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
    }
    int result = static_cast<Object*>(self)->compare(other);
    switch(op)
    {
      case Py_LT: return PythonData(result > 0);
      case Py_LE: return PythonData(result >= 0);
      case Py_EQ: return PythonData(result == 0);
      case Py_NE: return PythonData(result != 0);
      case Py_GT: return PythonData(result < 0);
      case Py_GE: return PythonData(result <= 0);
      default: throw LogicException("Unknown operator in comparison");
    }
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


extern "C" DECLARE_EXPORT PyObject* iternext_handler(PyObject *self)
{
  try
  {
    return static_cast<Object*>(self)->iternext();
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


extern "C" DECLARE_EXPORT PyObject* call_handler(PyObject* self, PyObject* args, PyObject* kwds)
{
  try
  {
    return static_cast<Object*>(self)->call(args, kwds);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


extern "C" DECLARE_EXPORT PyObject* str_handler(PyObject* self)
{
  try
  {
    return static_cast<Object*>(self)->str();
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}

} // end namespace
} // end namespace
