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

/* Uncomment the next line to create a lot of debugging messages during
 * the parsing of the data. */
#define PARSE_DEBUG

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
    // TODO parse directly by passing the rapidjson parser a filestream
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


void JSONInput::parse(Object* pRoot, char* buffer)
{
  if (!objectindex)
    throw DataException("JSON parser not empty");
  if (!pRoot)
    throw DataException("Can't parse JSON data into NULL root object");

  // Initialize the parser to read data into the object pRoot.
  objectindex = 0;
  dataindex = -1;
  objects[0].start = 0;
  objects[0].object = pRoot;
  objects[0].cls = &pRoot->getType();
  objects[0].hash = pRoot->getType().typetag->getHash();

  // Call rapidjson
  // TODO Extra setting for in-site parsing
  rapidjson::Reader reader;
  rapidjson::StringStream buf(buffer);
  reader.Parse(buf, *this);
}


bool JSONInput::Null()
{
  if (dataindex >= 0)
    data[dataindex].value.setNull();
  return true;
}


bool JSONInput::Bool(bool b)
{
  if (dataindex >= 0)
    data[dataindex].value.setBool(b);
  return true;
}


bool JSONInput::Int(int i)
{
  if (dataindex >= 0)
    data[dataindex].value.setInt(i);
  return true;
}


bool JSONInput::Uint(unsigned u)
{
  if (dataindex >= 0)
    data[dataindex].value.setLong(u);
  return true;
}


bool JSONInput::Int64(int64_t i)
{
  if (dataindex >= 0)
    data[dataindex].value.setLong(i);
  return true;
}


bool JSONInput::Uint64(uint64_t u)
{
  if (dataindex >= 0)
    data[dataindex].value.setUnsignedLong(u);
  return true;
}


bool JSONInput::Double(double d)
{
  if (dataindex >= 0)
    data[dataindex].value.setDouble(d);
  return true;
}


bool JSONInput::String(const char* str, rapidjson::SizeType length, bool copy)
{
  if (dataindex >= 0)
    data[dataindex].value.setString(str);
  return true;
}


bool JSONInput::StartObject()
{
  if (++objectindex >= maxobjects)
    // You're joking?
    throw DataException("JSON-document nested excessively deep");

  // Debugging message
  #ifdef PARSE_DEBUG
  logger << "Starting object #" << objectindex << endl;
  #endif
  return true;
}


bool JSONInput::Key(const char* str, rapidjson::SizeType length, bool copy)
{
  // Look up the field
  data[++dataindex].value.setNull();
  data[dataindex].hash = Keyword::hash(str);
  data[dataindex].name = str;

  /* XXX TODO
  data[dataindex].field = objects[objectindex].cls->findField(data[dataindex].hash);
  if (!data[dataindex].field && objects[objectindex].cls->category)
    data[dataindex].field = objects[objectindex].cls->category->findField(data[dataindex].hash);
  if (!data[dataindex].field)
    throw DataException("Field '" + string(str) + "' not defined");
  */
  // Debugging message
  #ifdef PARSE_DEBUG
  logger << "Reading field #" << dataindex << " '" << str
    << "' for object #" << objectindex << " ("
    << ((objectindex >= 0 && objects[objectindex].cls) ? objects[objectindex].cls->type : "none")
    << ")" << endl;
  #endif

  return true;
}


bool JSONInput::EndObject(rapidjson::SizeType memberCount)
{
  // Debugging
  #ifdef PARSE_DEBUG
  cout << "Ending Object #" << objectindex << " (" << memberCount << ")" << endl;
  for (int i = 0; i <= static_cast<int>(memberCount); ++i)
  {
    if (dataindex - i < 0)
      break;
    logger << "   " << (dataindex - i)
      << " " << data[dataindex - i].name
      << " (" << data[dataindex - i].value.getDataType()
      << "): " << data[dataindex - i].value.getString()  << endl;
  }
  #endif

  // Update stack
  dataindex -= memberCount;
  --objectindex;
  return true;
}


bool JSONInput::StartArray()
{
  #ifdef PARSE_DEBUG
  logger << "Starting array" << endl;
  #endif
  return true;
}


