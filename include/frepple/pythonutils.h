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

/** @file pythonutils.h
  * @brief Reusable functions for python functionality.
  *
  * Utility classes for interfacing with the Python language.
  */

#include "frepple/utils.h"

namespace frepple
{
namespace utils
{

/** @brief A template class to expose category classes which use a string
  * as the key to Python. */
template <class T>
class FreppleCategory : public PythonExtension< FreppleCategory<T> >
{
  public:
    /** Initialization method. */
    static int initialize()
    {
      // Initialize the type
      PythonType& x = PythonExtension< FreppleCategory<T> >::getType();
      x.setName(T::metadata->type);
      x.setDoc("frePPLe " + T::metadata->type);
      x.supportgetattro();
      x.supportsetattro();
      x.supportstr();
      x.supportcompare();
      x.supportcreate(Object::create<T>);
      const_cast<MetaCategory*>(T::metadata)->pythonClass = x.type_object();
      return x.typeReady();
    }
};


/** @brief A template class to expose classes to Python. */
template <class ME, class BASE>
class FreppleClass  : public PythonExtension< FreppleClass<ME,BASE> >
{
  public:
    static int initialize()
    {
      // Initialize the type
      PythonType& x = PythonExtension< FreppleClass<ME,BASE> >::getType();
      x.setName(ME::metadata->type);
      x.setDoc("frePPLe " + ME::metadata->type);
      x.supportgetattro();
      x.supportsetattro();
      x.supportstr();
      x.supportcompare();
      x.supportcreate(Object::create<ME>);
      x.setBase(BASE::metadata->pythonClass);
      x.addMethod("toXML", ME::toXML, METH_VARARGS, "return a XML representation");
      const_cast<MetaClass*>(ME::metadata)->pythonClass = x.type_object();
      return x.typeReady();
    }
};

} // end namespace
} // end namespace
