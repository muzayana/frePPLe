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
#include "frepple/model.h"

namespace frepple
{


DECLARE_EXPORT Plan* Plan::thePlan;
DECLARE_EXPORT const MetaCategory* Plan::metadata;


int Plan::initialize()
{
  // Initialize the plan metadata.
  metadata = new MetaCategory("plan","");

  // Initialize the Python type
  PythonType& x = PythonExtension<Plan>::getType();
  x.setName("parameters");
  x.setDoc("frePPLe global settings");
  x.supportgetattro();
  x.supportsetattro();
  int tmp = x.typeReady();
  const_cast<MetaCategory*>(metadata)->pythonClass = x.type_object();

  // Create a singleton plan object
  // Since we can count on the initialization being executed only once, also
  // in a multi-threaded configuration, we don't need a more advanced mechanism
  // to protect the singleton plan.
  thePlan = new Plan();

  // Add access to the information with a global attribute.
  PythonInterpreter::registerGlobalObject("settings", &Plan::instance());
  return tmp;
}


DECLARE_EXPORT Plan::~Plan()
{
  // Closing the logfile
  Environment::setLogFile("");

  // Clear the pointer to this singleton object
  thePlan = NULL;
}


DECLARE_EXPORT void Plan::setCurrent (Date l)
{
  // Update the time
  cur_Date = l;

  // Let all operationplans check for new ProblemBeforeCurrent and
  // ProblemBeforeFence problems.
  for (Operation::iterator i = Operation::begin(); i != Operation::end(); ++i)
    i->setChanged();
}


DECLARE_EXPORT void Plan::writeElement (XMLOutput *o, const Keyword& tag, mode m) const
{
  // No references
  assert(m != REFERENCE);

  // Write the head
  if (m != NOHEAD && m != NOHEADTAIL) o->BeginObject(tag);

  // Write all own fields
  o->writeElement(Tags::tag_name, name);
  o->writeElement(Tags::tag_description, descr);
  o->writeElement(Tags::tag_current, cur_Date);
  Plannable::writeElement(o, tag);

  // Persist all categories
  MetaCategory::persist(o);

  // Write the tail
  if (m != NOHEADTAIL && m != NOTAIL) o->EndObject(tag);
}


DECLARE_EXPORT void Plan::endElement (XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  if (pAttr.isA(Tags::tag_current))
    setCurrent(pElement.getDate());
  else if (pAttr.isA(Tags::tag_source))
    pIn.setSource(pElement.getString());
  else if (pAttr.isA(Tags::tag_description))
    pElement >> descr;
  else if (pAttr.isA(Tags::tag_name))
    pElement >> name;
  else if (pAttr.isA(Tags::tag_logfile))
    Environment::setLogFile(pElement.getString());
  else
    Plannable::endElement(pIn, pAttr, pElement);
}


DECLARE_EXPORT void Plan::beginElement(XMLInput& pIn, const Attribute& pAttr)
{
  const MetaCategory *cat = MetaCategory::findCategoryByGroupTag(pIn.getParentElement().first.getHash());
  if (cat)
  {
    if (cat->readFunction)
      // Hand over control to a registered read controller
      pIn.readto(cat->readFunction(cat,pIn.getAttributes()));
    else
      // There is no controller available.
      // This piece of code will be used to skip pieces of the XML file that
      // frePPLe doesn't need to be understand.
      pIn.IgnoreElement();
  }
}


DECLARE_EXPORT PyObject* Plan::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_name))
    return PythonObject(Plan::instance().getName());
  if (attr.isA(Tags::tag_description))
    return PythonObject(Plan::instance().getDescription());
  if (attr.isA(Tags::tag_current))
    return PythonObject(Plan::instance().getCurrent());
  if (attr.isA(Tags::tag_logfile))
    return PythonObject(Environment::getLogFile());
  return NULL;
}


DECLARE_EXPORT int Plan::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_name))
    Plan::instance().setName(field.getString());
  else if (attr.isA(Tags::tag_description))
    Plan::instance().setDescription(field.getString());
  else if (attr.isA(Tags::tag_current))
    Plan::instance().setCurrent(field.getDate());
  else if (attr.isA(Tags::tag_logfile))
    Environment::setLogFile(field.getString());
  else
    return -1; // Error
  return 0;  // OK
}

}
