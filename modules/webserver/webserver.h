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

/** @file webserver.h
  * @brief Header file for the module webserver.
  *
  * @namespace module_webserver
  * @brief Module for running an HTTP and websocket server.
  *
  * The web server is started with the command frepple.runWebServer().
  * The command accepts the following arguments:
  *
  *   - document_root<br>
  *     A directory to serve static files from.<br>
  *     The default value is ".".
  *
  *   - listening_ports<br>
  *     Comma-separated list of ports to listen on.<br>
  *     The default value is "8001".
  *
  *   - num_threads<br>
  *     Number of worker threads. Each incoming connection is handled in a
  *     separate thread. Therefore, this value is effectively a number of
  *     concurrent connections the server can handle.<br>
  *     The default value is 10.
  *
  *   - enable_directory_listing<br>
  *     Enable directory listing in the static file folders, either yes or
  *     no.<br>
  *     The default value is "no".
  *
  *   - request_timeout_ms<br>
  *     Timeout for network read and network write operations, expressed
  *     in milliseconds.<br>
  *     The default value is 600000.
  *
  *   - error_log_file<br>
  *     Path to a file for error logs.<br>
  *     The default value is "server_error.log".
  *
  *   - access_log_file<br>
  *     Path to a file for access logs.<br>
  *     By default the access is not logged.
  *
  *   - max_websocket_clients<br>
  *     The maximum number of websocket clients that can connect to the server.<br>
  *     The default value is 20.
  *
  *   - secret_key<br>
  *     Secret string used to validate login tokens.
  */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#ifdef POSTGRESQL_LIBPQ_FE_H
#include <postgresql/libpq-fe.h>
#else
#include <libpq-fe.h>
#endif

#include "frepple.h"
using namespace frepple;

#include "CivetServer.h"

namespace module_webserver
{

/** Initialization routine for the library. */
MODULE_EXPORT const char* initialize(const Environment::ParameterList&);


/** @brief This Python function runs the embedded HTTP web server. */
PyObject* runWebServer(PyObject*, PyObject*, PyObject*);


/** @brief This Python function runs a thread to persist data into a PostgreSQL database. */
PyObject* runDatabaseThread(PyObject*, PyObject*, PyObject*);


// Forward definitions
class Subscription;
class WebClient;
class PublisherBase;


/** @brief This class implements a queue that is writing results
  * into a PostgreSQL database.
  */
class DatabaseWriter
{
  friend PyObject* runDatabaseThread(PyObject*, PyObject*, PyObject*);
  public:
    /** Add a new statement to the queue. */
    static void pushStatement(string);

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


/** A class to store information about a websocket connection. */
class WebClient : public Association<PublisherBase,WebClient,Subscription>::ListB
{
  public:
    typedef Association<PublisherBase,WebClient,Subscription>::ListB subscriptionlist;
    typedef map<struct mg_connection*, WebClient> clientmap;

    /** Default constructor. */
    WebClient() : conn(NULL), username("anonymous"), expires(Date::infiniteFuture) {}

    /** Register a new client.
      * Before adding a client to the list, this method calls the
      * authenticate method. If it returns false, the client isn't added.
      */
    static void addClient(struct mg_connection* conn);

    /** Unregister a client. */
    static void removeClient(struct mg_connection* conn);

    /** Verify whether the client limit is reached or not. */
    static bool authenticate(const struct mg_connection* conn);

    /** Return the username. */
    string getUsername() const {return username;}

    /** Return the expiration date of the connection. */
    Date getExpires() const {return expires;}

    /** Return the connection pointer. */
    const struct mg_connection* getConnection() const {return conn;}

    /** Update the maximum number of websocket clients.
      * The default value is 20.
      */
    static void setMaxClients(unsigned long i) { max_clients = i; }

    /** Find the WebClient object for a connection. */
    static WebClient* getClient(struct mg_connection* c)
    {
      ScopeMutexLock only_me(lck);
      clientmap::iterator it = clients.find(c);
      return (it == clients.end()) ? NULL : &(it->second);
    }

    /** Return the secret key. */
    static string getSecretKey() {return secret_key;}

    /** Update the secret key. */
    static void setSecretKey(string k) {secret_key = k;}

    /** Print the status of all connections. */
    static void printStatusAll();

    /** Print the status of this connection. */
    void printStatus() const;

    /** Get the list of subscriptions.
      * Make sure to lock the access when using the list!
      */
    subscriptionlist& getSubscriptions() {return subscriptions;}

    /** Get the list of clients.
      * Make sure to lock the access when using the list!
      */
    static clientmap& getClients() {return clients;}

    /** This method is called when a subscription is added or removed.
      * The method is used to persist the subscription list.
      */
    void notify();

    /** Assure we are the only one accessing the subscription information. */
    static void lock() {lck.lock();}

    /** Free the subscription information for others. */
    static void unlock() {lck.unlock();}

  private:
    /** List of publishers this connection listens to. */
    subscriptionlist subscriptions;

    /** Constructor. */
    WebClient(struct mg_connection* c) : conn(c) {}

    /** List of all connected web clients. */
    static clientmap clients;

    /** Mutex lock to enforce serial access to the client list. */
    static Mutex lck;

    /** Maximum number of websocket clients.
      * The default value is 20.
      */
    static unsigned long max_clients;

    /** Secret key to validate login tokens. */
    static string secret_key;

    /** Pointer to the connection. */
    struct mg_connection* conn;

    /** User name of this connection. */
    string username;

