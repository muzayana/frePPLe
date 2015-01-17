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

int WebServer::websocket_chat(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  // Receive the chat message
  Date now = Date::now();
  SerializerJSONString o;
  o.writeString("{\"category\": \"chat\", \"messages\": [{");
  o.writeElement(Tags::tag_name, clnt->getUsername());
  o.writeElement(Tags::tag_value, data + 6);
  o.writeElement(Tags::tag_date, now);
  o.writeString("}]}");

  // Store the chat message in the database
  stringstream sql;
  sql << "INSERT INTO planningboard_chat (user_id, message, lastmodified) VALUES ("
    << clnt->getUserId() << ", $1, now())";
  DatabaseWriter::pushStatement(sql.str(), data+6);

  // Broadcast the message to all clients
  WebClient::lock();
  for (WebClient::clientmap::iterator i = WebClient::getClients().begin(); i != WebClient::getClients().end(); ++i)
    mg_websocket_write( i->first, WEBSOCKET_OPCODE_TEXT, o.getData().c_str(), o.getData().size() );
  WebClient::unlock();
  return 1;
}

} // end namespace
