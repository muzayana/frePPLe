/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2015 by Johan De Taeye, frePPLe bvba                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

/** @file freppleinterface.h
  * @brief Public header file for C and C++.
  *
  * This is the public header file for high-level access to the library
  * functionality.<br>
  * The methods listed provide also a safe interface API for accessing the
  * library functionality from C, C++, Visual Basic and other programming
  * languages.
  *
  * When extending the library, use the header file frepple.h instead.<br>
  * It provides a more detailed API to interface with frePPLe.
  */

#pragma once
#ifndef FREPPLE_INTERFACE_H
#define FREPPLE_INTERFACE_H

#ifdef __cplusplus
#include <string>
#endif

// For a windows shared library we use the C calling convention: __stdcall.
// Only such functions can be called from VBA...
// For cygwin we don't use the __stdcall, but still need the export/import.
#undef DECLARE_EXPORT
#if defined(WIN32) && !defined(DOXYGEN)
  #ifdef __CYGWIN__
    #ifdef FREPPLE_CORE
      #define DECLARE_EXPORT(type) __declspec (dllexport) type
    #else
      #define DECLARE_EXPORT(type) __declspec (dllimport) type
    #endif
  #else
    #ifdef FREPPLE_CORE
      #define DECLARE_EXPORT(type) __declspec (dllexport) type __stdcall
    #else
      #define DECLARE_EXPORT(type) __declspec (dllimport) type __stdcall
    #endif
  #endif
#else
  #define DECLARE_EXPORT(type) type
#endif

/** This method returns a version string. */
DECLARE_EXPORT(const char*) FreppleVersion();

/** This function should be called once when the client application starts,
  * and before calling any other function in the API.
  *
  * This method is synchronous, i.e. it returns only when the complete
  * processing is finished. The method can throw exceptions, and the client
  * is responsible for defining the correct handlers for these.
  */
DECLARE_EXPORT(void) FreppleInitialize();

/** The character buffer pointed to by the first parameter contains data in
  * XML format that is passed on to frePPLe for processing.<br>
  * The second argument specifies whether frePPLe should validate the data
  * against the XSD schema.<br>
  * The last argument specifies whether frePPLe needs to perform only the
  * validation and skip the actual processing.
  *
  * The client is responsible for the memory management in the data buffer.
  *
  * This method is synchroneous, i.e. it returns only when the complete
  * processing is finished. The method can throw exceptions, and the client
  * is responsible for defining the correct handlers for these.
  */
DECLARE_EXPORT(void) FreppleReadXMLData(const char*, bool, bool);

/** The first parameter is the name of a file that contains data in XML
  * format for frePPLe processing. If a NULL pointer is passed, frepple
  * will read from the standard input.<br>
  * The second argument specifies whether frePPLe should validate the data
  * against the XSD schema.<br>
  * The last argument specifies whether frePPLe needs to perform only the
  * validation and skip the actual processing.
  *
  * This method is synchroneous, i.e. it returns only when the complete
  * processing is finished. The method can throw exceptions, and the client
  * is responsible for defining the correct handlers for these.
  */
DECLARE_EXPORT(void) FreppleReadXMLFile(const char*, bool, bool);

/** Execute the Python code in a file.
  *
  * This method is synchroneous, i.e. it returns only when the complete
  * processing is finished. The method can throw exceptions, and the client
  * is responsible for defining the correct handlers for these.
  */
DECLARE_EXPORT(void) FreppleReadPythonFile(const char*);

/** Calling this function will save the frePPLe data in the file that
  * is passed as the argument.
  *
  * This method is synchroneous, i.e. it returns only when the complete
  * processing is finished. The method can throw exceptions, and the client
  * is responsible for defining the correct handlers for these.
  */
DECLARE_EXPORT(void) FreppleSaveFile(const char*);

/** This function causes the frepple executable to shut down in an orderly
  * way.
  *
  * This method is synchroneous, i.e. it returns only when the complete
  * processing is finished. The method can throw exceptions, and the client
  * is responsible for defining the correct handlers for these.
  */
DECLARE_EXPORT(void) FreppleExit();

#ifdef __cplusplus
/** Echo a message in the frePPLe log stream (which is either a file or
  * the standard output stream).
  *
  * This function is only available when using C++. The same functionality
  * is available to C with the function FreppleLog(const char*).
  */
DECLARE_EXPORT(void) FreppleLog(const std::string&);

#if defined(WIN32) && !defined(__CYGWIN__)
/** Install, uninstall or run frePPLe as a Windows service. */
DECLARE_EXPORT(int) FreppleService(short int);
#endif

/* The functions listed below can be called from C. */
extern "C"
{

#endif
  /** Echo a message in the frePPLe log stream (which is either a file or
    * the standard output stream).
    */
  DECLARE_EXPORT(void) FreppleLog(const char*);

  /** Same as FreppleInitialize, but catches all exceptions and returns a
    * status instead.
    *
    * Use this function when calling the library from C or VB applications.
    * @see FreppleInitialize
    */
  DECLARE_EXPORT(int) FreppleWrapperInitialize();

  /** Same as FreppleReadXMLData, but catches all exceptions and returns a
    * status instead.
    *
    * Use this function when calling the library from C or VB applications.
    * @see FreppleReadXMLData
    */
  DECLARE_EXPORT(int) FreppleWrapperReadXMLData(char*, bool, bool);

  /** Same as FreppleReadXMLFile, but catches all exceptions and returns a
    * status instead.
    *
    * Use this function when calling the library from C or VB applications.
    * @see FreppleReadXMLFile
    */
  DECLARE_EXPORT(int) FreppleWrapperReadXMLFile(const char*, bool, bool);

  /** Same as FreppleReadPythonFile, but catches all exceptions and returns a
    * status instead.
    *
    * Use this function when calling the library from C or VB applications.
    * @see FreppleReadPythonFile
    */
  DECLARE_EXPORT(int) FreppleWrapperReadPythonFile(const char*);

  /** Same as FreppleSaveFile, but catches all exceptions and returns a
    * status instead.
    *
    * Use this function when calling the library from C or VB applications.
    * @see FreppleSaveFile
    */
  DECLARE_EXPORT(int) FreppleWrapperSaveFile(char*);

  /** Same as FreppleExit, but catches all exceptions and returns a
    * status instead.
    *
    * Use this function when calling the library from C or VB applications.
    * @see FreppleExit
    */
  DECLARE_EXPORT(int) FreppleWrapperExit();

#ifdef __cplusplus
}  // End of "extern C"
#endif

#endif    // End of FREPPLE_INTERFACE_H
