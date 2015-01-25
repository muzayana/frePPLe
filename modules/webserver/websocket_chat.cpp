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

list<string> WebServer::history;


void WebServer::loadChatHistory(const string& c)
{
  DatabaseReader db(c);
  DatabaseReader::DatabaseResult res = db.fetchSQL(DatabaseStatement(
    "select username, message, planningboard_chat.lastmodified::timestamp without time zone "
    "from planningboard_chat "
    "inner join common_user on planningboard_chat.user_id = common_user.id "
    "order by planningboard_chat.id desc "
    "limit 100"));
  for (int i = res.countRows()-1; i >= 0; --i)
  {
    SerializerJSONString o;
    o.writeString("{");
    o.writeElement(Tags::tag_name, res.getValueString(i, 0));
    o.writeElement(Tags::tag_value, res.getValueString(i, 1));
    o.writeElement(Tags::tag_date, res.getValueDate(i, 2));
    o.writeString("}");
    history.push_back(o.getData().c_str());
  }
  for (list<string>::const_iterator j = history.begin(); j != history.end(); ++j)
    logger << "chat " << *j << endl;
}


int WebServer::websocket_chat(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  // Receive and serialize the chat message
  Date now = Date::now();
  SerializerJSONString o1;
  o1.writeString("{");
  o1.writeElement(Tags::tag_name, clnt->getUsername());
  o1.writeElement(Tags::tag_value, data + 6);
  o1.writeElement(Tags::tag_date, now);
  o1.writeString("}");
  stringstream o2;
  o2 << "{\"category\": \"chat\", \"messages\": [" << o1.getData() << "]}";

  // Append to the chat history in memory
  history.push_back(o1.getData());
  while (history.size() > 100)
    history.pop_front();

  // Store the chat message in the database
  DatabaseWriter::pushStatement(
    "INSERT INTO planningboard_chat "
      "(user_id, message, lastmodified) "
      "VALUES ($1, $2, now())",
    clnt->getUserId(),
    data+6);

  // Broadcast the message to all clients
  WebClient::lock();
  for (WebClient::clientmap::iterator i = WebClient::getClients().begin(); i != WebClient::getClients().end(); ++i)
    mg_websocket_write( i->first, WEBSOCKET_OPCODE_TEXT, o2.str().c_str(), o2.str().size() );
  WebClient::unlock();
  return 1;
}

} // end namespace
