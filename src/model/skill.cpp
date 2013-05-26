/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba                 *
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

template<class Skill> DECLARE_EXPORT Tree utils::HasName<Skill>::st;
DECLARE_EXPORT const MetaCategory* Skill::metadata;
DECLARE_EXPORT const MetaClass* SkillDefault::metadata;


int Skill::initialize()
{
  // Initialize the metadata
  metadata = new MetaCategory("skill", "skills", reader, writer);

  // Initialize the Python class
  return FreppleCategory<Skill>::initialize();
}


int SkillDefault::initialize()
{
  // Initialize the metadata
  SkillDefault::metadata = new MetaClass(
    "skill",
    "skill_default",
    Object::createString<SkillDefault>,
    true);

  // Initialize the Python class
  return FreppleClass<SkillDefault,Skill>::initialize();
}


DECLARE_EXPORT void Skill::writeElement(XMLOutput *o, const Keyword& tag, mode m) const
{
  // Write a reference
  if (m == REFERENCE)
  {
    o->writeElement(tag, Tags::tag_name, getName());
    return;
  }

  // Write the complete object
  if (m != NOHEADER) o->BeginObject(tag, Tags::tag_name, XMLEscape(getName()));

  // That was it
  o->EndObject(tag);
}


DECLARE_EXPORT void Skill::beginElement(XMLInput& pIn, const Attribute& pAttr)
{
  if (pAttr.isA(Tags::tag_resourceskill)
      && pIn.getParentElement().first.isA(Tags::tag_resourceskills))
  {
    ResourceSkill *s =
      dynamic_cast<ResourceSkill*>(MetaCategory::ControllerDefault(ResourceSkill::metadata,pIn.getAttributes()));
    if (s) s->setSkill(this);
    pIn.readto(s);
  }
}


DECLARE_EXPORT void Skill::endElement (XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  // No specific fields to retrieve
}


DECLARE_EXPORT Skill::~Skill()
{
  // The ResourceSkill objects are automatically deleted by the destructor
  // of the Association list class.

  // Clean up the references on the load models
  for (Operation::iterator o = Operation::begin(); o != Operation::end(); ++o)
    for(Operation::loadlist::const_iterator l = o->getLoads().begin();
      l != o->getLoads().end(); ++l)
      if (l->getSkill() == this)
        const_cast<Load&>(*l).setSkill(NULL);
}


DECLARE_EXPORT PyObject* Skill::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_name))
    return PythonObject(getName());
  if (attr.isA(Tags::tag_resourceskills))
    return new ResourceSkillIterator(this);
  return NULL;
}


DECLARE_EXPORT int Skill::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_name))
    setName(field.getString());
  else
    return -1;  // Error
  return 0;  // OK
}


}
