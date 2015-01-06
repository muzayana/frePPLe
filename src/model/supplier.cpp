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

#define FREPPLE_CORE
#include "frepple/model.h"

namespace frepple
{

template<class Supplier> DECLARE_EXPORT Tree utils::HasName<Supplier>::st;
DECLARE_EXPORT const MetaCategory* Supplier::metadata;
DECLARE_EXPORT const MetaClass* SupplierDefault::metadata;


int Supplier::initialize()
{
  // Initialize the metadata
  metadata = new MetaCategory("supplier", "suppliers", reader, writer);

  // Initialize the Python class
  return FreppleCategory<Supplier>::initialize();
}


int SupplierDefault::initialize()
{
  // Initialize the metadata
  SupplierDefault::metadata = new MetaClass(
    "supplier",
    "supplier_default",
    Object::createString<SupplierDefault>, true);

  // Initialize the Python class
  return FreppleClass<SupplierDefault,Supplier>::initialize();
}


DECLARE_EXPORT void Supplier::writeElement(Serializer* o, const Keyword& tag, mode m) const
{
  // Writing a reference
  if (m == REFERENCE)
  {
    o->writeElement(tag, Tags::tag_name, getName());
    return;
  }

  // Write the head
  if (m != NOHEAD && m != NOHEADTAIL)
    o->BeginObject(tag, Tags::tag_name, getName());

  // Write the fields
  HasDescription::writeElement(o, tag);
  HasHierarchy<Supplier>::writeElement(o, tag);

  // Write the custom fields
  PythonDictionary::write(o, getDict());

  // Write the tail
  if (m != NOTAIL && m != NOHEADTAIL) o->EndObject(tag);
}


DECLARE_EXPORT void Supplier::beginElement(XMLInput& pIn, const Attribute& pAttr)
{
  if (pAttr.isA(Tags::tag_supplieritem)
      && pIn.getParentElement().first.isA(Tags::tag_supplieritems))
  {
    SupplierItem *s =
      dynamic_cast<SupplierItem*>(MetaCategory::ControllerDefault(SupplierItem::metadata,pIn.getAttributes()));
    if (s) s->setSupplier(this);
    pIn.readto(s);
  }
  else
  {
    PythonDictionary::read(pIn, pAttr, getDict());
    HasHierarchy<Supplier>::beginElement(pIn, pAttr);
  }
}


DECLARE_EXPORT void Supplier::endElement(XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  HasDescription::endElement(pIn, pAttr, pElement);
  HasHierarchy<Supplier>::endElement(pIn, pAttr, pElement);
}


DECLARE_EXPORT Supplier::~Supplier()
{
}


DECLARE_EXPORT PyObject* Supplier::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_name))
    return PythonObject(getName());
  if (attr.isA(Tags::tag_description))
    return PythonObject(getDescription());
  if (attr.isA(Tags::tag_category))
    return PythonObject(getCategory());
  if (attr.isA(Tags::tag_subcategory))
    return PythonObject(getSubCategory());
  if (attr.isA(Tags::tag_source))
    return PythonObject(getSource());
  if (attr.isA(Tags::tag_owner))
    return PythonObject(getOwner());
  if (attr.isA(Tags::tag_hidden))
    return PythonObject(getHidden());
  if (attr.isA(Tags::tag_members))
    return new SupplierIterator(this);
  if (attr.isA(Tags::tag_supplieritems))
    return new SupplierItemIterator(this);
  return NULL;
}


DECLARE_EXPORT int Supplier::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_name))
    setName(field.getString());
  else if (attr.isA(Tags::tag_description))
    setDescription(field.getString());
  else if (attr.isA(Tags::tag_category))
    setCategory(field.getString());
  else if (attr.isA(Tags::tag_subcategory))
    setSubCategory(field.getString());
  else if (attr.isA(Tags::tag_source))
    setSource(field.getString());
  else if (attr.isA(Tags::tag_owner))
  {
    if (!field.check(Supplier::metadata))
    {
      PyErr_SetString(PythonDataException, "supplier owner must be of type supplier");
      return -1;
    }
    Supplier* y = static_cast<Supplier*>(static_cast<PyObject*>(field));
    setOwner(y);
  }
  else if (attr.isA(Tags::tag_hidden))
    setHidden(field.getBool());
  else
    return -1;
  return 0;
}


} // end namespace