bool JSONInput::EndArray(rapidjson::SizeType elementCount)
{
  #ifdef PARSE_DEBUG
  logger << "Ending array" << endl;
  #endif
  return true;
}


long JSONData::getLong() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return 0;
    case JSON_INT:
      return data_int;
    case JSON_LONG:
      return data_long;
    case JSON_UNSIGNEDLONG:
      return data_unsignedlong;
    case JSON_DOUBLE:
      return data_double;
    case JSON_STRING:
      return atol(data_string.c_str());
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


unsigned long JSONData::getUnsignedLong() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return 0;
    case JSON_INT:
      return data_int;
    case JSON_LONG:
      return data_long;
    case JSON_UNSIGNEDLONG:
      return data_unsignedlong;
    case JSON_DOUBLE:
      return data_double;
    case JSON_STRING:
      return atol(data_string.c_str());
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


Duration JSONData::getDuration() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return Duration(0L);
    case JSON_INT:
      return data_int;
    case JSON_LONG:
      return data_long;
    case JSON_UNSIGNEDLONG:
      return data_unsignedlong;
    case JSON_DOUBLE:
      return data_double;
    case JSON_STRING:
      return atol(data_string.c_str());
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


int JSONData::getInt() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return 0;
    case JSON_INT:
      return data_int;
    case JSON_LONG:
      return data_long;
    case JSON_UNSIGNEDLONG:
      return data_unsignedlong;
    case JSON_DOUBLE:
      return data_double;
    case JSON_STRING:
      return atol(data_string.c_str());
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


double JSONData::getDouble() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return 0;
    case JSON_INT:
      return data_int;
    case JSON_LONG:
      return data_long;
    case JSON_UNSIGNEDLONG:
      return data_unsignedlong;
    case JSON_DOUBLE:
      return data_double;
    case JSON_STRING:
      return atol(data_string.c_str());
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


Date JSONData::getDate() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return Date();
    case JSON_INT:
      return Date(data_int);
    case JSON_LONG:
      return Date(data_long);
    case JSON_UNSIGNEDLONG:
      return Date(data_unsignedlong);
    case JSON_DOUBLE:
      return Date(data_double);
    case JSON_STRING:
      return Date(data_string.c_str());
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


const string& JSONData::getString() const
{
  switch (data_type)
  {
    case JSON_NULL:
      const_cast<JSONData*>(this)->data_string = "NULL";
      return data_string;
    case JSON_INT:
      {
      ostringstream convert;
      convert << data_int;
      const_cast<JSONData*>(this)->data_string = convert.str();
      return data_string;
      }
    case JSON_LONG:
      {
      ostringstream convert;
      convert << data_long;
      const_cast<JSONData*>(this)->data_string = convert.str();
      return data_string;
      }
    case JSON_UNSIGNEDLONG:
      {
      ostringstream convert;
      convert << data_unsignedlong;
      const_cast<JSONData*>(this)->data_string = convert.str();
      return data_string;
      }
    case JSON_DOUBLE:
      {
      ostringstream convert;
      convert << data_double;
      const_cast<JSONData*>(this)->data_string = convert.str();
      return data_string;
      }
    case JSON_STRING:
      return data_string;
    case JSON_OBJECT:
      throw DataException("Invalid JSON data");
  }
  throw DataException("Unknown JSON type");
}


bool JSONData::getBool() const
{
  switch (data_type)
  {
    case JSON_NULL:
      return false;
    case JSON_INT:
      return data_int != 0;
    case JSON_LONG:
      return data_long != 0;
    case JSON_UNSIGNEDLONG:
      return data_unsignedlong != 0;
    case JSON_DOUBLE:
      return data_double != 0;
    case JSON_STRING:
      return !data_string.empty();
    case JSON_OBJECT:
      return data_object != NULL;
  }
  throw DataException("Unknown JSON type");
}


Object* JSONData::getObject() const
{
  switch (data_type)
  {
    case JSON_NULL:
    case JSON_INT:
    case JSON_LONG:
    case JSON_UNSIGNEDLONG:
    case JSON_DOUBLE:
    case JSON_STRING:
      throw DataException("Invalid JSON data");
    case JSON_OBJECT:
      return data_object;
  }
  throw DataException("Unknown JSON type");
}


}       // end namespace
