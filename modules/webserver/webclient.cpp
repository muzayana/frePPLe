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

#include <iomanip>
#include "webserver.h"
#include <openssl/sha.h>


namespace module_webserver
{

WebClient::clientmap WebClient::clients;
Mutex WebClient::lck;
unsigned long WebClient::max_clients = 20;
string WebClient::secret_key;

PythonFunction PublisherBase::registrationHook;
PythonFunction PublisherBase::unregistrationHook;

Publisher<Resource>::objectmap Publisher<Resource>::objects;
Publisher<Buffer>::objectmap Publisher<Buffer>::objects;
Publisher<Demand>::objectmap Publisher<Demand>::objects;
Publisher<Operation>::objectmap Publisher<Operation>::objects;


bool WebClient::authenticate(const struct mg_connection* conn)
{
  // Assure the maximum number of connections isn't exceeded.
  {
  ScopeMutexLock only_me(lck);
  if (clients.size() >= max_clients)
    return false;
  }

  // Verify the token
  string username;
  string token;
  string timestamp;
  if (!CivetServer::getParam(const_cast<struct mg_connection*>(conn), "user", username, 0)
    || !CivetServer::getParam(const_cast<struct mg_connection*>(conn), "token", token, 0)
    || !CivetServer::getParam(const_cast<struct mg_connection*>(conn), "time", timestamp, 0))
    // We're missing at least one of the query string parameters
    return false;
  else
  {
    // Compute expected token
    string data = username + timestamp + WebClient::getSecretKey();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);
    stringstream s;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
      s << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    if (s.str() != token)
      // Oh dear, the token isn't matching
      return false;
  }

  // Session shouldn't be expired yet
  Date expires = atol(timestamp.c_str());
  if (Date::now() > expires) return false;

  // All clear now...
  return true;
}


void WebClient::addClient(struct mg_connection* conn)
{
  // Exclusive access
  ScopeMutexLock only_me(lck);

  // Look up in the list, and exit if the connection already exists
  clientmap::iterator it = clients.find(conn);
  if (it != clients.end()) return;

  // Add into the list
  clients[conn] = WebClient(conn); // Note: we copy a client object here

  // Pick up the user name from the URI
  CivetServer::getParam(conn, "user", clients[conn].username, 0);

  // Pick up the timestamp from the URI
  string timestamp;
  CivetServer::getParam(conn, "time", timestamp, 0);
  clients[conn].expires = atol(timestamp.c_str());
}


void WebClient::removeClient(struct mg_connection* conn)
{
  // Exclusive access
  ScopeMutexLock only_me(lck);

  // Look up in the map, and remove
  clientmap::iterator it = clients.find(conn);
  if (it != clients.end())
    clients.erase(it);
}


void WebClient::printStatusAll()
{
  logger << "Web client status:" << endl;
  for (clientmap::const_iterator i = clients.begin(); i != clients.end(); ++ i)
    i->second.printStatus();
}


void WebClient::printStatus() const
{
  // Common info
  struct mg_request_info *rq = mg_get_request_info(conn);
  logger << "Connection with " << rq->remote_ip << " " << username << " subscribed to :" << endl;

  // List of subscriptions
  for (subscriptionlist::const_iterator i = subscriptions.begin(); i!=subscriptions.end(); ++i)
    logger << "   " << i->getName() << endl;
}


void WebClient::notify()
{
  // TODO persist all subscriptions of this webclient
  logger << "notification " << username << endl;
}

}       // end namespace
