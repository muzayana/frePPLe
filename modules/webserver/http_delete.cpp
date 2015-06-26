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

bool WebServer::handleDelete(CivetServer *server, struct mg_connection *conn)
{
  struct mg_request_info *request_info = mg_get_request_info(conn);

  // Check if the first section of the URL is a category name
  const char* slash = strchr(request_info->uri + 1, '/');
  string categoryname;
  if (slash)
    categoryname = string(request_info->uri + 1, slash - request_info->uri - 1);
  else
    categoryname = request_info->uri + 1;
  const MetaCategory* cat = MetaCategory::findCategoryByTag(categoryname.c_str());
  if (!cat)
  {
    mg_printf(conn,
      "HTTP/1.1 404 Not found\r\n"
      "Content-Length: 112\r\n\r\n"
      "<html><head><title>Category not found</title></head><body>Sorry, the entity category doesn't exist.</body></html>"
      );
    return true;
  }

  // Return a single object
  string entitykey = slash + 1;
  Object* entity = cat->find(entitykey);
  if (!entity)
  {
    mg_printf(conn,
      "HTTP/1.1 404 Not found\r\n"
      "Content-Length: 112\r\n\r\n"
      "<html><head><title>Entity not found</title></head><body>Sorry, the requested object doesn't exist.</body></html>"
      );
    return true;
  }

  // Delete the object 
  try
  {
    delete entity;
    mg_printf(conn, 
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 20\r\n\r\n"
      "Successfully deleted"
      );
  }
  catch (...)
  {
    mg_printf(conn,
      "HTTP/1.1 500 Server error\r\n"
      "Content-Length: 105\r\n\r\n"
      "<html><head><title>Server error</title></head><body>Exception occurred when deleting object</body></html>"
      );
  }
  return true;
}

} // End namespace