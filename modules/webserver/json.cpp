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
#include <iomanip>

// With VC++ we use the Win32 functions to browse a directory
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
// With Unix-like systems we use a check suggested by the autoconf tools
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#endif


namespace module_webserver
{


PyObject* saveJSONfile(PyObject* self, PyObject* args)
{
  // Pick up arguments
  char *filename;
  char *content = NULL;
  int formatted = 0;
  int ok = PyArg_ParseTuple(args, "s|sp:saveJSONfile", &filename, &content, &formatted);
  if (!ok) return NULL;

  // Execute and catch exceptions
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    JSONSerializerFile o(filename);
    if (content)
    {
      if (!strcmp(content, "BASE"))
        o.setContentType(BASE);
      else if (!strcmp(content, "PLAN"))
        o.setContentType(PLAN);
      else if (!strcmp(content, "DETAIL"))
        o.setContentType(DETAIL);
      else
        throw DataException("Invalid content type '" + string(content) + "'");
    }
    if (formatted)
      o.setFormatted(true);
    o.setMode(true);
    o.pushCurrentObject(&Plan::instance());
    o.incParents();
    Plan::instance().writeElement(&o, Tags::plan);
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


void JSONSerializer::escape(const string& x)
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


void JSONInput::visit(Object *pRoot, JSONInput::JsonValue o)
{
  switch (o.getTag())
  {
    case JSON_NUMBER:
        logger << "NUMBER: " << o.toNumber() << endl;
        break;
    case JSON_STRING:
        logger << "STRING: " << o.toString() << endl;
        break;
    case JSON_ARRAY:
        logger << "ARRAY starting" << endl;
        for (JsonIterator i = begin(o); i != end(); ++i)
          visit(pRoot, i->value);
        logger << "ARRAY ending" << endl;
        break;
    case JSON_OBJECT:
        for (JsonIterator i = begin(o); i != end(); ++i)
        {
          logger << "OBJECT starting "  << i->key << endl;
          visit(pRoot, i->value);
        logger << "OBJECT ending" << endl;
        }
        break;
    case JSON_TRUE:
        logger << "TRUE" << endl;
        break;
    case JSON_FALSE:
        logger << "FALSE" << endl;
        break;
    case JSON_NULL:
        logger << "NULL" << endl;
        break;
    }
}


void JSONInput::parse(Object *pRoot, char* buf)
{
  JSONInput::JsonValue value;
  JSONInput::JsonAllocator allocator;
  JSONInput::parse(buf, &value, allocator);
  visit(pRoot, value);
}


void JSONInputFile::parse(Object *pRoot)
{
  // Check if string has been set
  if (filename.empty())
    throw DataException("Missing input file or directory");

  // Check if the parameter is the name of a directory
  struct stat stat_p;
  if (stat(filename.c_str(), &stat_p))
    // Can't verify the status
    throw RuntimeException("Couldn't open input file '" + filename + "'");
  else if (stat_p.st_mode & S_IFDIR)
  {
    // Data is a directory: loop through all *.xml files now. No recursion in
    // subdirectories is done.
    // The code is unfortunately different for Windows & Linux. Sigh...
#ifdef _MSC_VER
    string f = filename + "\\*.json";
    WIN32_FIND_DATA dir_entry_p;
    HANDLE h = FindFirstFile(f.c_str(), &dir_entry_p);
    if (h == INVALID_HANDLE_VALUE)
      throw RuntimeException("Couldn't open input file '" + f + "'");
    do
    {
      f = filename + '/' + dir_entry_p.cFileName;
      JSONInputFile(f.c_str()).parse(pRoot);
    }
    while (FindNextFile(h, &dir_entry_p));
    FindClose(h);
#elif HAVE_DIRENT_H
    struct dirent *dir_entry_p;
    DIR *dir_p = opendir(filename.c_str());
    while (NULL != (dir_entry_p = readdir(dir_p)))
    {
      int n = NAMLEN(dir_entry_p);
      if (n > 4 && !strcmp(".xml", dir_entry_p->d_name + n - 4))
      {
        string f = filename + '/' + dir_entry_p->d_name;
        JSONInputFile(f.c_str()).parse(pRoot);
      }
    }
    closedir(dir_p);
#else
    throw RuntimeException("Can't process a directory on your platform");
#endif
  }
  else
  {
    // Normal file
    // Read the complete file in a memory buffer
    ifstream t;
    t.open(filename.c_str());
    t.seekg(0, ios::end);
    ifstream::pos_type length = t.tellg();
    if (length > 100000000)
      throw DataException("Maximum JSON file size is 100MB");
    t.seekg(0, std::ios::beg);
    char *buffer = new char[length];
    t.read(buffer, length);
    t.close();

    // Parse the data
    JSONInput::parse(pRoot, buffer);
  }
}


#define JSON_ZONE_SIZE 4096
#define JSON_STACK_SIZE 32


void *JSONInput::JsonAllocator::allocate(size_t size)
{
  size = (size + 7) & ~7;

  if (head && head->used + size <= JSON_ZONE_SIZE)
  {
    char *p = (char *)head + head->used;
    head->used += size;
    return p;
  }

  size_t allocSize = sizeof(Zone) + size;
  Zone *zone = (Zone *)malloc(allocSize <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : allocSize);
  zone->used = allocSize;
  if (allocSize <= JSON_ZONE_SIZE || head == NULL)
  {
    zone->next = head;
    head = zone;
  }
  else
  {
    zone->next = head->next;
    head->next = zone;
  }
  return (char *)zone + sizeof(Zone);
}


void JSONInput::JsonAllocator::deallocate()
{
  while (head)
  {
    Zone *next = head->next;
    free(head);
    head = next;
  }
}


double JSONInput::string2double(char *s, char **endptr)
{
  // Skip sign
  char ch = *s;
  if (ch == '-') ++s;

  // Before the decimal
  double result = 0;
  while (isdigit(*s))
    result = result * 10 + (*s++ - '0');

  // Decimal and after
  if (*s == '.')
  {
    ++s;
    double fraction = 1;
    while (isdigit(*s))
    {
      fraction *= 0.1;
      result += (*s++ - '0') * fraction;
    }
  }

  // Exponent
  if (*s == 'e' || *s == 'E')
  {
    ++s;

    double base = 10;
    if (*s == '+')
      ++s;
    else if (*s == '-')
    {
      ++s;
      base = 0.1;
    }

    int exponent = 0;
    while (isdigit(*s))
      exponent = (exponent * 10) + (*s++ - '0');

    double power = 1;
    for (; exponent; exponent >>= 1, base *= base)
      if (exponent & 1)
        power *= base;

    result *= power;
  }

  // Final result
  *endptr = s;
  return ch == '-' ? -result : result;
}


void JSONInput::parse(char *s, JSONInput::JsonValue *value, JSONInput::JsonAllocator &allocator)
{
  char *firstChar = s;
  char *endptr = s;
  JsonNode *tails[JSON_STACK_SIZE];
  JsonTag tags[JSON_STACK_SIZE];
  char *keys[JSON_STACK_SIZE];
  JsonValue o;
  int pos = -1;
  bool separator = true;

  while (*s)
  {
    // Skip leading whitespace
    while (isspace(*s)) ++s;
    endptr = s++;

    switch (*endptr)
    {
      case '-':
        if (!isdigit(*s) && *s != '.')
          throw DataException("Invalid JSON data: bad number at position " + (s - firstChar));
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        o = JsonValue(string2double(endptr, &s));
        if (!isdelim(*s))
          throw DataException("Invalid JSON data: bad number at position " + (s - firstChar));
        break;
      case '"':
        o = JsonValue(JSON_STRING, s);
        for (char *it = s; *s; ++it, ++s)
        {
          int c = *it = *s;
          if (c == '\\')
          {
            c = *++s;
            switch (c)
            {
              case '\\':
              case '"':
              case '/':
                *it = c;
                break;
              case 'b':
                *it = '\b';
                break;
              case 'f':
                *it = '\f';
                break;
              case 'n':
                *it = '\n';
                break;
              case 'r':
                *it = '\r';
                break;
              case 't':
                *it = '\t';
                break;
              case 'u':
                c = 0;
                for (int i = 0; i < 4; ++i)
                {
                  if (isxdigit(*++s))
                    c = c * 16 + char2int(*s);
                  else
                    throw DataException("Invalid JSON data: bad string at position " + (s - firstChar));
                }
                if (c < 0x80)
                  *it = c;
                else if (c < 0x800)
                {
                  *it++ = 0xC0 | (c >> 6);
                  *it = 0x80 | (c & 0x3F);
                }
                else
                {
                  *it++ = 0xE0 | (c >> 12);
                  *it++ = 0x80 | ((c >> 6) & 0x3F);
                  *it = 0x80 | (c & 0x3F);
                }
                break;
              default:
                throw DataException("Invalid JSON data: bad string at position " + (s - firstChar));
              }
            }
          else if ((unsigned int)c < ' ' || c == '\x7F')
              throw DataException("Invalid JSON data: bad string at position " + (s - firstChar));
          else if (c == '"')
          {
            *it = 0;
            ++s;
            break;
          }
        }
        if (!isdelim(*s))
          throw DataException("Invalid JSON data: bad string at position " + (s - firstChar));
        break;
      case 't':
        if (!(s[0] == 'r' && s[1] == 'u' && s[2] == 'e' && isdelim(s[3])))
          throw DataException("Invalid JSON data: bad identifier at position " + (s - firstChar));
        o = JsonValue(JSON_TRUE);
        s += 3;
        break;
      case 'f':
        if (!(s[0] == 'a' && s[1] == 'l' && s[2] == 's' && s[3] == 'e' && isdelim(s[4])))
          throw DataException("Invalid JSON data: bad identifier at position " + (s - firstChar));
        o = JsonValue(JSON_FALSE);
        s += 4;
        break;
      case 'n':
        if (!(s[0] == 'u' && s[1] == 'l' && s[2] == 'l' && isdelim(s[3])))
          throw DataException("Invalid JSON data: bad identifier at position " + (s - firstChar));
        o = JsonValue(JSON_NULL);
        s += 3;
        break;
      case ']':
        if (pos == -1)
          throw DataException("Invalid JSON data: stack underflow at position " + (s - firstChar));
        if (tags[pos] != JSON_ARRAY)
          throw DataException("Invalid JSON data: bracket mismatch at position " + (s - firstChar));
        o = listToValue(JSON_ARRAY, tails[pos--]);
        break;
      case '}':
        if (pos == -1)
          throw DataException("Invalid JSON data: stack underflow at position " + (s - firstChar));
        if (tags[pos] != JSON_OBJECT)
          throw DataException("Invalid JSON data: bracket mismatch at position " + (s - firstChar));
        if (keys[pos] != NULL)
          throw DataException("Invalid JSON data: unexpected character at position " + (s - firstChar));
        o = listToValue(JSON_OBJECT, tails[pos--]);
        break;
      case '[':
        if (++pos == JSON_STACK_SIZE)
          throw DataException("Invalid JSON data: stack overflow at position " + (s - firstChar));
        tails[pos] = NULL;
        tags[pos] = JSON_ARRAY;
        keys[pos] = NULL;
        separator = true;
        continue;
      case '{':
        if (++pos == JSON_STACK_SIZE)
          throw DataException("Invalid JSON data: stack overflow at position " + (s - firstChar));
        tails[pos] = NULL;
        tags[pos] = JSON_OBJECT;
        keys[pos] = NULL;
        separator = true;
        continue;
      case ':':
        if (separator || keys[pos] == NULL)
          throw DataException("Invalid JSON data: unexpected character at position " + (s - firstChar));
        separator = true;
        continue;
      case ',':
        if (separator || keys[pos] != NULL)
          throw DataException("Invalid JSON data: unexpected character at position " + (s - firstChar));
        separator = true;
        continue;
      case '\0':
        continue;
      default:
        throw DataException("Invalid JSON data: unexpected character at position " + (s - firstChar));
      }

      separator = false;
      if (pos == -1)
      {
        endptr = s;
        *value = o;
        return;
      }

      /** TODO The parser builds a DOM-like structure in memory here. We'ld like to send SAX-like events instead, or even better DOM-with-flush. */
      if (tags[pos] == JSON_OBJECT)
      {
        if (!keys[pos])
        {
          if (o.getTag() != JSON_STRING)
            throw DataException("Invalid JSON data: unquoted key at position " + (endptr - firstChar));
          keys[pos] = o.toString();
          continue;
        }
        tails[pos] = insertAfter(tails[pos], (JsonNode *)allocator.allocate(sizeof(JsonNode)));
        tails[pos]->key = keys[pos];
        keys[pos] = NULL;
      }
      else
        tails[pos] = insertAfter(tails[pos], (JsonNode *)allocator.allocate(sizeof(JsonNode) - sizeof(char *)));
      tails[pos]->value = o;
    }
    throw LogicException("Unreachable code reached in JSON parser");
}


const DataValue* JSONAttributeList::get(const Keyword& kw) const // TODO XXX
{
  /*
  for (JSONInput::JsonIterator i(node); i != JSONInput::JsonIterator(NULL); ++i)
  {
    if (Keyword::hash(i->key) == kw.getHash())
      return i->value;
  }
  return NULL;
  */
  return NULL;
}


PyObject* readJSONfile(PyObject* self, PyObject* args)
{
  // Pick up arguments
  char *filename = NULL;
  int ok = PyArg_ParseTuple(args, "s:readJSONfile", &filename);
  if (!ok) return NULL;

  // Execute and catch exceptions
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    if (!filename) throw DataException("Missing filename");
    JSONInputFile(filename).parse(&Plan::instance());
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


PyObject* readJSONdata(PyObject *self, PyObject *args)
{
  // Pick up arguments
  char *data;
  int ok = PyArg_ParseTuple(args, "s:readJSONdata", &data);
  if (!ok) return NULL;

  // Free Python interpreter for other threads
  Py_BEGIN_ALLOW_THREADS

  // Execute and catch exceptions
  try
  {
    if (!data) throw DataException("No input data");
    JSONInputString(data).parse(&Plan::instance());
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");  // Safer than using Py_None, which is not portable across compilers
}


}       // end namespace
