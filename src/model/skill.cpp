/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2015 by frePPLe bvba                                 *
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
  metadata = MetaCategory::registerCategory<Skill>("skill", "skills", reader, finder);
  registerFields<Skill>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python class
  return FreppleCategory<Skill>::initialize();
}


int SkillDefault::initialize()
{
  // Initialize the metadata
  SkillDefault::metadata = MetaClass::registerClass<SkillDefault>(
    "skill",
    "skill_default",
    Object::create<SkillDefault>,
    true);

  // Initialize the Python class
  return FreppleClass<SkillDefault,Skill>::initialize();
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

}
