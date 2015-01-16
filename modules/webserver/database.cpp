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

#include "webserver.h"
#include <deque>

namespace module_webserver
{


DatabaseWriter* DatabaseWriter::writeSingleton = NULL;

string DatabaseWriter::connectionstring;


PyObject* runDatabaseThread (PyObject* self, PyObject* args, PyObject* kwds)
{
  // Pick up arguments
  const char *con = "";
  int ok = PyArg_ParseTuple(args, "|s:runDatabaseThread", &con);
  if (!ok) return NULL;

  // Create a new thread
  DatabaseWriter::connectionstring = con;
  DatabaseWriter::writeSingleton = new DatabaseWriter();

  // Return. The database writer is now running in a seperate thread from now onwards.
  return Py_BuildValue("");
}


DatabaseWriter::DatabaseWriter()
{
#ifdef HAVE_PTHREAD_H
  pthread_t writer;
  int errcode = pthread_create(
    &writer,     // thread struct
    NULL,        // default thread attributes
    writethread, // start routine
    this         // arg to routine
    );
  if (errcode)
  {
    ostringstream ch;
    ch << "Can't create any threads, error " << errcode;
    throw RuntimeException(ch.str());
  };
#else
  unsigned int writer_id;
  HANDLE writer = reinterpret_cast<HANDLE>(
        _beginthreadex(0,  // Security attributes
            0,             // Stack size
            &writethread,  // Thread function
            this,          // Argument list
            0,             // Initial state is 0, "running"
            &writer_id     // Address to receive the thread identifier
            ));
    if (!writer)
      throw RuntimeException("Can't create any threads, error " + errno);
#endif    // End of #ifdef ifHAVE_PTHREAD_H
}


void DatabaseWriter::pushStatement(string sql)
{
  if (!writeSingleton)
    throw LogicException("Database writer not initialized");
  ScopeMutexLock l(writeSingleton->lock);
  writeSingleton->statements.push_back(sql);
}


string DatabaseWriter::popStatement()
{
  ScopeMutexLock l(lock);
  if (statements.empty())
    return "";
  string c = statements.front();
  statements.pop_front();
  return c;
}


#if defined(HAVE_PTHREAD_H)
void* DatabaseWriter::writethread(void *arg)
#else
unsigned __stdcall DatabaseWriter::writethread(void *arg)
#endif
{
  // Each OS-level thread needs to initialize a Python thread state.
  // But we won't be executing Python code from this thread...
  //PythonInterpreter::addThread();

  // Connect to the database
  DatabaseWriter* writer = static_cast<DatabaseWriter*>(arg);
  PGconn *conn = PQconnectdb(writer->connectionstring.c_str());
  if (PQstatus(conn) != CONNECTION_OK)
  {
    logger << "Database thread error: Connection failed: " << PQerrorMessage(conn) << endl;
    PQfinish(conn);
    //PythonInterpreter::deleteThread();
    return 0;
  }

  // Switch to autocommit
  PGresult* res = PQexec(conn, "SET AUTOCOMMIT = ON");
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    logger << "Database thread error: Autocommit failed: " << PQerrorMessage(conn) << endl;
    PQfinish(conn);
    //PythonInterpreter::deleteThread();
    return 0;
  }
  PQclear(res);

  // Message
  logger << "Initialized database writer thread" << endl;

  // Endless loop
  while (true)
  {
    // Sleep for a second
    Environment::sleep(1000); // milliseconds

    // Loop while we have commands in the queue
    while (true)
    {
#if defined(HAVE_PTHREAD_H)
      // Verify whether there has been a cancellation request in the meantime
      pthread_testcancel();
#endif
      // Pick up a statement
      string stmt = writer->popStatement();
      if (stmt.empty()) break; // Queue is empty

      // Execute the statement
      PGresult* res = PQexec(conn, stmt.c_str());
      if (PQresultStatus(res) != PGRES_COMMAND_OK)
      {
        logger << "Database thread error: statement failed: " << PQerrorMessage(conn) << endl;
        logger << "  Statement: " << stmt << endl;
        // TODO Catch dropped connections PGRES_FATAL_ERROR and then call PQreset(conn) to reconnect automatically
      }
      PQclear(res);
    }    // While queue not empty
  };  // Infinite loop till program ends

  // Finalize
  PQfinish(conn);
  //PythonInterpreter::deleteThread();
  logger << "Finished database writer thread" << endl;
  return 0;
}

}       // end namespace
