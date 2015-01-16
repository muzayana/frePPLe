/***************************************************************************
 *                                                                         *
 * Copyright (C) 2014 by Johan De Taeye, frePPLe bvba                      *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "json.h"
#include "webserver.h"

namespace module_webserver
{


MODULE_EXPORT const char* initialize(const Environment::ParameterList& params)
{
  // Initialize only once
  static bool init = false;
  static const char* name = "webserver";
  if (init)
  {
    logger << "Warning: Initializing module webserver more than once." << endl;
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
    PythonInterpreter::registerGlobalMethod(
      "runWebServer", runWebServer, METH_VARARGS,
      "Runs the embedded web server.");
    PythonInterpreter::registerGlobalMethod(
      "saveJSONfile", saveJSONfile, METH_VARARGS,
      "Save the model to a JSON-file.");
    PythonInterpreter::registerGlobalMethod(
      "runDatabaseThread", runDatabaseThread, METH_VARARGS,
      "Start a thread to persist data in a PostgreSQL database.");
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


}       // end namespace
