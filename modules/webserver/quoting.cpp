/***************************************************************************
 *                                                                         *
 * Copyright (C) 2015 by frePPLe bvba                                      *
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


// Used to keep track of demands found in the XML data
list<Demand*> xml_demands;


// Callback to capture the demands in the XML data
void parsingCallback(Object* obj)
{
  if (obj->getType() == *DemandDefault::metadata)
    xml_demands.push_back(static_cast<DemandDefault*>(obj));
}


bool WebServer::quote_or_inquiry(struct mg_connection* conn, bool keepreservation)
{
  struct mg_request_info *request_info = mg_get_request_info(conn);
  char post_data[100 * 1024]; // A fixed buffer of 100K bytes

  /* TODO POST encoded as application/x-www-form-urlencoded
  mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1));
  mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2));
  */

  /** TODO POST encoded as multipart/form-data encoded JSON data. */

  /** POST encoded as multipart/form-data encoded XML data. */
  int post_data_len = mg_read(conn, post_data, sizeof(post_data));

  // Poor man's request data parser: We consider everything between "<plan"
  // and "</plan>" as posted data content.
  /** TODO need to test MultiPartParser
  MultiPartParser parser("----------ThIs_Is_tHe_bouNdaRY_$");
  parser.execute(post_data, post_data_len);
  */
  post_data[post_data_len] = 0;
  char *char_start = post_data;
  while (*char_start && strncmp(char_start, "<plan", 5))
    ++char_start;
  char* tmp = char_start;
  while (*tmp && strncmp(tmp, "</plan>", 7))
    ++tmp;
  if (*tmp)
    *(tmp+7) = 0;

  // Error message if the request parsing failed
  if (!*char_start)
  {
    if (loglevel > 2)
      logger << "Invalid quoting request posted: " << post_data << endl;
    mg_printf(conn,
      "HTTP/1.1 400 Bad Request\r\n"
      "Content-Length: 34\r\n\r\n"
      "Can't find the data in the request"
      );
    return true;
  }
  if (loglevel > 2)
    logger << "Quoting request posted: " << char_start << endl;

  // Parse the XML data, with validation enabled
  xml_demands.clear();
  XMLInputString xml(char_start);
  xml.setUserExitCpp(parsingCallback);
  xml.parse(&Plan::instance(), true);

  // Prepare to collect results
  ostringstream response;
  response << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
  response << "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" << endl;
  response << "<demands>" << endl;

  // Clean up the supply planned for all demands
  OperatorDelete solver_delete;
  solver_delete.setLogLevel(loglevel);
  for (list<Demand*>::iterator dmd = xml_demands.begin(); dmd != xml_demands.end(); ++dmd)
  {
    if (loglevel > 2)
      logger << "Erasing plan of demand " << *dmd << endl;
    solver_delete.solve(*dmd);
  }

  // Plan the list of all demand
  SolverMRP solver_plan;
  solver_plan.setLogLevel(loglevel);
  solver_plan.setAutocommit(false);
  solver_plan.setConstraints(15);
  solver_plan.setPlanType(1);
  XMLSerializer xmlserializer(response);
  xmlserializer.setContentType(DETAIL);
  for (list<Demand*>::iterator dmd = xml_demands.begin(); dmd != xml_demands.end(); ++dmd)
  {
    if (loglevel > 2)
      logger << "Planning demand " << *dmd << endl;
    solver_plan.solve(*dmd, &(solver_plan.getCommands()));
    xmlserializer.pushCurrentObject(*dmd);
    if (keepreservation)
    {
      solver_plan.scanExcess(&(solver_plan.getCommands()));
      solver_plan.getCommands().CommandManager::commit();
      (*dmd)->writeElement(&xmlserializer, Tags::demand, DETAIL);
    }
    else
    {
      (*dmd)->writeElement(&xmlserializer, Tags::demand, DETAIL);
      solver_plan.getCommands().rollback();
    }
  }
  response << "</demands></plan>" << endl;

  // Persist in the database
  for (list<Demand*>::iterator dmd = xml_demands.begin(); dmd != xml_demands.end(); ++dmd)
  {
    DatabaseWriter::pushStatement(
      "delete from demand where name = $1;",
      (*dmd)->getName()
      );
    DatabaseWriter::pushStatement(
      "insert into demand "
        "(name, quantity, priority, description, status, item_id, location_id, "
        "customer_id, minshipment, maxlateness, category, due, lastmodified) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, now())",
      (*dmd)->getName(),
      to_string(static_cast<long double>((*dmd)->getQuantity())),
      to_string(static_cast<long long>((*dmd)->getPriority())),
      (*dmd)->getDescription(),
      string("quote"),
      (*dmd)->getItem()->getName(),
      (*dmd)->getLocation() ? (*dmd)->getLocation()->getName() : string(""),
      (*dmd)->getCustomer() ? (*dmd)->getCustomer()->getName() : string(""),
      to_string(static_cast<long double>((*dmd)->getMinShipment())),
      to_string(static_cast<long double>((*dmd)->getMaxLateness())),
      (*dmd)->getCategory(),
      static_cast<string>((*dmd)->getDue())
      );
  }

  // Collect the replanning results
  mg_printf(conn,
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: %d\r\n\r\n"
    "%s",
    static_cast<int>(response.str().size()),
    response.str().c_str()
    );
  return true;
}

} // End namespace