    /** Expiration date of this connection. */
    Date expires;
};


/** Objects which can be subscribed to. TODO Rework to make it follow a proxy pattern. */
class PublisherBase : public Association<PublisherBase,WebClient,Subscription>::ListA
{
  public:
    /** Constructor. */
    PublisherBase(Object* o = NULL) : owner(o) {};

    typedef Association<PublisherBase,WebClient,Subscription>::ListA subscriberlist;

    /** Get the list of subscribers. */
    subscriberlist& getSubscribers() {return subscribers;}

    /** Specify a Python function that is called after registering an object. */
    DECLARE_EXPORT void setRegistrationHook(PyObject* p) {registrationHook = p;}

    /** Return the Python function that is called after registering an object. */
    PythonFunction getRegistrationHook() const {return registrationHook;}

    /** Specify a Python function that is called after unregistering an object. */
    DECLARE_EXPORT void setUnregistrationHook(PyObject* p) {unregistrationHook = p;}

    /** Return the Python function that is called after unregistering an object. */
    PythonFunction getUnregistrationHook() const {return unregistrationHook;}

    /** Return the proxy owner. */
    Object* getOwner() {return owner;}

  private:
    subscriberlist subscribers;
    Object* owner;

  protected:
    static PythonFunction registrationHook;
    static PythonFunction unregistrationHook;
};


template <class T> class Publisher : public PublisherBase
{
  private:
    typedef map<T*, Publisher<T> > objectmap;
    static objectmap objects;

  public:

    /** Constructor. */
    Publisher(T* o = NULL) : PublisherBase(o) {};

    static void registerSubscriber(WebClient* clt, string nm)
    {
      // Find the object
      T *d = T::find(nm);
      if (!d) return;

      // Find or create a publisher
      typename objectmap::iterator i = objects.find(d);
      Publisher<T> *pub = NULL;
      if (i == objects.end())
      {
        objects[d] = Publisher<T>(d);
        pub = &(objects.find(d)->second);
      }
      else
        pub = &(i->second);

      // Add subscription
      new Subscription(clt, pub, nm);

      // Call Python registration hook
      // TODO registration hook will need to know the user name
      if (registrationHook) registrationHook.call(d);
    }


    static void unregisterSubscriber(WebClient* clt, string nm)
    {
      // Find the object
      T *d = T::find(nm);
      if (!d) return;

      // Find the publisher
      typename objectmap::iterator i = objects.find(d);
      if (i == objects.end()) return;

      // Remove subscription
      delete clt->getSubscriptions().find(&(i->second));

      // Call Python unregistration hook
      // TODO unregistration hook will need to know the user name
      if (unregistrationHook) unregistrationHook.call(d);
    }
};


/** Subscription link between a web client and a publisher. */
class Subscription : public Association<PublisherBase,WebClient,Subscription>::Node
{
  // TODO the subscription should store extra information, such as "row number", "format", "height", ... Or just a json-string with attributes? Or leave this to the database/python layer?
  public:
    /** Constructor. */
    Subscription(WebClient* w, PublisherBase* p, string n)
    {
      if (!w || !p) throw LogicException("Creation NULL subscription");
      setPtrA(p, p->getSubscribers());
      setPtrB(w, w->getSubscriptions());
      setName(n);
      getWebClient()->notify();
    }

    /** Destructor. */
    ~Subscription()
    {
      getPublisher()->getSubscribers().erase(this);
      getWebClient()->getSubscriptions().erase(this);
      getWebClient()->notify();
    }

    /** Return the publisher. */
    PublisherBase* getPublisher() { return getPtrA(); }

    /** Return the web client. */
    WebClient* getWebClient() { return getPtrB(); }
};


/** A class to process requests to the embedded HTTP and websocket server. */
class WebServer : public CivetHandler
{
  public:
    /** Constructor. */
    WebServer(bool* e) : exitNow(e) {}

    /** Main dispatcher for HTTP GET. */
    bool handleGet(CivetServer *server, struct mg_connection *conn);

    /** Main dispatcher for all incoming websocket data. */
    static int data_callback(struct mg_connection*, int, char*, size_t);

    /** Callback function when a websocket client is connecting. */
    static int connect_callback(const struct mg_connection *conn);

    /** Callback function when the handshaking with a websocket client is
      * succesfully completed.
      */
    static void ready_callback(struct mg_connection *conn);

    /** Callback function when a websocket client is disconnecting. */
    static void close_callback(struct mg_connection *conn);

  private:
    /** Flag to trigger shutting down the server. */
    bool *exitNow;

    /** Dispatcher for websocket data in the form: /get/ */
    static int websocket_get(struct mg_connection*, int, char*, size_t, WebClient*);

    /** Dispatcher for websocket data in the form: /chat/ */
    static int websocket_chat(struct mg_connection*, int, char*, size_t, WebClient*);

    /** Dispatcher for websocket data in the form: /plan/ */
    static int websocket_plan(struct mg_connection*, int, char*, size_t, WebClient*);

    /** Dispatcher for websocket data in the form: /solve/ */
    static int websocket_solve(struct mg_connection*, int, char*, size_t, WebClient*);

    /** Dispatcher for websocket data in the form: /register/ */
    static int websocket_register(struct mg_connection*, int, char*, size_t, WebClient*);

    /** Dispatcher for websocket data in the form: /unregister/ */
    static int websocket_unregister(struct mg_connection*, int, char*, size_t, WebClient*);
};

}   // End namespace

#endif


