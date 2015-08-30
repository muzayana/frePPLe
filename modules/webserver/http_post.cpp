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

bool WebServer::handlePost(CivetServer *server, struct mg_connection *conn)
{
  struct mg_request_info *request_info = mg_get_request_info(conn);

  try
  {
    if (!strcmp(request_info->uri, "/json"))
    {
    }
    else if (!strcmp(request_info->uri, "/xml"))
    {
      /*  TODO
        char post_data[1024], input1[sizeof(post_data)], input2[sizeof(post_data)];
    int post_data_len;

        // User has submitted a form, show submitted data and a variable value
        post_data_len = mg_read(conn, post_data, sizeof(post_data));

        // Parse form data. input1 and input2 are guaranteed to be NUL-terminated
        mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1));
        mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2));
        mg_upload
        */
    }
    else
    {
      mg_printf(conn, 
        "HTTP/1.1 404 Not found\r\n"
        "Content-Length: 42\r\n\r\n"
        "Only posting to /xml and /json is allowed."
        );
      return true;
    }
    mg_printf(conn, 
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 20\r\n\r\n"
      "Successfully uploaded"
      );
  }
  catch(exception& e)
  {
    mg_printf(conn, 
      "HTTP/1.1 500 Server error\r\n"
      "Content-Length: %d\r\n\r\n"
      "%s",
      static_cast<int>(strlen(e.what())), e.what()
      );
  }
  catch(...)
  {
    mg_printf(conn, 
      "HTTP/1.1 500 Server error\r\n"
      "Content-Length: 16\r\n\r\n"
      "Unknow exception"
      );
  }
  return true;
}

} // End namespace
