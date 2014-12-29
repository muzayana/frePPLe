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
#include <iomanip>

namespace module_webserver
{


PyObject* saveJSONfile(PyObject* self, PyObject* args)
{
  // Pick up arguments
  char *filename;
  char *content = NULL;
  int ok = PyArg_ParseTuple(args, "s|s:save", &filename, &content);
  if (!ok) return NULL;

  // Execute and catch exceptions
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    SerializerJSONFile o(filename);
    if (content)
    {
      if (!strcmp(content,"STANDARD"))
        o.setContentType(Serializer::STANDARD);
      else if (!strcmp(content,"PLAN"))
        o.setContentType(Serializer::PLAN);
      else if (!strcmp(content,"PLANDETAIL"))
        o.setContentType(Serializer::PLANDETAIL);
      else
        throw DataException("Invalid content type '" + string(content) + "'");
    }
    o.setMode(true);
    Plan::instance().writeElement(&o, Tags::tag_plan);
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}


void SerializerJSON::escape(const string& x)
{
  *m_fp << "\"";
  for (const char* p = x.c_str(); *p; ++p)
  {
    switch (*p)
    {
      case '"': *m_fp << "\\\""; break;
      case '/': *m_fp << "\\/"; break;
      case '\\': *m_fp << "\\\\"; break;
      case '\b': *m_fp << "\\b"; break;
      case '\t': *m_fp << "\\t"; break;
      case '\n': *m_fp << "\\n"; break;
      case '\f': *m_fp << "\\f"; break;
      case '\r': *m_fp << "\\r"; break;
      default:
        if (*p < ' ')
          // Control characters
          *m_fp << "\\u" << setw(4) << static_cast<int>(*p);
        else
          *m_fp << *p;
    }
  }
  *m_fp << "\"";
}

}       // end namespace
