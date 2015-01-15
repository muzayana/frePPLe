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

#include <stdio.h>
#include <stdlib.h>

#include "database.h"

namespace module_webserver
{


DatabaseWriter* DatabaseWriter::writeSingleton = NULL;

string DatabaseWriter::connectionstring;


void DatabaseWriter::initialize(const string& c)
{
  connectionstring = c;
  writeSingleton = new DatabaseWriter();
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
  PythonInterpreter::addThread();

  // Connect to the database
  DatabaseWriter* writer = static_cast<DatabaseWriter*>(arg);
  PGconn *conn = PQconnectdb(writer->connectionstring.c_str());
  if (PQstatus(conn) != CONNECTION_OK)
  {
    logger << "Error: Connection to database failed: " << PQerrorMessage(conn) << endl;
    PQfinish(conn);
    PythonInterpreter::deleteThread();
    return 0;
  }

  // Switch to autocommit
  PGresult* res = PQexec(conn, "set autocommit = on");
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    logger << "Error: Database autocommit failed: " << PQerrorMessage(conn) << endl;
    PQfinish(conn);
    PythonInterpreter::deleteThread();
    return 0;
  }
  PQclear(res);


      for (int i = 0; i < 1000; ++i)
      {

        DatabaseWriter::pushStatement("Insert into writer (message) values ('message')");
      }
      logger << "added" << endl;

  // Endless loop
  while (true)
  {
    // Wait for first message in the queue
    //Sleep(1000);  // TODO Windows only, sleep 10 seconds

    // Loop while we have commands in the queue
    string stmt;
    logger << "START processing" << endl;
    clock_t begin = clock();
    while (true)
    {
#if defined(HAVE_PTHREAD_H)
      // Verify whether there has been a cancellation request in the meantime
      pthread_testcancel();
#endif
      // Pick up a statement
      stmt = writer->popStatement();
      if (stmt.empty()) break; // Queue is empty

      // Execute the statement
      PGresult* res = PQexec(conn, stmt.c_str());
      if (PQresultStatus(res) != PGRES_COMMAND_OK)
      {
        logger << "Error: Database statement failed: " << PQerrorMessage(conn) << endl;
        logger << "  Statement: " << stmt << endl;
      }
      PQclear(res);
    }    // While queue not empty

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    logger << "END processing " << end << "   " << begin << "  " << elapsed_secs << endl;
  };  // Infinite loop till thread is cancelled

  // Finalize
  PQfinish(conn);
  PythonInterpreter::deleteThread();
  writeSingleton = NULL;
  return 0;
}

}       // end namespace
