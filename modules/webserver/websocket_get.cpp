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

#include "json.h"
#include "webserver.h"


namespace module_webserver
{

int WebServer::websocket_get(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  static Keyword tag_messages("messages");
  SerializerJSONString o;
  o.setContentType(Serializer::STANDARD);
  o.writeString("{\"category\": \"name\",");

  // Item
  if (data_len == 5 || !strncmp(data+5, "item/", 5))
  {
    o.BeginList(Tags::items);
    for (Item::iterator it = Item::begin(); it != Item::end(); ++it)
      o.writeElement(Tags::item, Tags::name, it->getName());
    o.EndList(Tags::items);
  }

  // Resources
  if (data_len == 5 || !strncmp(data+5, "resource/", 9))
  {
    o.BeginList(Tags::resources);
    for (Resource::iterator res = Resource::begin(); res != Resource::end(); ++res)
      o.writeElement(Tags::resource, Tags::name, res->getName());
    o.EndList(Tags::resources);
  }

  // Buffer
  if (data_len == 5 || !strncmp(data+5, "buffer/", 7))
  {
    o.BeginList(Tags::buffers);
    for (Buffer::iterator bf = Buffer::begin(); bf != Buffer::end(); ++bf)
      o.writeElement(Tags::buffer, Tags::name, bf->getName());
    o.EndList(Tags::buffers);
  }

  // Operation
  if (data_len == 5 || !strncmp(data+5, "operation/", 10))
  {
    o.BeginList(Tags::operations);
    for (Operation::iterator op = Operation::begin(); op != Operation::end(); ++op)
      o.writeElement(Tags::operation, Tags::name, op->getName());
    o.EndList(Tags::operations);
  }

  // Demand
  if (data_len == 5 || !strncmp(data+5, "demand/", 7))
  {
    o.BeginList(Tags::demands);
    for (Demand::iterator dm = Demand::begin(); dm != Demand::end(); ++dm)
    {
      //dm->writeElement(&o, Tags::demand);
      //o.writeElement(Tags::demand, Tags::name, dm->getName());
      o.BeginObject(Tags::demand, Tags::name, dm->getName());
      o.writeElement(Tags::customer, dm->getCustomer()->getName());
      o.writeElement(Tags::quantity, dm->getQuantity());
      o.writeElement(Tags::item, dm->getItem()->getName());
      o.writeElement(Tags::due, dm->getDue());
      o.writeElement(Tags::priority, dm->getPriority());
      o.EndObject(Tags::demand);
    }
    o.EndList(Tags::demands);
  }

  // Chat history
  if (data_len == 5 && !WebServer::history.empty())
  {
    o.BeginList(tag_messages);
    bool first = true;
    for (list<string>::const_iterator j = history.begin(); j != history.end(); ++j)
    {
      if (first)
        first = false;
      else
        o.writeString(",");
      o.writeString(*j);
    }
    o.EndList(tag_messages);
  }

  // Send the result
  o.writeString("}");
  mg_websocket_write( conn, WEBSOCKET_OPCODE_TEXT, o.getData().c_str(), o.getData().size() );
  return 1;
}


int WebServer::websocket_plan(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  SerializerJSONString o;
  bool ok = true;
  o.setReferencesOnly(true);
  o.setContentType(Serializer::PLANDETAIL);
  o.writeString("{\"category\": \"plan\", ");

  if (!strncmp(data+6, "demand/", 7))
  {
    string name(data+13);
    Demand *dmd = Demand::find(name);
    if (dmd)
    {
      o.BeginList(Tags::demands);
      dmd->writeElement(&o, Tags::demand);
      o.EndList(Tags::demands);
    }
    else
      // Not found
      ok = false;
  }
  else if (!strncmp(data+6, "resource/", 9))
  {
    string name(data+15);
    Resource *res = Resource::find(name);
    if (res)
    {
      o.BeginList(Tags::resources);
      res->writeElement(&o, Tags::resource);
      o.EndList(Tags::resources);
    }
    else
      // Not found
      ok = false;
  }
  else if (!strncmp(data+6, "buffer/", 7))
  {
    string name(data+13);
    Buffer *buf = Buffer::find(name);
    if (buf)
    {
      o.BeginList(Tags::buffers);
      buf->writeElement(&o, Tags::buffer);
      o.EndList(Tags::buffers);
    }
    else
      // Not found
      ok = false;
  }
  else if (!strncmp(data+6, "operation/", 10))
  {
    string name(data+16);
    Operation *oper = Operation::find(name);
    if (oper)
    {
      o.BeginList(Tags::operations);
      oper->writeElement(&o, Tags::operation);
      o.EndList(Tags::operations);
    }
    else
      // Not found
      ok = false;
  }
  else
    // Don't know this type
    ok = false;
  if (ok)
  {
    o.EndObject(Tags::plan);
    mg_websocket_write( conn, WEBSOCKET_OPCODE_TEXT, o.getData().c_str(), o.getData().size() );
  }
  return 1;
}


int WebServer::websocket_solve(struct mg_connection *conn, int bits,
  char *data, size_t data_len, WebClient* clnt)
{
  Demand *changedDemand = NULL;
  int demandChanges = 0; // 0: None, 1: single demand, 2: all

  if (!strncmp(data+7, "erase/", 6))
  {
    // Erase the previous plan
    logger << "Deleting the plan" << endl;
    for (Operation::iterator e=Operation::begin(); e!=Operation::end(); ++e)
      e->deleteOperationPlans();
    demandChanges = 2;
  }
  else if (!strncmp(data+7, "replan/backward/", 15))
  {
    // Regenerate the plan
    logger << "Completing the plan in backward planning mode" << endl;
    SolverMRP solver;
    solver.setConstraints(15);
    // TODO pick up plan type arguments from the command
    // TODO During this planning no other users should connect or use the planboard
    // Plan types:
    // - 1: Constrained plan.<br>
    // - 2: Unconstrained plan with alternate search.<br>
    // - 3: Unconstrained plan without alternate search.<br>
    solver.setPlanType(1);
    solver.setLogLevel(1);
    solver.setErasePreviousFirst(false);
    solver.solve();
    demandChanges = 2;
  }
  else if (!strncmp(data+7, "replan/forward/", 14))
  {
    // Regenerate the plan
    logger << "Completing the plan in backward planning mode" << endl;
    Duration delta(86400L * 3650L);
    for (Demand::iterator it = Demand::begin(); it != Demand::end(); ++it)
      it->setDue(it->getDue() - delta);
    SolverMRP solver;
    solver.setConstraints(15);
    // TODO pick up plan type arguments from the command
    // TODO During this planning no other users should connect or use the planboard
    // Plan types:
    // - 1: Constrained plan.<br>
    // - 2: Unconstrained plan with alternate search.<br>
    // - 3: Unconstrained plan without alternate search.<br>
    solver.setPlanType(1);
    solver.setLogLevel(1);
    solver.setErasePreviousFirst(false);
    solver.solve();
    for (Demand::iterator it = Demand::begin(); it != Demand::end(); ++it)
      it->setDue(it->getDue() + delta);
    demandChanges = 2;
  }
  else if (!strncmp(data+7, "demand/backward/", 16))
  {
    string name(data+23);
    Demand *dem = Demand::find(name);
    if (dem)
    {
      logger << "Planning demand '" << name << "' in backward mode" << endl;
      // Remove existing plan
      OperatorDelete unplan;
      unplan.solve(dem);
      // Create new plan
      SolverMRP solver;
      solver.setConstraints(15);
      solver.setPlanType(1);
      solver.setLogLevel(2);
      dem->solve(solver, &solver.getCommands());
      demandChanges = 1;
      changedDemand = dem;
    }
  }
  else if (!strncmp(data+7, "demand/forward/", 15))
  {
    string name(data+22);
    Demand *dem = Demand::find(name);
    if (dem)
    {
      logger << "Planning demand '" << name << "' in forward mode" << endl;
      Duration delta(86400L * 3650L);
      dem->setDue(dem->getDue() - delta);
      // Remove existing plan
      OperatorDelete unplan;
      unplan.solve(dem);
      // Create new plan
      SolverMRP solver;
      solver.setConstraints(15);
      solver.setPlanType(1);
      dem->solve(solver, &solver.getCommands());
      dem->setDue(dem->getDue() + delta);
      demandChanges = 1;
      changedDemand = dem;
    }
  }
  else if (!strncmp(data+7, "unplan/", 7))
  {
    string name(data+14);
    Demand *dem = Demand::find(name);
    if (dem)
    {
      logger << "Unplanning demand '" << name << "'" << endl;
      // Remove existing plan
      OperatorDelete unplan;
      unplan.solve(dem);
      demandChanges = 1;
      changedDemand = dem;
    }
  }

  // TODO Collect changed entities during the solving methods

  // Push the changes to all subscribed clients
  // TODO Currently we refresh every subscribed object to each client. We should only refresh the subscribed elements that have changed.
  // TODO First publish to the client who submitted the change, then the others (ideally asynchronous).
  WebClient::lock();
  for (WebClient::clientmap::iterator i = WebClient::getClients().begin(); i != WebClient::getClients().end(); ++i)
  {
    struct mg_request_info *rq = mg_get_request_info(i->first);
    SerializerJSONString o;
    bool ok = true;
    o.setReferencesOnly(true);
    o.setContentType(Serializer::PLANDETAIL);
    o.writeString("{\"category\": \"plan\", ");
    bool first = true;
    for (WebClient::subscriptionlist::iterator j = i->second.getSubscriptions().begin();
      j != i->second.getSubscriptions().end(); ++j)
    {
      if (j->getPublisher()->getOwner()->getType().category != Resource::metadata)
        continue;
      if (first)
      {
        o.BeginList(Tags::resources);
        first = false;
      }
      static_cast<Resource*>(j->getPublisher()->getOwner())->writeElement(&o, Tags::resource);
    }
    if (!first)
      o.EndList(Tags::resources);
    first = true;
    for (WebClient::subscriptionlist::iterator j = i->second.getSubscriptions().begin();
      j != i->second.getSubscriptions().end(); ++j)
    {
      if (j->getPublisher()->getOwner()->getType().category != Buffer::metadata)
        continue;
      if (first)
      {
        o.BeginList(Tags::buffers);
        first = false;
      }
      static_cast<Buffer*>(j->getPublisher()->getOwner())->writeElement(&o, Tags::buffer);
    }
    if (!first)
      o.EndList(Tags::buffers);
    first = true;
    for (WebClient::subscriptionlist::iterator j = i->second.getSubscriptions().begin();
      j != i->second.getSubscriptions().end(); ++j)
    {
      if (j->getPublisher()->getOwner()->getType().category != Operation::metadata)
        continue;
      if (first)
      {
        o.BeginList(Tags::operations);
        first = false;
      }
      static_cast<Operation*>(j->getPublisher()->getOwner())->writeElement(&o, Tags::operation);
    }
    if (!first)
      o.EndList(Tags::operations);
    first = true;
    if (demandChanges != 2)
    {
      // Write planned demand (if there is one) or subscribed demands
      for (WebClient::subscriptionlist::iterator j = i->second.getSubscriptions().begin();
        j != i->second.getSubscriptions().end(); ++j)
      {
        if (j->getPublisher()->getOwner()->getType().category != Demand::metadata)
          continue;
        if (first)
        {
          o.BeginList(Tags::demands);
          first = false;
        }
        Demand * dm = static_cast<Demand*>(j->getPublisher()->getOwner());
        dm->writeElement(&o, Tags::demand);
        if (changedDemand == dm)
          changedDemand = NULL;
      }
      if (changedDemand)
      {
        if (first)
        {
          o.BeginList(Tags::demands);
          first = false;
        }
        changedDemand->writeElement(&o, Tags::demand);
      }
    }
    else
    {
      // Write all demands
      for (Demand::iterator d = Demand::begin(); d != Demand::end(); ++d)
      {
        if (first)
        {
          o.BeginList(Tags::demands);
          first = false;
        }
        d->writeElement(&o, Tags::demand);
      }
    }
    if (!first)
      o.EndList(Tags::demands);
    if (o.getData().size())
    {
      o.EndObject(Tags::plan);
      mg_websocket_write( i->first, WEBSOCKET_OPCODE_TEXT, o.getData().c_str(), o.getData().size() );  // TODO performance check for large model
    }
  }
  WebClient::unlock();
  return 1;
}

}       // end namespace