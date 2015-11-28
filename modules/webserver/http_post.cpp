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

  // TODO Assure single user access from this point onwards

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
    else if (!strcmp(request_info->uri, "/quote"))
       return quote_or_inquiry(conn, true);
    else if (!strcmp(request_info->uri, "/inquiry"))
       return quote_or_inquiry(conn, false);
    else
    {
      mg_printf(conn,
        "HTTP/1.1 404 Not found\r\n"
        "Content-Length: 24\r\n\r\n"
        "Cannot post to this URL"
        );
      return true;
    }
    mg_printf(conn,
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 20\r\n\r\n"
      "Successfully uploaded"
      );
    return true;
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
      "Content-Length: 17\r\n\r\n"
      "Unknown exception"
      );
  }
  return true;
}


#define LF 10
#define CR 13


MultiPartParser::MultiPartParser(char *boundary)
{
  multipart_boundary = boundary;
  boundary_length = strlen(boundary);
  lookbehind = boundary + boundary_length + 1; //XXX weird
  index = 0;
  state = s_start;
}


size_t MultiPartParser::execute(const char *buf, size_t len)
{
  size_t i = 0;
  size_t mark = 0;
  char c, cl;
  int is_last = 0;

  while(i < len) {
    c = buf[i];
    is_last = (i == (len - 1));
    switch (state) {
      case s_start:
        index = 0;
        state = s_start_boundary;

      /* fallthrough */
      case s_start_boundary:
        if (index == boundary_length) {
          if (c != CR)
            return i;
          index++;
          break;
        }
        else if (index == (boundary_length + 1))
        {
          if (c != LF)
            return i;
          index = 0;
          if (on_part_data_begin())
            return i;
          state = s_header_field_start;
          break;
        }
        if (c != multipart_boundary[index])
          return i;
        index++;
        break;

      case s_header_field_start:
        mark = i;
        state = s_header_field;

      /* fallthrough */
      case s_header_field:
        if (c == CR)
        {
          state = s_headers_almost_done;
          break;
        }

        if (c == ':')
        {
          if (on_header_field(buf + mark, i - mark))
            return i;
          state = s_header_value_start;
          break;
        }

        cl = tolower(c);
        if ((c != '-') && (cl < 'a' || cl > 'z'))
          return i;
        if (is_last)
        {
          if (on_header_field(buf + mark, (i - mark) + 1))
            return i;
        }
        break;

      case s_headers_almost_done:
        if (c != LF)
          return i;

        state = s_part_data_start;
        break;

      case s_header_value_start:
        if (c == ' ')
          break;

        mark = i;
        state = s_header_value;

      /* fallthrough */
      case s_header_value:
        if (c == CR)
        {
          if (on_header_value(buf + mark, i - mark))
            return i;
          state = s_header_value_almost_done;
          break;
        }
        if (is_last)
        {
          if (on_header_value(buf + mark, (i - mark) + 1))
            return i;
        }
        break;

      case s_header_value_almost_done:
        if (c != LF)
          return i;
        state = s_header_field_start;
        break;

      case s_part_data_start:
        if (on_headers_complete())
          return i;
        mark = i;
        state = s_part_data;

      /* fallthrough */
      case s_part_data:
        if (c == CR)
        {
          if (on_part_data(buf + mark, i - mark))
            return i;
          mark = i;
          state = s_part_data_almost_boundary;
          lookbehind[0] = CR;
          break;
        }
        if (is_last)
        {
          if (on_part_data(buf + mark, (i - mark) + 1))
            return i;
        }
        break;

      case s_part_data_almost_boundary:
        if (c == LF)
        {
          state = s_part_data_boundary;
          lookbehind[1] = LF;
          index = 0;
          break;
        }
        if (on_part_data(lookbehind, 1))
          return i;
        state = s_part_data;
        mark = i --;
        break;

      case s_part_data_boundary:
        if (multipart_boundary[index] != c)
        {
          if (on_part_data(lookbehind, 2 + index))
            return i;
          state = s_part_data;
          mark = i --;
          break;
        }
        lookbehind[2 + index] = c;
        if ((++ index) == boundary_length) {
          if (on_part_data_end())
            return i;
          state = s_part_data_almost_end;
        }
        break;

      case s_part_data_almost_end:
        if (c == '-')
        {
          state = s_part_data_final_hyphen;
          break;
        }
        if (c == CR)
        {
          state = s_part_data_end;
          break;
        }
        return i;

      case s_part_data_final_hyphen:
        if (c == '-')
        {
          if (on_body_end())
            return i;
          state = s_end;
          break;
        }
        return i;

      case s_part_data_end:
        if (c == LF)
        {
          state = s_header_field_start;
          if (on_part_data_begin())
            return i;
          break;
        }
        return i;

      case s_end:
        break;

      default:
        return 0;
    }
    ++ i;
  }

  return len;
}

} // End namespace
