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

string DatabaseWriter::defaultconnectionstring;


PGresult *DatabaseStatement::execute(PGconn* conn) const
{
  const char* paramValues[4];
  switch(args)
  {
    case 0:
      return PQexec(conn, sql.c_str());
    case 1:
      paramValues[0] = arg1.c_str();
      return PQexecParams(conn, sql.c_str(), 1, NULL, paramValues, NULL, NULL, 0);
    case 2:
      paramValues[0] = arg1.c_str();
      paramValues[1] = arg2.c_str();
      return PQexecParams(conn, sql.c_str(), 2, NULL, paramValues, NULL, NULL, 0);
    case 3:
      paramValues[0] = arg1.c_str();
      paramValues[1] = arg2.c_str();
      paramValues[2] = arg3.c_str();
      return PQexecParams(conn, sql.c_str(), 3, NULL, paramValues, NULL, NULL, 0);
    case 4:
      paramValues[0] = arg1.c_str();
      paramValues[1] = arg2.c_str();
      paramValues[2] = arg3.c_str();
      paramValues[3] = arg4.c_str();
      return PQexecParams(conn, sql.c_str(), 4, NULL, paramValues, NULL, NULL, 0);
    default:
      throw DataException("Database statement gets more than 4 arguments passed");
  }
}


DatabaseReader::DatabaseReader(const string& c) : connectionstring(c)
{
  conn = PQconnectdb(connectionstring.c_str());
  if (PQstatus(conn) != CONNECTION_OK)
  {
    stringstream o;
    o << "Database error: Connection failed: " << PQerrorMessage(conn) << endl;
    PQfinish(conn);
    conn = NULL;
    throw RuntimeException(o.str());
  }
}


DatabaseReader::~DatabaseReader()
{
  if (conn) PQfinish(conn);
}


void DatabaseReader::executeSQL(DatabaseStatement& stmt)
{
  PGresult *res = stmt.execute(conn);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    stringstream o;
    o << "Database error: " << PQerrorMessage(conn) << endl;
    o << "   statement: " << stmt << endl;
    PQclear(res);
    throw RuntimeException(o.str());
  }
  PQclear(res);
}


DatabaseReader::DatabaseResult DatabaseReader::fetchSQL(DatabaseStatement& stmt)
{
  PGresult *res = stmt.execute(conn);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    stringstream o;
    o << "Database error: " << PQerrorMessage(conn) << endl;
    o << "   statement: " << stmt << endl;
    PQclear(res);
    throw RuntimeException(o.str());
  }
  return DatabaseResult(res);
}


PyObject* runDatabaseThread (PyObject* self, PyObject* args, PyObject* kwds)
{
  // Pick up arguments
  const char *con = "";
  int ok = PyArg_ParseTuple(args, "|s:runDatabaseThread", &con);
  if (!ok) return NULL;

  // Create a new thread
  DatabaseWriter::launchWriter(con);

  // Return. The database writer is now running in a seperate thread from now onwards.
  return Py_BuildValue("");
}


void DatabaseWriter::launchWriter(const string& c)
{
  if (writeSingleton)
    throw RuntimeException("Database writer already running");
  writeSingleton = new DatabaseWriter(c);
}


DatabaseWriter::DatabaseWriter(const string& c) : connectionstring(c)
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


void DatabaseWriter::pushStatement(const string& sql)
{
  if (!writeSingleton)
    throw LogicException("Database writer not initialized");
  ScopeMutexLock l(writeSingleton->lock);
  writeSingleton->statements.push_back(DatabaseStatement(sql));
}


void DatabaseWriter::pushStatement(const string& sql, const string& arg1)
{
  if (!writeSingleton)
    throw LogicException("Database writer not initialized");
  ScopeMutexLock l(writeSingleton->lock);
  writeSingleton->statements.push_back(DatabaseStatement(sql, arg1));
}


void DatabaseWriter::pushStatement(
  const string& sql, const string& arg1, const string& arg2
  )
{
  if (!writeSingleton)
    throw LogicException("Database writer not initialized");
  ScopeMutexLock l(writeSingleton->lock);
  writeSingleton->statements.push_back(DatabaseStatement(sql, arg1, arg2));
}


void DatabaseWriter::pushStatement(
  const string& sql, const string& arg1, const string& arg2, const string& arg3
  )
{
  if (!writeSingleton)
    throw LogicException("Database writer not initialized");
  ScopeMutexLock l(writeSingleton->lock);
  writeSingleton->statements.push_back(DatabaseStatement(sql, arg1, arg2, arg3));
}


void DatabaseWriter::pushStatement(
  const string& sql, const string& arg1, const string& arg2, const string& arg3, const string& arg4
  )
{
  if (!writeSingleton)
    throw LogicException("Database writer not initialized");
  ScopeMutexLock l(writeSingleton->lock);
  writeSingleton->statements.push_back(DatabaseStatement(sql, arg1, arg2, arg3, arg4));
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
      // To be reviewed: we remote the statement, regardless whether execution failed or not. We may loose some changes if eg the connection was dropped.
      writer->lock.lock();
      if (writer->statements.empty())
      {
        writer->lock.unlock();
        break; // Queue is empty
      }
      const DatabaseStatement stmt = writer->statements.front();
      writer->statements.pop_front();
      writer->lock.unlock();

      // Execute the statement
      res = stmt.execute(conn);
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
