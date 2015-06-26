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

#include "json.h"
#include "webserver.h"

namespace module_webserver
{

bool WebServer::handleGet(CivetServer *server, struct mg_connection *conn)
{
  struct mg_request_info *request_info = mg_get_request_info(conn);

  // Write the complete model
  if (!strcmp(request_info->uri, "/"))
  {
    string format = "xml";
    CivetServer::getParam(conn, "format", format);
    if (format == "json")
    {
      // JSON format
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
      SerializerJSONString o;
      o.writeString("{");
      MetaCategory::persistAll(&o);
      o.writeString("}");
      mg_printf(conn, "%s", o.getData().c_str());
    }
    else
    {
      // XML format (default)
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/xml\r\n\r\n");
      mg_printf(conn, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
      mg_printf(conn, "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");
      SerializerXMLString o;
      MetaCategory::persistAll(&o);
      mg_printf(conn, "%s", o.getData().c_str());
      mg_printf(conn, "</plan>\n");
    }
    return true;
  }

  // Return the index page
  if (!strcmp(request_info->uri, "/index.html"))
  {
    static string::size_type indexlength = 0;
    static string index;
    if (!indexlength)
    {
      buildIndex(index);
      indexlength = index.size();
    }
    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s",
      indexlength, index.c_str()
      );
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
  const char* slash = strchr(request_info->uri + 1, '/');
  string categoryname;
  if (slash)
    categoryname = string(request_info->uri + 1, slash - request_info->uri - 1);
  else
    categoryname = request_info->uri + 1;
  const MetaCategory* cat = MetaCategory::findCategoryByTag(categoryname.c_str());
  if (!cat)
    // We don't handle this URL, and it will be interpreted as a static file.
    return false;

  // Get requested output format
  string format = "xml";
  CivetServer::getParam(conn, "format", format);
  if (!slash || strncmp(request_info->uri + 1, cat->type.c_str(), slash - request_info->uri - 1))
  {
    // Return all objects of a single category
    if (format == "json")
    {
      // JSON format
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
      SerializerJSONString o;
      o.writeString("{");
      cat->persist(&o);
      o.writeString("}");
      mg_printf(conn, "%s", o.getData().c_str());
      return true;
    }
    else
    {
      // XML format (default)
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/xml\r\n\r\n");
      mg_printf(conn, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
      mg_printf(conn, "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");
      SerializerXMLString o;
      cat->persist(&o);
      mg_printf(conn, "%s", o.getData().c_str());
      mg_printf(conn, "</plan>\n");
      return true;
    }
  }
  else
  {
    // Return a single object
    string entitykey = slash + 1;
    const Object* entity = cat->find(entitykey);
    if (!entity)
    {
      mg_printf(conn,
        "HTTP/1.1 404 Not found\r\n"
        "Content-Length: 112\r\n\r\n"
        "<html><head><title>Entity not found</title></head><body>Sorry, the requested object doesn't exist.</body></html>"
        );
      return true;
    }

    // Return a single object objects in JSON format
    if (format == "json")
    {
      // JSON format
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
      SerializerJSONString o;
      o.writeString("{");
      o.BeginList(*(cat->grouptag));
      entity->writeElement(&o, *(cat->typetag), FULL);
      o.EndList(*(cat->grouptag));
      o.writeString("}");
      mg_printf(conn, "%s", o.getData().c_str());
      return true;
    }
    else
    {
      // Return a single object in XML format
      mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/xml\r\n\r\n");
      mg_printf(conn, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
      mg_printf(conn, "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");
      SerializerXMLString o;
      o.BeginList(*(cat->grouptag));
      entity->writeElement(&o, *(cat->typetag), FULL);
      o.EndList(*(cat->grouptag));
      mg_printf(conn, "%s", o.getData().c_str());
      mg_printf(conn, "</plan>\n");
      return true;
    }
  }
}


void WebServer::buildIndex(string& index)
{
  // Header
  ostringstream strm;
  strm << "<!DOCTYPE html>" << endl
    << "<html lang=\"en-us\"><head>" << endl
    << "<title>frePPLe" << PACKAGE_VERSION << " web service API</title>" << endl
    << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />" << endl
    << "<meta name=\"robots\" content=\"NONE,NOARCHIVE\" />" << endl;

  // Style
  strm << "<style>" << endl
    << "</style>" << endl;

  // Body
  strm << "</head><body>" << endl
    << "<h1>frePPLe" << PACKAGE_VERSION << " web service API</h1>" << endl;
  strm << "<h2>GET data</h2>" << endl;
  strm << "<h2>POST data</h2>" << endl;

  // Posting of XML data
  strm << "<ul><li>Upload XML data<br/><form action=\"/\" method=\"post\">" << endl
    << "Data: <textarea id=\"xmldata\" name=\"xmldata\" rows=\"20\" cols=\"100\">" << endl
    << "&lt;?xml version=\"1.0\" encoding=\"UTF-8\" ?&gt;" << endl
    << "&lt;plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"&gt;" << endl
    << "&lt;/plan&gt;" << endl
    << "</textarea>" << endl
    << "<br/><input type=\"submit\"/></form></li>" << endl;
  strm << "<li>Upload XML file</li>" << endl;
  strm << "<li>Upload JSON data</li>" << endl;
  strm << "<li>Upload JSON data file</li></ul>" << endl;
  strm << "<h2>DELETE data</h2>" << endl;
  strm << "<h2>Actions</h2>" << endl;
  strm << "</body></html>" << endl;

  // Get the final result
  index = strm.str();
}

} // End namespace