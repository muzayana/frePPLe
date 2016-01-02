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


bool JSONInput::Null()
{
  cout << "Null()" << endl; return true;
}


bool JSONInput::Bool(bool b)
{
  cout << "Bool(" << boolalpha << b << ")" << endl; return true;
}


bool JSONInput::Int(int i)
{
  cout << "Int(" << i << ")" << endl; return true;
}


bool JSONInput::Uint(unsigned u)
{
  cout << "Uint(" << u << ")" << endl; return true;
}


bool JSONInput::Int64(int64_t i)
{
  cout << "Int64(" << i << ")" << endl; return true;
}


bool JSONInput::Uint64(uint64_t u)
{
  cout << "Uint64(" << u << ")" << endl; return true;
}


bool JSONInput::Double(double d)
{
  cout << "Double(" << d << ")" << endl; return true;
}


bool JSONInput::String(const char* str, rapidjson::SizeType length, bool copy)
{
    cout << "String(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
    return true;
}


bool JSONInput::StartObject()
{
  cout << "StartObject()" << endl; return true;
}


bool JSONInput::Key(const char* str, rapidjson::SizeType length, bool copy)
{
    cout << "Key(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
    return true;
}


bool JSONInput::EndObject(rapidjson::SizeType memberCount)
{
  cout << "EndObject(" << memberCount << ")" << endl; return true;
}


bool JSONInput::StartArray()
{
  cout << "StartArray()" << endl; return true;
}


bool JSONInput::EndArray(rapidjson::SizeType elementCount)
{
  cout << "EndArray(" << elementCount << ")" << endl; return true;
}

}       // end namespace
