/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba                 *
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
#include "frepple.h"
#include "freppleinterface.h"
using namespace frepple;
#include <sys/stat.h>


DECLARE_EXPORT(const char*) FreppleVersion()
{
  return PACKAGE_VERSION;
}


DECLARE_EXPORT(void) FreppleInitialize()
{
  // Initialize only once
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  // Initialize the libraries
  LibraryUtils::initialize();
  LibraryModel::initialize();
  LibrarySolver::initialize();

  // Search for the initialization PY file
  string init = Environment::searchFile("init.py");
  if (!init.empty())
  {
    // Execute the commands in the file
    try
    {
      PythonInterpreter::executeFile(init);
    }
    catch (...)
    {
      logger << "Exception caught during execution of 'init.py'" << endl;
      throw;
    }
  }

  // Search for the initialization XML file
  init = Environment::searchFile("init.xml");
  if (!init.empty())
  {
    // Execute the commands in the file
    try { XMLInputFile(init).parse(&Plan::instance(),true); }
    catch (...)
    {
      logger << "Exception caught during execution of 'init.xml'" << endl;
      throw;
    }
  }
}


DECLARE_EXPORT(void) FreppleReadXMLData (const char* x, bool validate, bool validateonly)
{
  if (!x) return;
  if (validateonly)
    XMLInputString(x).parse(NULL, true);
  else
    XMLInputString(x).parse(&Plan::instance(), validate);
}


DECLARE_EXPORT(void) FreppleReadXMLFile (const char* filename, bool validate, bool validateonly)
{
  if (!filename)
  {
    // Read from standard input
    xercesc::StdInInputSource in;
    if (validateonly)
      // When no root object is passed, only the input validation happens
      XMLInput().parse(in, NULL, true);
    else
      XMLInput().parse(in, &Plan::instance(), validate);
  }
  else if (validateonly)
    // Read and validate a file
    XMLInputFile(filename).parse(NULL, true);
  else
    // Read, execute and optionally validate a file
    XMLInputFile(filename).parse(&Plan::instance(),validate);
}


DECLARE_EXPORT(void) FreppleReadPythonFile(const char* filename)
{
  if (!filename)
    throw DataException("No Python file passed to execute");
  PythonInterpreter::executeFile(filename);
}


DECLARE_EXPORT(void) FreppleSaveFile(const char* x)
{
  XMLOutputFile o(x);
  o.writeElementWithHeader(Tags::tag_plan, &Plan::instance());
}


/** Closing any resources still used by frePPle.<br>
  * Allocated memory is not freed up with this call - for performance
  * reasons it is easier to "leak" the memory. The memory is freed when
  * the process exits.
  */
DECLARE_EXPORT(void) FreppleExit()
{
  // Close the log file
  Environment::setLogFile("");
}


DECLARE_EXPORT(void) FreppleLog(const string& msg)
{
  logger << msg << endl;
}

#if defined(WIN32) && !defined(__CYGWIN__)
DECLARE_EXPORT(int) FreppleService(short int action)
{
  switch(action)
  {
    case 0: return Service::install();
    case 1: return Service::uninstall();
    case 2: return Service::run();
  }
  logger << "Invalid argument for method FreppleService" << endl;
  return EXIT_FAILURE;
}
#endif


extern "C" DECLARE_EXPORT(void) FreppleLog(const char* msg)
{
  logger << msg << endl;
}


extern "C" DECLARE_EXPORT(int) FreppleWrapperInitialize()
{
  try {FreppleInitialize();}
  catch (...) {return EXIT_FAILURE;}
  return EXIT_SUCCESS;
}


extern "C" DECLARE_EXPORT(int) FreppleWrapperReadXMLData(char* d, bool v, bool c)
{
  try {FreppleReadXMLData(d, v, c);}
  catch (...) {return EXIT_FAILURE;}
  return EXIT_SUCCESS;
}


extern "C" DECLARE_EXPORT(int) FreppleWrapperReadXMLFile(const char* f, bool v, bool c)
{
  try {FreppleReadXMLFile(f, v, c);}
  catch (...) {return EXIT_FAILURE;}
  return EXIT_SUCCESS;
}


extern "C" DECLARE_EXPORT(int) FreppleWrapperReadPythonFile(const char* f)
{
  try {FreppleReadPythonFile(f);}
  catch (...) {return EXIT_FAILURE;}
  return EXIT_SUCCESS;
}


extern "C" DECLARE_EXPORT(int) FreppleWrapperSaveFile(char* f)
{
  try {FreppleSaveFile(f);}
  catch (...) {return EXIT_FAILURE;}
  return EXIT_SUCCESS;
}


extern "C" DECLARE_EXPORT(int) FreppleWrapperExit()
{
  try {FreppleExit();}
  catch (...) {return EXIT_FAILURE;}
  return EXIT_SUCCESS;
}


/** Used to initialize frePPLe as a Python extension module. */
PyMODINIT_FUNC PyInit_frepple(void)
{
  try
  {
    FreppleInitialize();
    return PythonInterpreter::getModule();
  }
  catch(const exception& e)
  {
    logger << "Initialization failed: " << e.what() << endl;
    return NULL;
  }
  catch (...)
  {
    logger << "Initialization failed: reason unknown" << endl;
    return NULL;
  }
}
