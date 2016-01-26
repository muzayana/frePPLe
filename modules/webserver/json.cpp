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
//#define PARSE_DEBUG

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
    {
      t.close();
      throw DataException("Maximum JSON file size is 100MB");
    }
    t.seekg(0, std::ios::beg);
    char *buffer = new char[length];
    t.read(buffer, length);
    t.close();

    // Parse the data
    JSONInput::parse(pRoot, buffer);
  }
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
  if (!pRoot)
    throw DataException("Can't parse JSON data into NULL root object");

  // Initialize the parser to read data into the object pRoot.
  objectindex = -1;
  dataindex = -1;
  objects[0].start = 0;
  objects[0].object = pRoot;
  objects[0].cls = &pRoot->getType();
  objects[0].hash = pRoot->getType().typetag->getHash();

  // Call the rapidjson in-site parser.
  // The parser will modify the string buffer during the parsing!
  rapidjson::InsituStringStream buf(buffer);
  rapidjson::Reader reader;
  reader.Parse(buf, *this);
}


bool JSONInput::Null()
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setNull();

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::Bool(bool b)
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setBool(b);

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::Int(int i)
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setInt(i);

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::Uint(unsigned u)
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setLong(u);

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::Int64(int64_t i)
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setLong(i);

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::Uint64(uint64_t u)
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setUnsignedLong(u);

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::Double(double d)
{
  if (dataindex < 0)
    return true;

  data[dataindex].value.setDouble(d);

  if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::String(const char* str, rapidjson::SizeType length, bool copy)
{
  if (dataindex < 0)
    return true;

  // Note: JSON allows NULLs in the string values. FrePPLe doesn't, and the
  // next line will only copy the part before the null characters.
  // In XML, null characters are officially forbidden.
  data[dataindex].value.setString(str);

  if (data[dataindex].hash == Tags::type.getHash())
  {
    // Immediate processing of the type field
    objects[objectindex].cls = MetaClass::findClass(str);
    if (!objects[objectindex].cls)
      throw DataException("Unknown type " + string(str));
  }
  else if (objectindex == 0 && objects[objectindex].object
    && data[dataindex].field && !data[dataindex].field->isGroup())
  {
    // Immediately process updates to the root object
    data[dataindex].field->setField(objects[objectindex].object, data[dataindex].value);
    --dataindex;
  }
  return true;
}


bool JSONInput::StartObject()
{
  if (++objectindex >= maxobjects)
    // You're joking?
    throw DataException("JSON-document nested excessively deep");

  // Reset the pointer to the object class being read
  if (objectindex && dataindex >= 0 && data[dataindex].field)
  {
    objects[objectindex].cls = data[dataindex].field->getClass();
    objects[objectindex].object = NULL;
    objects[objectindex].start = dataindex + 1;
  }
  else if (objectindex)
    objects[objectindex].cls = NULL;

  // Debugging message
  #ifdef PARSE_DEBUG
  logger << "Starting object #" << objectindex
    << " (type " << (objects[objectindex].cls ? objects[objectindex].cls->type : "NULL")<< ")" << endl;
  #endif
  return true;
}


bool JSONInput::Key(const char* str, rapidjson::SizeType length, bool copy)
{

  if (++dataindex >= maxdata)
    // You're joking?
    throw DataException("JSON-document nested excessively deep");

  // Look up the field
  data[dataindex].value.setNull();
  data[dataindex].hash = Keyword::hash(str);
  data[dataindex].name = str;

  if (objects[objectindex].cls)
  {
    data[dataindex].field = objects[objectindex].cls->findField(data[dataindex].hash);
    if (!data[dataindex].field && objects[objectindex].cls->category)
      data[dataindex].field = objects[objectindex].cls->category->findField(data[dataindex].hash);
    if (!data[dataindex].field && data[dataindex].hash != Tags::type.getHash())
      throw DataException("Field '" + string(str) + "' not defined");
  }
  else
    data[dataindex].field = NULL;

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
  // Build a dictionary with all fields of this model
  JSONDataValueDict dict(data, objects[objectindex].start, dataindex);

  // Check if we need to add a parent object to the dict
  bool found_parent = false;
  if (objectindex > 0 && objects[objectindex].cls->parent)
  {
    assert(objects[objectindex-1].cls);
    const MetaClass* cl = objects[objectindex-1].cls;
    for (MetaClass::fieldlist::const_iterator i = objects[objectindex].cls->getFields().begin();
      i != objects[objectindex].cls->getFields().end(); ++i)
      if ((*i)->getFlag(PARENT) && objectindex >= 1)
      {
        const MetaFieldBase* fld = data[objects[objectindex].start-1].field;
        if (fld && !fld->isGroup())
          // Only under a group field can we inherit from a parent object
          continue;
        if (*((*i)->getClass()) == *cl
          || (cl->category && *((*i)->getClass()) == *(cl->category)))
        {
          // Parent object matches expected type as parent field
          // First, create the parent object. It is normally created only
          // AFTER all its fields are read in, and that's too late for us.
          if (!objects[objectindex-1].object)
          {
            JSONDataValueDict dict_parent(data, objects[objectindex-1].start, objects[objectindex].start-1);
            if (objects[objectindex-1].cls->category)
            {
              assert(objects[objectindex-1].cls->category->readFunction);
              objects[objectindex-1].object =
                objects[objectindex-1].cls->category->readFunction(
                  objects[objectindex-1].cls,
                  dict_parent
                  );
            }
            else
            {
              assert(static_cast<const MetaCategory*>(objects[objectindex-1].cls)->readFunction);
              objects[objectindex-1].object =
                static_cast<const MetaCategory*>(objects[objectindex-1].cls)->readFunction(
                  objects[objectindex-1].cls,
                  dict_parent
                  );
            }
            // Set fields already available now on the parent object
            for (int idx = objects[objectindex-1].start; idx < objects[objectindex].start; ++idx)
            {
              if (data[idx].hash == Tags::type.getHash() || data[idx].hash == Tags::action.getHash())
                continue;
              if (data[idx].field && !data[idx].field->isGroup())
              {
                  data[idx].field->setField(objects[objectindex-1].object, data[idx].value);
                  data[idx].field = NULL; // Mark as already applied
              }
              else if (data[idx].hash == Tags::booleanproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 1);
              else if (data[idx].hash == Tags::dateproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 2);
              else if (data[idx].hash == Tags::doubleproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 3);
              else if (data[idx].hash == Tags::stringproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 4);
            }

          }
          // Add reference to parent to the current dict
          if (++dataindex >= maxdata)
            // You're joking?
            throw DataException("JSON-document nested excessively deep");
          data[dataindex].field = *i;
          data[dataindex].hash = (*i)->getHash();
          data[dataindex].value.setObject(objects[objectindex-1].object);
          dict.enlarge();
          found_parent = true;
          break;
        }
      }
  }
  if (!found_parent && objectindex > 0
    && objects[objectindex].cls->category
    && objects[objectindex].cls->category->parent)
  {
    assert(objects[objectindex-1].cls);
    const MetaClass* cl = objects[objectindex-1].cls;
    for (MetaClass::fieldlist::const_iterator i = objects[objectindex].cls->category->getFields().begin();
      i != objects[objectindex].cls->category->getFields().end(); ++i)
      if ((*i)->getFlag(PARENT) && objectindex >= 1)
      {
        const MetaFieldBase* fld = data[objects[objectindex].start-1].field;
        if (fld && !fld->isGroup())
          // Only under a group field can we inherit from a parent object
          continue;
        if (*((*i)->getClass()) == *cl
          || (cl->category && *((*i)->getClass()) == *(cl->category)))
        {
          // Parent object matches expected type as parent field
          // First, create the parent object. It is normally created only
          // AFTER all its fields are read in, and that's too late for us.
          if (!objects[objectindex-1].object)
          {
            JSONDataValueDict dict_parent(data, objects[objectindex-1].start, objects[objectindex].start-1);
            if (objects[objectindex-1].cls->category)
            {
              assert(objects[objectindex-1].cls->category->readFunction);
              objects[objectindex-1].object =
                objects[objectindex-1].cls->category->readFunction(
                  objects[objectindex-1].cls,
                  dict_parent
                  );
            }
            else
            {
              assert(static_cast<const MetaCategory*>(objects[objectindex-1].cls)->readFunction);
              objects[objectindex-1].object =
                static_cast<const MetaCategory*>(objects[objectindex-1].cls)->readFunction(
                  objects[objectindex-1].cls,
                  dict_parent
                  );
            }
            // Set fields already available now on the parent object
            for (int idx = objects[objectindex-1].start; idx < objects[objectindex].start; ++idx)
            {
              if (data[idx].hash == Tags::type.getHash() || data[idx].hash == Tags::action.getHash())
                continue;
              if (data[idx].field && !data[idx].field->isGroup())
              {
                  data[idx].field->setField(objects[objectindex-1].object, data[idx].value);
                  data[idx].field = NULL; // Mark as already applied
              }
              else if (data[idx].hash == Tags::booleanproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 1);
              else if (data[idx].hash == Tags::dateproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 2);
              else if (data[idx].hash == Tags::doubleproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 3);
              else if (data[idx].hash == Tags::stringproperty.getHash())
                objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 4);
            }
          }
          // Add reference to parent to the current dict
          if (++dataindex >= maxdata)
            // You're joking?
            throw DataException("JSON-document nested excessively deep");
          data[dataindex].field = *i;
          data[dataindex].hash = (*i)->getHash();
          data[dataindex].value.setObject(objects[objectindex-1].object);
          dict.enlarge();
          break;
        }
      }
  }

  // Debugging
  #ifdef PARSE_DEBUG
  logger << "Ending Object #" << objectindex << " ("
    << ((objectindex >= 0 && objects[objectindex].cls) ? objects[objectindex].cls->type : "none")
    << "):" << endl;
  dict.print();
  #endif

  // Root object never gets created
  if (!objectindex)
    return true;

  // Call the object factory for the category and pass all field values
  // in a dictionary.
  // In some cases, the reading of the child fields already triggered the
  // creation of the parent. In such cases we can skip the creation step
  // here.
  if (!objects[objectindex].object)
  {
    if (objects[objectindex].cls->category)
    {
      assert(objects[objectindex].cls->category->readFunction);
      objects[objectindex].object =
        objects[objectindex].cls->category->readFunction(
          objects[objectindex].cls,
          dict
          );
    }
    else
    {
      assert(static_cast<const MetaCategory*>(objects[objectindex].cls)->readFunction);
      objects[objectindex].object =
        static_cast<const MetaCategory*>(objects[objectindex].cls)->readFunction(
          objects[objectindex].cls,
          dict
          );
    }
  }

  // Update all fields on the new object
  if (objects[objectindex].object)
  {
    for (int idx = dict.getStart(); idx <= dict.getEnd(); ++idx)
    {
      if (data[idx].hash == Tags::type.getHash() || data[idx].hash == Tags::action.getHash())
        continue;
      if (data[idx].field && !data[idx].field->isGroup())
        data[idx].field->setField(objects[objectindex].object, data[idx].value);
      else if (data[idx].hash == Tags::booleanproperty.getHash())
        objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 1);
      else if (data[idx].hash == Tags::dateproperty.getHash())
        objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 2);
      else if (data[idx].hash == Tags::doubleproperty.getHash())
        objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 3);
      else if (data[idx].hash == Tags::stringproperty.getHash())
        objects[objectindex].object->setProperty(data[idx].name, data[idx].value, 4);
    }
  }

  if (objectindex && dataindex && data[dict.getStart()-1].field && data[dict.getStart()-1].field->isPointer())
    // Update parent object
    data[dict.getStart()-1].value.setObject(objects[objectindex].object);

  // Update stack
  dataindex = objects[objectindex--].start - 1;
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
      return NULL;
    case JSON_OBJECT:
      return data_object;
  }
  throw DataException("Unknown JSON type");
}


void JSONDataValueDict::print()
{
  for (int i = strt; i <= nd; ++i)
  {
    if (fields[i].field)
      logger << "  " << i << "   " << fields[i].field->getName().getName() << ": ";
    else
      logger << "  " << i << "   null: ";
    Object *obj = static_cast<Object*>(fields[i].value.getObject());
    if (obj)
      logger << "pointer to " << obj->getType().type << endl;
    else
      logger << fields[i].value.getString() << endl;
  }
}


const JSONData* JSONDataValueDict::get(const Keyword& key) const
{
  for (int i = strt; i <= nd; ++i)
    if (fields[i].hash == key.getHash())
      return &fields[i].value;
  return NULL;
}


}       // end namespace
