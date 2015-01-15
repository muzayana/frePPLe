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

/** @file database.h
  * @brief Header file for reading and writing to PostgreSQL databases.
  */

#ifndef DATABASE_H
#define DATABASE_H

#ifdef POSTGRESQL_LIBPQ_FE_H
#include <postgresql/libpq-fe.h>
#else
#include <libpq-fe.h>
#endif

#include "frepple.h"
using namespace frepple;
#include <deque>

namespace module_webserver
{

/** @brief This class implements a queue that is writing results
  * into a PostgreSQL database.
  */
class DatabaseWriter
{
  public:
    /** Add a new statement to the queue. */
    static void pushStatement(string);    

    /** Initialize the writer thread. */
    static void initialize(const string&);

  private:
    /** Constructor. */
    DatabaseWriter();

    /** This functions runs a loop that executes all statements. */
#if defined(HAVE_PTHREAD_H)
    static void* writethread(void *arg);
#else
    static unsigned __stdcall writethread(void *);
#endif

    /** Queue of statements. */
    deque<string> statements;

    /** Lock to assure the queue is manipulated only from a single thread. */
    Mutex lock;

    /** Pop a statement from the queue. */
    string popStatement();

    /** Database connection string. */
    static string connectionstring;

    /** Singleton instance of this class. */
    static DatabaseWriter* writeSingleton;
};

}   // End namespace

#endif


