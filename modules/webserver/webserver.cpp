/***************************************************************************
 *                                                                         *
 * Copyright (C) 2014 by frePPLe bvba                                      *
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

namespace module_webserver
{
  
PythonFunction PublisherBase::registrationHook;
PythonFunction PublisherBase::unregistrationHook;

template<class T> map<T*, Publisher<T> > Publisher<T>::objects;

PyObject* runWebServer (PyObject* self, PyObject* args, PyObject* kwds)
{
  // Define callback functions
  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.websocket_connect = WebServer::connect_callback;
  callbacks.websocket_ready = WebServer::ready_callback;
  callbacks.websocket_data = WebServer::data_callback;
  callbacks.connection_close = WebServer::close_callback;

  // Build all options
  const char *options[] = {
    "document_root", ".",
    "listening_ports", "8001",
    "num_threads", "10",
    "enable_directory_listing", "no",
    "request_timeout_ms", "600000",
    "error_log_file", "server_error.log",
    "access_log_file", "access_log",
    "ssl_certificate", 0,
    0
    };
  string document_root;
  string listening_ports;
  string num_threads;
  string enable_directory_listing;
  string request_timeout_ms;
  string ssl_certificate;
  string access_log_file;
  string error_log_file;
  string database_connection;
  PyObject* pyobj = PyDict_GetItemString(kwds,"document_root");
  if (pyobj)
  {
    PythonData d(pyobj);
    document_root = d.getString();
    options[1] = document_root.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"listening_ports");
  if (pyobj)
  {
    PythonData d(pyobj);
    listening_ports = d.getString();
    options[3] = listening_ports.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"num_threads");
  if (pyobj)
  {
    PythonData d(pyobj);
    num_threads = d.getString();
    options[5] = num_threads.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"enable_directory_listing");
  if (pyobj)
  {
    PythonData d(pyobj);
    enable_directory_listing = d.getString();
    options[7] = enable_directory_listing.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"request_timeout_ms");
  if (pyobj)
  {
    PythonData d(pyobj);
    request_timeout_ms = d.getString();
    options[9] = request_timeout_ms.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"error_log_file");
  if (pyobj)
  {
    PythonData d(pyobj);
    error_log_file = d.getString();
    options[11] = error_log_file.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"access_log_file");
  if (pyobj)
  {
    PythonData d(pyobj);
    access_log_file = d.getString();
    options[13] = access_log_file.c_str();
  }
  pyobj = PyDict_GetItemString(kwds,"ssl_certificate");
  if (pyobj)
  {
    PythonData d(pyobj);
    ssl_certificate = d.getString();
    options[15] = ssl_certificate.c_str();
  }
  else
    options[14] = NULL;
  pyobj = PyDict_GetItemString(kwds,"max_websocket_clients");
  if (pyobj)
  {
    PythonData d(pyobj);
    WebClient::setMaxClients(d.getUnsignedLong());
  }
  pyobj = PyDict_GetItemString(kwds,"secret_key");
  if (pyobj)
  {
    PythonData d(pyobj);
    WebClient::setSecretKey(d.getString());
  }
  else
  {
    PyErr_SetString(PythonDataException, "Required argument 'secret_key' missing");
    return NULL;
  }

  // Start a database connection, if a connection string is provided
  pyobj = PyDict_GetItemString(kwds,"database_connection");
  if (pyobj)
  {
    PythonData d(pyobj);
    database_connection = d.getString();
    if (!database_connection.empty())
    {
      DatabaseWriter::setConnectionString(database_connection);
      // Launch writer
      DatabaseWriter::launchWriter(database_connection);
      // Load chat history
      WebServer::loadChatHistory(database_connection);
      WebServer::setConnectionString(database_connection);
    }
  }

  // Start server
  bool exitNow = false;
  CivetServer server(options, &callbacks);
  WebServer handler(&exitNow);
  server.addHandler("**", &handler);

  // Wait for the server to shut down
#ifdef _WIN32
  while (!exitNow) Sleep(1000);
#else
  while (!exitNow) sleep(1);
#endif
  return Py_BuildValue("");
}



int WebServer::connect_callback(const struct mg_connection *conn)
{
  // If the connection limit is reached, we reply 1. The server then aborts
  // the handshake to open the connection.
  return WebClient::authenticate(conn) ? 0 : 1;
}


void WebServer::ready_callback(struct mg_connection *conn)
{
  WebClient::addClient(conn);
}


void WebServer::close_callback(const struct mg_connection *conn)
{
  // This handler is not specific to websockets, and is also called for
  // HTTP connections.
  if (mg_get_header(conn, "Sec-WebSocket-Key"))
    WebClient::removeClient(conn);
}


int WebServer::data_callback(struct mg_connection *conn, int bits,
  char *data, size_t data_len)
{
  // Verify validity of the request
  WebClient *clnt = WebClient::getClient(conn);
  if (!data || !clnt) return 1;
  if (Date::now() > clnt->getExpires())
  {
    WebClient::removeClient(conn);
    return 1;
  }

  // Make sure the data is null terminated. Avoids trouble with the string
  // comparison functions.
  data[data_len] = 0;

  // Logging
  logger << "receiving: " << data << endl;

  // Main action dispatcher for websocket data
  if (!strncmp(data, "/get/", 5))
    // - /get/demand/ /get/item/ /get/resource/ /get/buffer/ /get/operation/
    //   Get a list of object names
    return websocket_get(conn, bits, data, data_len, clnt);
  else if (!strncmp(data, "/plan/", 6))
    // - /plan/demand/x /plan/resource/x /plan/buffer/x /plan/operation/x 
    //   Return the plan of an object
    return websocket_plan(conn, bits, data, data_len, clnt);
  else if (!strncmp(data, "/solve/", 7))
    // - /solve/erase/ /solve/replan/ /solve/demand/x
    //   Plan generation functions
    return websocket_solve(conn, bits, data, data_len, clnt);
  else if (!strncmp(data, "/chat/", 6))
    // - /chat/
    //   Sending and receiving chat messages
    return websocket_chat(conn, bits, data, data_len, clnt);
  else if (!strncmp(data, "/register/", 10))
    // - /reg/demand/x /reg/resource/x /reg/buffer/x /reg/operation/x /reg/item/x
    //   Register an entity, return its plan  
    return websocket_register(conn, bits, data, data_len, clnt);
  else if (!strncmp(data, "/unregister/", 12))
    // - /unreg/demand/x /unreg/resource/x /unreg/buffer/x /unreg/operation/x /unreg/item/x
    //   Unregister an entity    
    return websocket_unregister(conn, bits, data, data_len, clnt);
  else if (!strncmp(data, "/status/", 7))
  {
    // Get the status of the web server   TODO Remove and replace with better logging and status...
    mg_lock_connection(conn);
    char msg[] = "<?xml version='1.0' encoding='UTF-8' ?><status>Echoed status to log file</status>";
    WebClient::printStatusAll();
    mg_websocket_write( conn, WEBSOCKET_OPCODE_TEXT, msg, strlen(msg) );
    mg_unlock_connection(conn);
  }
  return 1;
}


int WebServer::websocket_register(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  mg_lock_connection(conn);
  if (!strncmp(data+10, "demand/", 7))
  {
    string name(data+17);
    Publisher<Demand>::registerSubscriber(clnt, name);
  }
  else if (!strncmp(data+10, "resource/", 9))
  {
    string name(data+19);
    Publisher<Resource>::registerSubscriber(clnt, name);
  }
  else if (!strncmp(data+10, "buffer/", 7))
  {
    string name(data+17);
    Publisher<Buffer>::registerSubscriber(clnt, name);
  }
  else if (!strncmp(data+10, "operation/", 10))
  {
    string name(data+20);
    Publisher<Operation>::registerSubscriber(clnt, name);
  }
  mg_unlock_connection(conn);
  return 1;
}


int WebServer::websocket_unregister(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  mg_lock_connection(conn);
  if (!strncmp(data+12, "demand/", 7))
  {
    string name(data+19);
    Publisher<Demand>::unregisterSubscriber(clnt, name);
  }
  else if (!strncmp(data+12, "resource/", 9))
  {
    string name(data+21);
    Publisher<Resource>::unregisterSubscriber(clnt, name);
  }
  else if (!strncmp(data+12, "buffer/", 7))
  {
    string name(data+19);
    Publisher<Buffer>::unregisterSubscriber(clnt, name);
  }
  else if (!strncmp(data+12, "operation/", 10))
  {
    string name(data+22);
    Publisher<Operation>::unregisterSubscriber(clnt, name);
  }
  mg_unlock_connection(conn);
  return 1;
}


}       // end namespace
