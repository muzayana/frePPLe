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

namespace module_webserver
{

bool WebServer::handleGet(CivetServer *server, struct mg_connection *conn)
{
  struct mg_request_info *request_info = mg_get_request_info(conn);

  // Write the complete model
  if (!strcmp(request_info->uri, "/"))
  {
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/xml\r\n\r\n");
    mg_printf(conn, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    mg_printf(conn, "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");
    XMLOutputString o;
    MetaCategory::persistAll(&o);
    mg_printf(conn, o.getData().c_str());
    mg_printf(conn, "</plan>\n");
    return true;
  }

  // Stop the web server
  if (!strcmp(request_info->uri, "/stop/"))
  {
    // Update the flag that signals the server to shut down
    *exitNow = true;
    return false;
  }

  // Check if the first section of the URL is a category name
  const MetaCategory* cat = MetaCategory::findCategoryByTag(request_info->uri + 1);
  if (!cat)
    // We don't handle this URL, and it will be interpreted as a static file.
    return false;

  if (true)
  {
    // Return all objects of a single category
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/xml\r\n\r\n");
    mg_printf(conn, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    mg_printf(conn, "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");
    XMLOutputString o;
    cat->persist(&o);
    mg_printf(conn, o.getData().c_str());
    mg_printf(conn, "</plan>\n");
    return true;
  }
  else
  {
    // Return a single object TODO
  }
}

} // End namespace