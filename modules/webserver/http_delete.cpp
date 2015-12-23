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
      "HTTP/1.1 404 Not Found\r\n\r\n"
      "<html><head><title>Category not found</title></head><body>Sorry, the entity category doesn't exist.</body></html>"
      );
    return true;
  }

  // Return a single object
  string entitykey = slash + 1;
  vector<XMLInput::fld> f(1);
  f[0].name = "name";
  f[0].hash = Tags::name.getHash();
  f[0].value = entitykey;
  f[0].field = NULL;
  XMLDataValueDict dict(f, 0, 0);
  Object* entity = cat->find(dict);
  if (!entity)
  {
    mg_printf(conn,
      "HTTP/1.1 404 Not Found\r\n\r\n"
      "<html><head><title>Entity not found</title></head><body>Sorry, the requested object doesn't exist.</body></html>"
      );
    return true;
  }

  // Delete the object
  try
  {
    string persist;
    CivetServer::getParam(conn, "persist", persist);
    if (persist == "1")
    {
      // Erase from the database.  TODO make this method generic?
      // The statements below only work cool if the database cascades the
      // delete to all child records. This is currently NOT the case (Postgres
      // supports it, but django doesn't support it when creating the schema).
      if (categoryname == "demand")
        DatabaseWriter::pushStatement(
          "delete from demand where name = $1;",
          static_cast<Demand*>(entity)->getName()
          );
      else if (categoryname == "item")
        DatabaseWriter::pushStatement(
          "delete from item where name = $1;",
          static_cast<Item*>(entity)->getName()
          );
      else if (categoryname == "customer")
        DatabaseWriter::pushStatement(
          "delete from customer where name = $1;",
          static_cast<Customer*>(entity)->getName()
          );
      else if (categoryname == "supplier")
        DatabaseWriter::pushStatement(
          "delete from supplier where name = $1;",
          static_cast<Supplier*>(entity)->getName()
          );
      else if (categoryname == "resource")
        DatabaseWriter::pushStatement(
          "delete from resource where name = $1;",
          static_cast<Resource*>(entity)->getName()
          );
      else if (categoryname == "buffer")
        DatabaseWriter::pushStatement(
          "delete from buffer where name = $1;",
          static_cast<Buffer*>(entity)->getName()
          );
      else if (categoryname == "skill")
        DatabaseWriter::pushStatement(
          "delete from skill where name = $1;",
          static_cast<Skill*>(entity)->getName()
          );
      else if (categoryname == "location")
        DatabaseWriter::pushStatement(
          "delete from location where name = $1;",
          static_cast<Location*>(entity)->getName()
          );
      else if (categoryname == "calendar")
        DatabaseWriter::pushStatement(
          "delete from calendar where name = $1;",
          static_cast<Calendar*>(entity)->getName()
          );
      else if (categoryname == "operation")
        DatabaseWriter::pushStatement(
          "delete from operation where name = $1;",
          static_cast<Operation*>(entity)->getName()
          );
      else
        throw DataException("DELETE method not implemented");
    }

    delete entity;

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\n\r\n"
      "Successfully deleted"
      );
  }
  catch (...)
  {
    mg_printf(conn,
      "HTTP/1.1 500 Internal Server Error\r\n\r\n"
      "<html><head><title>Server error</title></head><body>Exception occurred when deleting object</body></html>"
      );
  }
  return true;
}

} // End namespace