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

void SerializerJSON::escape(const string& x)
{
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
        {
          // Control characters
          *m_fp << "\\u" << setw(4) << static_cast<int>(*p);
        }
        else
          *m_fp << *p;
    }
  }

}

}       // end namespace
