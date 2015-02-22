/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#define FREPPLE_CORE
#include "frepple/utils.h"
#include <sys/stat.h>

// These headers are required for the loading of dynamic libraries and the
// detection of the number of cores.
#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif


namespace frepple
{
namespace utils
{

// Repository of all categories and commands
DECLARE_EXPORT const MetaCategory* MetaCategory::firstCategory = NULL;
DECLARE_EXPORT MetaCategory::CategoryMap MetaCategory::categoriesByTag;
DECLARE_EXPORT MetaCategory::CategoryMap MetaCategory::categoriesByGroupTag;

// Repository of loaded modules
DECLARE_EXPORT set<string> Environment::moduleRegistry;

// Number of processors.
// The value initialized here is updated when the getProcessorCores function
// is called the first time.
DECLARE_EXPORT int Environment::processorcores = -1;

// Output logging stream, whose input buffer is shared with either
// Environment::logfile or cout.
DECLARE_EXPORT ostream logger(cout.rdbuf());

// Output file stream
DECLARE_EXPORT ofstream Environment::logfile;

// Name of the log file
DECLARE_EXPORT string Environment::logfilename;

// Hash value computed only once
DECLARE_EXPORT const hashtype MetaCategory::defaultHash(Keyword::hash("default"));

vector<PythonType*> PythonExtensionBase::table;


void LibraryUtils::initialize()
{
  // Initialize only once
  static bool init = false;
  if (init)
  {
    logger << "Warning: Calling frepple::LibraryUtils::initialize() more "
        << "than once." << endl;
    return;
  }
  init = true;

  // Validate the license file
  LicenseValidator x;

  // Initialize Xerces parser
  xercesc::XMLPlatformUtils::Initialize();

  // Initialize the Python interpreter
  PythonInterpreter::initialize();

  // Register new methods in Python
  PythonInterpreter::registerGlobalMethod(
    "loadmodule", loadModule, METH_VARARGS,
    "Dynamically load a module in memory.");
}


DECLARE_EXPORT string Environment::searchFile(const string filename)
{
#ifdef _MSC_VER
  static char pathseperator = '\\';
#else
  static char pathseperator = '/';
#endif

  // First: check the current directory
  struct stat stat_p;
  int result = stat(filename.c_str(), &stat_p);
  if (!result && (stat_p.st_mode & S_IREAD))
    return filename;

  // Second: check the FREPPLE_HOME directory, if it is defined
  string fullname;
  char * envvar = getenv("FREPPLE_HOME");
  if (envvar)
  {
    fullname = envvar;
    if (*fullname.rbegin() != pathseperator)
      fullname += pathseperator;
    fullname += filename;
    result = stat(fullname.c_str(), &stat_p);
    if (!result && (stat_p.st_mode & S_IREAD))
      return fullname;
  }

#ifdef DATADIRECTORY
  // Third: check the data directory
  fullname = DATADIRECTORY;
  if (*fullname.rbegin() != pathseperator)
    fullname += pathseperator;
  fullname.append(filename);
  result = stat(fullname.c_str(), &stat_p);
  if (!result && (stat_p.st_mode & S_IREAD))
    return fullname;
#endif

#ifdef LIBDIRECTORY
  // Fourth: check the lib directory
  fullname = LIBDIRECTORY;
  if (*fullname.rbegin() != pathseperator)
    fullname += pathseperator;
  fullname += "frepple";
  fullname += pathseperator;
  fullname += filename;
  result = stat(fullname.c_str(), &stat_p);
  if (!result && (stat_p.st_mode & S_IREAD))
    return fullname;
#endif

#ifdef SYSCONFDIRECTORY
  // Fifth: check the sysconf directory
  fullname = SYSCONFDIRECTORY;
  if (*fullname.rbegin() != pathseperator)
    fullname += pathseperator;
  fullname += filename;
  result = stat(fullname.c_str(), &stat_p);
  if (!result && (stat_p.st_mode & S_IREAD))
    return fullname;
#endif

  // Not found
  return "";
}


DECLARE_EXPORT int Environment::getProcessorCores()
{
  // Previously detected already
  if (processorcores >= 1) return processorcores;

  // Detect the number of cores on the machine
#ifdef WIN32
  // Windows
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  processorcores = sysinfo.dwNumberOfProcessors;
#else
  // Linux, Solaris and AIX.
  // Tough luck for other platforms.
  processorcores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  // Detection failed...
  if (processorcores<1) processorcores = 1;
  return processorcores;
}


DECLARE_EXPORT void Environment::setLogFile(const string& x)
{
  // Bye bye message
  if (!logfilename.empty())
    logger << "Stop logging at " << Date::now() << endl;

  // Close an eventual existing log file.
  if (logfile.is_open()) logfile.close();

  // No new logfile specified: redirect to the standard output stream
  if (x.empty() || x == "+")
  {
    logfilename = x;
    logger.rdbuf(cout.rdbuf());
    return;
  }

  // Open the file: either as a new file, either appending to existing file
  if (x[0] != '+') logfile.open(x.c_str(), ios::out);
  else logfile.open(x.c_str()+1, ios::app);
  if (!logfile.good())
  {
    // Redirect to the previous logfile (or cout if that's not possible)
    if (logfile.is_open()) logfile.close();
    logfile.open(logfilename.c_str(), ios::app);
    logger.rdbuf(logfile.is_open() ? logfile.rdbuf() : cout.rdbuf());
    // The log file could not be opened
    throw RuntimeException("Could not open log file '" + x + "'");
  }

  // Store the file name
  logfilename = x;

  // Redirect the log file.
  logger.rdbuf(logfile.rdbuf());

  // Print a nice header
  logger << "Start logging frePPLe " << PACKAGE_VERSION
#ifdef _WIN64
    << " 64-bit"
#endif
	<< " (" << __DATE__ << ") at " << Date::now() << endl;
}


DECLARE_EXPORT void Environment::loadModule(string lib, ParameterList& parameters)
{
  // Type definition of the initialization function
  typedef const char* (*func)(const ParameterList&);

  // Validate
  if (lib.empty())
    throw DataException("Error: No library name specified for loading");

#ifdef WIN32
  // Load the library - The windows way

  // Change the error mode: we handle errors now, not the operating system
  UINT em = SetErrorMode(SEM_FAILCRITICALERRORS);
  HINSTANCE handle = LoadLibraryEx(lib.c_str(),NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
  if (!handle) handle = LoadLibraryEx(lib.c_str(), NULL, 0);
  if (!handle)
  {
    // Get the error description
    char error[256];
    FormatMessage(
      FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      0,
      error,
      256,
      NULL );
    throw RuntimeException(error);
  }
  SetErrorMode(em);  // Restore the previous error mode

  // Find the initialization routine
  func inithandle =
    reinterpret_cast<func>(GetProcAddress(HMODULE(handle), "initialize"));
  if (!inithandle)
  {
    // Get the error description
    char error[256];
    FormatMessage(
      FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      0,
      error,
      256,
      NULL );
    throw RuntimeException(error);
  }

#else
  // Load the library - The UNIX way

  // Search the frePPLe directories for the library
  string fullpath = Environment::searchFile(lib);
  if (fullpath.empty())
    throw RuntimeException("Module '" + lib + "' not found");
  dlerror(); // Clear the previous error
  void *handle = dlopen(fullpath.c_str(), RTLD_NOW | RTLD_GLOBAL);
  const char *err = dlerror();  // Pick up the error string
  if (err)
  {
    // Search the normal path for the library
    dlerror(); // Clear the previous error
    handle = dlopen(lib.c_str(), RTLD_NOW | RTLD_GLOBAL);
    err = dlerror();  // Pick up the error string
    if (err) throw RuntimeException(err);
  }

  // Find the initialization routine
  func inithandle = (func)(dlsym(handle, "initialize"));
  err = dlerror(); // Pick up the error string
  if (err) throw RuntimeException(err);
#endif

  // Call the initialization routine with the parameter list
  string x = (inithandle)(parameters);
  if (x.empty()) throw DataException("Invalid or unlicensed module");

  // Insert the new module in the registry
  moduleRegistry.insert(x);
}


DECLARE_EXPORT void MetaClass::registerClass (const string& a, const string& b,
    bool def, creatorDefault f)
{
  // Find or create the category
  MetaCategory* cat
    = const_cast<MetaCategory*>(MetaCategory::findCategoryByTag(a.c_str()));

  // Check for a valid category
  if (!cat)
    throw LogicException("Category " + a
        + " not found when registering class " + b);

  // Update fields
  type = b.empty() ? "unspecified" : b;
  typetag = &Keyword::find(type.c_str());
  category = cat;

  // Update the metadata table
  cat->classes[Keyword::hash(b)] = this;

  // Register this tag also as the default one, if requested
  if (def) cat->classes[Keyword::hash("default")] = this;

  // Set method pointers to NULL
  factoryMethodDefault = f;
}


DECLARE_EXPORT MetaCategory::MetaCategory (const string& a, const string& gr,
    readController f, writeController w, findController s)
{
  // Update registry
  if (!a.empty()) categoriesByTag[Keyword::hash(a)] = this;
  if (!gr.empty()) categoriesByGroupTag[Keyword::hash(gr)] = this;

  // Update fields
  readFunction = f;
  writeFunction = w;
  findFunction = s;
  type = a.empty() ? "unspecified" : a;
  typetag = &Keyword::find(type.c_str());
  group = gr.empty() ? "unspecified" : gr;
  grouptag = &Keyword::find(group.c_str());

  // Maintain a linked list of all registered categories
  nextCategory = NULL;
  if (!firstCategory)
    firstCategory = this;
  else
  {
    const MetaCategory *i = firstCategory;
    while (i->nextCategory) i = i->nextCategory;
    const_cast<MetaCategory*>(i)->nextCategory = this;
  }
}


DECLARE_EXPORT const MetaCategory* MetaCategory::findCategoryByTag(const char* c)
{
  // Loop through all categories
  CategoryMap::const_iterator i = categoriesByTag.find(Keyword::hash(c));
  return (i!=categoriesByTag.end()) ? i->second : NULL;
}


DECLARE_EXPORT const MetaCategory* MetaCategory::findCategoryByTag(const hashtype h)
{
  // Loop through all categories
  CategoryMap::const_iterator i = categoriesByTag.find(h);
  return (i!=categoriesByTag.end()) ? i->second : NULL;
}


DECLARE_EXPORT const MetaCategory* MetaCategory::findCategoryByGroupTag(const char* c)
{
  // Loop through all categories
  CategoryMap::const_iterator i = categoriesByGroupTag.find(Keyword::hash(c));
  return (i!=categoriesByGroupTag.end()) ? i->second : NULL;
}


DECLARE_EXPORT const MetaCategory* MetaCategory::findCategoryByGroupTag(const hashtype h)
{
  // Loop through all categories
  CategoryMap::const_iterator i = categoriesByGroupTag.find(h);
  return (i!=categoriesByGroupTag.end()) ? i->second : NULL;
}


DECLARE_EXPORT const MetaClass* MetaCategory::findClass(const char* c) const
{
  // Look up in the registered classes
  MetaCategory::ClassMap::const_iterator j = classes.find(Keyword::hash(c));
  return (j == classes.end()) ? NULL : j->second;
}


DECLARE_EXPORT const MetaClass* MetaCategory::findClass(const hashtype h) const
{
  // Look up in the registered classes
  MetaCategory::ClassMap::const_iterator j = classes.find(h);
  return (j == classes.end()) ? NULL : j->second;
}


DECLARE_EXPORT void MetaCategory::persistAll(Serializer* o)
{
  for (const MetaCategory *i = firstCategory; i; i = i->nextCategory)
    i->persist(o);
}


DECLARE_EXPORT const MetaClass* MetaClass::findClass(const char* c)
{
  // Loop through all categories
  for (MetaCategory::CategoryMap::const_iterator i = MetaCategory::categoriesByTag.begin();
      i != MetaCategory::categoriesByTag.end(); ++i)
  {
    // Look up in the registered classes
    MetaCategory::ClassMap::const_iterator j
      = i->second->classes.find(Keyword::hash(c));
    if (j != i->second->classes.end()) return j->second;
  }
  // Not found...
  return NULL;
}


DECLARE_EXPORT void MetaClass::printClasses()
{
  logger << "Registered classes:" << endl;
  // Loop through all categories
  for (MetaCategory::CategoryMap::const_iterator i = MetaCategory::categoriesByTag.begin();
      i != MetaCategory::categoriesByTag.end(); ++i)
  {
    logger << "  " << i->second->type << endl;
    // Loop through the classes for the category
    for (MetaCategory::ClassMap::const_iterator
        j = i->second->classes.begin();
        j != i->second->classes.end();
        ++j)
      if (j->first == Keyword::hash("default"))
        logger << "    default ( = " << j->second->type << " )" << j->second << endl;
      else
        logger << "    " << j->second->type << j->second << endl;
  }
}


DECLARE_EXPORT Action MetaClass::decodeAction(const char *x)
{
  // Validate the action
  if (!x) throw LogicException("Invalid action NULL");
  else if (!strcmp(x,"AC")) return ADD_CHANGE;
  else if (!strcmp(x,"A")) return ADD;
  else if (!strcmp(x,"C")) return CHANGE;
  else if (!strcmp(x,"R")) return REMOVE;
  else throw LogicException("Invalid action '" + string(x) + "'");
}


DECLARE_EXPORT Action MetaClass::decodeAction(const AttributeList& atts)
{
  // Decode the string and return the default in the absence of the attribute
  const DataElement* c = atts.get(Tags::tag_action);
  return *c ? decodeAction(c->getString().c_str()) : ADD_CHANGE;
}


DECLARE_EXPORT bool MetaClass::raiseEvent(Object* v, Signal a) const
{
  bool result(true);
  for (list<Functor*>::const_iterator i = subscribers[a].begin();
      i != subscribers[a].end(); ++i)
    // Note that we always call all subscribers, even if one or more
    // already replied negatively. However, an exception thrown from a
    // callback method will break the publishing chain.
    if (!(*i)->callback(v,a)) result = false;

  // Raise the event also on the category, if there is a valid one
  return (category && category!=this) ?
      (result && category->raiseEvent(v,a)) :
      result;
}


Object* MetaCategory::ControllerDefault (const MetaClass* cat, const AttributeList& in)
{
  Action act = ADD;
  switch (act)
  {
    case REMOVE:
      throw DataException
      ("Entity " + cat->type + " doesn't support REMOVE action");
    case CHANGE:
      throw DataException
      ("Entity " + cat->type + " doesn't support CHANGE action");
    default:
      /* Lookup for the class in the map of registered classes. */
      const MetaClass* j;
      if (cat->category)
        // Class metadata passed: we already know what type to create
        j = cat;
      else
      {
        // Category metadata passed: we need to look up the type
        const DataElement* type = in.get(Tags::tag_type);
        j = static_cast<const MetaCategory&>(*cat).findClass(*type ? Keyword::hash(type->getString()) : MetaCategory::defaultHash);
        if (!j)
        {
          string t(*type ? type->getString() : "default");
          throw LogicException("No type " + t + " registered for category " + cat->type);
        }
      }

      // Call the factory method
      Object* result = j->factoryMethodDefault();

      // Run the callback methods
      if (!result->getType().raiseEvent(result, SIG_ADD))
      {
        // Creation denied
        delete result;
        throw DataException("Can't create object");
      }

      // Creation accepted
      return result;
  }
  throw LogicException("Unreachable code reached");
  return NULL;
}


void HasDescription::writeElement(Serializer* o, const Keyword &t, mode m) const
{
  // Note that this function is never called on its own. It is always called
  // from the writeElement() method of a subclass.
  // Hence, we don't bother about the mode.
  o->writeElement(Tags::tag_category, cat);
  o->writeElement(Tags::tag_subcategory, subcat);
  o->writeElement(Tags::tag_description, descr);
  o->writeElement(Tags::tag_source, getSource());
}


void HasDescription::endElement (XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  if (pAttr.isA(Tags::tag_category))
    setCategory(pElement.getString());
  else if (pAttr.isA(Tags::tag_subcategory))
    setSubCategory(pElement.getString());
  else if (pAttr.isA(Tags::tag_description))
    setDescription(pElement.getString());
  else if (pAttr.isA(Tags::tag_source))
    setSource(pElement.getString());
}


DECLARE_EXPORT bool matchWildcard(const char* wild, const char *str)
{
  // Empty arguments: always return a match
  if (!wild || !str) return 1;

  const char *cp = NULL, *mp = NULL;

  while ((*str) && *wild != '*')
  {
    if (*wild != *str && *wild != '?')
      // Does not match
      return 0;
    wild++;
    str++;
  }

  while (*str)
  {
    if (*wild == '*')
    {
      if (!*++wild) return 1;
      mp = wild;
      cp = str+1;
    }
    else if (*wild == *str || *wild == '?')
    {
      wild++;
      str++;
    }
    else
    {
      wild = mp;
      str = cp++;
    }
  }

  while (*wild == '*') wild++;
  return !*wild;
}

} // end namespace
} // end namespace

