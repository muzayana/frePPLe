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

template<class Demand> DECLARE_EXPORT Tree utils::HasName<Demand>::st;
DECLARE_EXPORT const MetaCategory* Demand::metadata;
DECLARE_EXPORT const MetaClass* DemandDefault::metadata;


int Demand::initialize()
{
  // Initialize the metadata
  metadata = new MetaCategory("demand", "demands", reader, writer, finder);

  // Initialize the Python class
  return FreppleCategory<Demand>::initialize();
}


int DemandDefault::initialize()
{
  // Initialize the metadata
  DemandDefault::metadata = new MetaClass(
    "demand",
    "demand_default",
    Object::createString<DemandDefault>, true);

  // Initialize the Python class
  return FreppleClass<DemandDefault,Demand>::initialize();
}


DECLARE_EXPORT void Demand::setQuantity(double f)
{
  // Reject negative quantities, and no-change updates
  double delta(f - qty);
  if (f < 0.0 || fabs(delta)<ROUNDING_ERROR) return;

  // Update the quantity
  qty = f;
  setChanged();
}


DECLARE_EXPORT Demand::~Demand()
{
  // Reset the motive on all operationplans marked with this demand.
  // This loop is linear with the model size. It doesn't scale well, but
  // deleting a demand is not too common.
  for (OperationPlan::iterator i; i != OperationPlan::end(); i++)
    if (i->getMotive() == this) i->setMotive(NULL);

  // Remove the delivery operationplans
  deleteOperationPlans(true);
}


DECLARE_EXPORT void Demand::deleteOperationPlans
(bool deleteLocked, CommandManager* cmds)
{
  // Delete all delivery operationplans.
  // Note that an extra loop is used to assure that our iterator doesn't get
  // invalidated during the deletion.
  while (true)
  {
    // Find a candidate to delete
    OperationPlan *candidate = NULL;
    for (OperationPlan_list::iterator i = deli.begin(); i!=deli.end(); ++i)
      if (deleteLocked || !(*i)->getLocked())
      {
        candidate = *i;
        break;
      }
    if (!candidate) break;
    if (cmds)
      // Use delete command
      cmds->add(new CommandDeleteOperationPlan(candidate));
    else
      // Delete immediately
      delete candidate;
  }


  // Mark the demand as being changed, so the problems can be redetected
  setChanged();
}


DECLARE_EXPORT void Demand::removeDelivery(OperationPlan * o)
{
  // Valid opplan check
  if (!o) return;

  // See if the demand field on the operationplan points to this demand
  if (o->dmd != this)
    throw LogicException("Delivery operationplan incorrectly registered");

  // Remove the reference on the operationplan
  o->dmd = NULL;  // Required to avoid endless loop
  o->setDemand(NULL);

  // Find in the list of deliveries
  OperationPlan_list::iterator j = deli.begin();
  while (j!=deli.end() && *j!=o) ++j;

  // Check that the operation is found
  // It is possible it is not found! This happens if e.g. an operationplan
  // is created but destroyed again before it is initialized.
  if (j!=deli.end())
  {
    // Remove from the list
    deli.erase(j);
    // Mark the demand as being changed, so the problems can be redetected
    setChanged();
  }
}


DECLARE_EXPORT const Demand::OperationPlan_list& Demand::getDelivery() const
{
  // We need to check the sorting order of the list first! It could be disturbed
  // when operationplans are being moved around.
  // The sorting routine isn't very efficient, but should suffice since the
  // list of delivery operationplans is short and isn't expected to be
  // disturbed very often.
  for (bool swapped(!deli.empty()); swapped; swapped=false)
  {
    OperationPlan_list::iterator j = const_cast<Demand*>(this)->deli.begin();
    ++j;
    for (OperationPlan_list::iterator i =
        const_cast<Demand*>(this)->deli.begin();
        j!=const_cast<Demand*>(this)->deli.end(); ++j)
    {
      if ((*i)->getDates().getEnd() < (*j)->getDates().getEnd())
      {
        // Oh yes, the ordering was disrupted indeed...
        iter_swap(i,j);
        // The Borland compiler doesn't understand that this variable is used.
        // It gives a incorrect warning message...
        swapped = true;
        break;
      }
      ++i;
    }
  }

  return deli;
}


DECLARE_EXPORT OperationPlan* Demand::getLatestDelivery() const
{
  const Demand::OperationPlan_list& l = getDelivery();
  return l.empty() ? NULL : *(l.begin());
}


DECLARE_EXPORT OperationPlan* Demand::getEarliestDelivery() const
{
  const Demand::OperationPlan_list& l = getDelivery();
  OperationPlan *last = NULL;
  for (Demand::OperationPlan_list::const_iterator i = l.begin(); i!=l.end(); ++i)
    last = *i;
  return last;
}


DECLARE_EXPORT void Demand::addDelivery (OperationPlan * o)
{
  // Dummy call to this function
  if (!o) return;

  // Check if it is already in the list.
  // If it is, simply exit the function. No need to give a warning message
  // since it's harmless.
  for (OperationPlan_list::iterator i = deli.begin(); i!=deli.end(); ++i)
    if (*i == o) return;

  // Add to the list of delivery operationplans. The insertion is such
  // that the delivery list is sorted in terms of descending end time.
  // i.e. the opplan with the latest end date is on the front of the list.
  // Note: We're forcing resorting the deliveries with the getDelivery()
  // method. Operation plans dates could have changed, thus disturbing the
  // original order.
  getDelivery();
  OperationPlan_list::iterator j = deli.begin();
  while (j!=deli.end() && (*j)->getDates().getEnd()>o->getDates().getEnd()) ++j;
  deli.insert(j, o);

  // Mark the demand as being changed, so the problems can be redetected
  setChanged();

  // Create link between operationplan and demand
  o->setDemand(this);

  // Check validity of operation being used
  Operation* tmpOper = getDeliveryOperation();
  if (tmpOper && tmpOper != o->getOperation())
    logger << "Warning: Delivery Operation '" << o->getOperation()
        << "' different than expected '" << tmpOper
        << "' for demand '" << this << "'" << endl;
}


DECLARE_EXPORT Operation* Demand::getDeliveryOperation() const
{
  // Operation can be specified on the demand itself,
  if (oper) return oper;
  // ... or on the item,
  if (it) return it->getOperation();
  // ... or it doesn't exist at all
  return NULL;
}


DECLARE_EXPORT double Demand::getPlannedQuantity() const
{
  double delivered(0.0);
  for (OperationPlan_list::const_iterator i=deli.begin(); i!=deli.end(); ++i)
    delivered += (*i)->getQuantity();
  return delivered;
}


DECLARE_EXPORT void Demand::writeElement(Serializer *o, const Keyword& tag, mode m) const
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
  HasHierarchy<Demand>::writeElement(o, tag);
  o->writeElement(Tags::tag_operation, oper);
  o->writeElement(Tags::tag_customer, cust);
  Plannable::writeElement(o, tag);

  o->writeElement(Tags::tag_quantity, qty);
  o->writeElement(Tags::tag_item, it);
  o->writeElement(Tags::tag_due, dueDate);
  if (getPriority()) o->writeElement(Tags::tag_priority, getPriority());
  if (getMaxLateness() != TimePeriod::MAX)
    o->writeElement(Tags::tag_maxlateness, getMaxLateness());
  if (getMinShipment() != 1.0)
    o->writeElement(Tags::tag_minshipment, getMinShipment());

  if (!deli.empty())
  {
    if (o->getContentType() == Serializer::PLAN)
    {
      // Write the delivery plan
      o->BeginList(Tags::tag_operationplans);
      for (OperationPlan_list::const_iterator i=deli.begin(); i!=deli.end(); ++i)
        o->writeElement(Tags::tag_operationplan, *i, FULL);
      o->EndList(Tags::tag_operationplans);
    }
    else if (o->getContentType() == Serializer::PLANDETAIL)
    {
      // Write all pegged operationplans
      o->BeginList(Tags::tag_operationplans);
      for (PeggingIterator i(this); i; --i)
      {
        const OperationPlan* oplan = i.getOperationPlan();
        o->BeginObject(
          Tags::tag_operationplan, Tags::tag_id, oplan->getIdentifier(),
          Tags::tag_operation, oplan->getOperation()->getName()
          );
        if (oplan->getDemand())
          o->writeElement(Tags::tag_demand, oplan->getDemand());
        o->writeElement(Tags::tag_start, oplan->getDates().getStart());
        o->writeElement(Tags::tag_end, oplan->getDates().getEnd());
        // The pegged quantity and level are the only extra field we export in addition
        // to the standard export fields of an operationplan.
        o->writeElement(Tags::tag_pegging, i.getQuantity());
        o->writeElement(Tags::tag_level, i.getLevel());
        o->writeElement(Tags::tag_quantity, oplan->getQuantity());
        o->writeElement(Tags::tag_criticality, oplan->getCriticality());
        if (oplan->getLocked()) o->writeElement (Tags::tag_locked, true);
        if (!oplan->getConsumeMaterial()) o->writeElement(Tags::tag_consume_material, false);
        if (!oplan->getProduceMaterial()) o->writeElement(Tags::tag_produce_material, false);
        if (!oplan->getConsumeCapacity()) o->writeElement(Tags::tag_consume_capacity, false);
        o->writeElement(Tags::tag_source, oplan->getSource());
        o->writeElement(Tags::tag_owner, oplan->owner);
        o->EndObject(Tags::tag_operationplan);
      }
      o->EndList(Tags::tag_operationplans);
    }
  }

  // Write the problems and constraints
  if (o->getContentType() == Serializer::PLAN
      || o->getContentType() == Serializer::PLANDETAIL)
  {
    bool first = true;
    for (Problem::const_iterator j = Problem::begin(const_cast<Demand*>(this), true); j!=Problem::end(); ++j)
    {
      if (first)
      {
        first = false;
        o->BeginList(Tags::tag_problems);
      }
      o->writeElement(Tags::tag_problem, *j, FULL);
    }
    if (!first) o->EndList(Tags::tag_problems);
    if (!constraints.empty())
    {
      o->BeginList(Tags::tag_constraints);
      for (Problem::const_iterator i = constraints.begin(); i != constraints.end(); ++i)
        o->writeElement(Tags::tag_problem, *i, FULL);
      o->EndList(Tags::tag_constraints);
    }
  }

  // Write the custom fields
  PythonDictionary::write(o, getDict());

  // Write the tail
  if (m != NOHEADTAIL && m != NOTAIL) o->EndObject(tag);
}


DECLARE_EXPORT void Demand::beginElement(XMLInput& pIn, const Attribute& pAttr)
{
  if (pAttr.isA (Tags::tag_item))
    pIn.readto( Item::reader(Item::metadata,pIn.getAttributes()) );
  else if (pAttr.isA (Tags::tag_operation))
    pIn.readto( Operation::reader(Operation::metadata,pIn.getAttributes()) );
  else if (pAttr.isA (Tags::tag_customer))
    pIn.readto( Customer::reader(Customer::metadata,pIn.getAttributes()) );
  else if (pAttr.isA(Tags::tag_operationplan))
    pIn.readto(OperationPlan::createOperationPlan(OperationPlan::metadata,pIn.getAttributes()));
  else
  {
    PythonDictionary::read(pIn, pAttr, getDict());
    HasHierarchy<Demand>::beginElement(pIn, pAttr);
  }
}


DECLARE_EXPORT void Demand::endElement(XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
{
  if (pAttr.isA (Tags::tag_quantity))
    setQuantity (pElement.getDouble());
  else if (pAttr.isA (Tags::tag_priority))
    setPriority (pElement.getInt());
  else if (pAttr.isA (Tags::tag_due))
    setDue(pElement.getDate());
  else if (pAttr.isA (Tags::tag_operation))
  {
    Operation *o = dynamic_cast<Operation*>(pIn.getPreviousObject());
    if (o) setOperation(o);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA (Tags::tag_customer))
  {
    Customer *c = dynamic_cast<Customer*>(pIn.getPreviousObject());
    if (c) setCustomer(c);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA (Tags::tag_item))
  {
    Item *i = dynamic_cast<Item*>(pIn.getPreviousObject());
    if (i) setItem(i);
    else throw LogicException("Incorrect object type during read operation");
  }
  else if (pAttr.isA (Tags::tag_maxlateness))
    setMaxLateness(pElement.getTimeperiod());
  else if (pAttr.isA (Tags::tag_minshipment))
    setMinShipment(pElement.getDouble());
  else if (pAttr.isA(Tags::tag_operationplan))
  {
    OperationPlan* opplan
      = dynamic_cast<OperationPlan*>(pIn.getPreviousObject());
    if (opplan) addDelivery(opplan);
    else throw LogicException("Incorrect object type during read operation");
  }
  else
  {
    Plannable::endElement(pIn, pAttr, pElement);
    HasDescription::endElement(pIn, pAttr, pElement);
    HasHierarchy<Demand>::endElement (pIn, pAttr, pElement);
  }
}


DECLARE_EXPORT PyObject* Demand::getattro(const Attribute& attr)
{
  if (attr.isA(Tags::tag_name))
    return PythonObject(getName());
  if (attr.isA(Tags::tag_quantity))
    return PythonObject(getQuantity());
  if (attr.isA(Tags::tag_due))
    return PythonObject(getDue());
  if (attr.isA(Tags::tag_priority))
    return PythonObject(getPriority());
  if (attr.isA(Tags::tag_owner))
    return PythonObject(getOwner());
  if (attr.isA(Tags::tag_item))
    return PythonObject(getItem());
  if (attr.isA(Tags::tag_customer))
    return PythonObject(getCustomer());
  if (attr.isA(Tags::tag_operation))
    return PythonObject(getOperation());
  if (attr.isA(Tags::tag_description))
    return PythonObject(getDescription());
  if (attr.isA(Tags::tag_category))
    return PythonObject(getCategory());
  if (attr.isA(Tags::tag_subcategory))
    return PythonObject(getSubCategory());
  if (attr.isA(Tags::tag_source))
    return PythonObject(getSource());
  if (attr.isA(Tags::tag_minshipment))
    return PythonObject(getMinShipment());
  if (attr.isA(Tags::tag_maxlateness))
    return PythonObject(getMaxLateness());
  if (attr.isA(Tags::tag_hidden))
    return PythonObject(getHidden());
  if (attr.isA(Tags::tag_operationplans))
    return new DemandPlanIterator(this);
  if (attr.isA(Tags::tag_pegging))
    return new PeggingIterator(this);
  if (attr.isA(Tags::tag_constraints))
    return new ProblemIterator(*(constraints.begin()));
  if (attr.isA(Tags::tag_members))
    return new DemandIterator(this);
  return NULL;
}


DECLARE_EXPORT int Demand::setattro(const Attribute& attr, const PythonObject& field)
{
  if (attr.isA(Tags::tag_name))
    setName(field.getString());
  else if (attr.isA(Tags::tag_priority))
    setPriority(field.getInt());
  else if (attr.isA(Tags::tag_quantity))
    setQuantity(field.getDouble());
  else if (attr.isA(Tags::tag_due))
    setDue(field.getDate());
  else if (attr.isA(Tags::tag_item))
  {
    if (!field.check(Item::metadata))
    {
      PyErr_SetString(PythonDataException, "demand item must be of type item");
      return -1;
    }
    Item* y = static_cast<Item*>(static_cast<PyObject*>(field));
    setItem(y);
  }
  else if (attr.isA(Tags::tag_customer))
  {
    if (!field.check(Customer::metadata))
    {
      PyErr_SetString(PythonDataException, "demand customer must be of type customer");
      return -1;
    }
    Customer* y = static_cast<Customer*>(static_cast<PyObject*>(field));
    setCustomer(y);
  }
  else if (attr.isA(Tags::tag_description))
    setDescription(field.getString());
  else if (attr.isA(Tags::tag_category))
    setCategory(field.getString());
  else if (attr.isA(Tags::tag_subcategory))
    setSubCategory(field.getString());
  else if (attr.isA(Tags::tag_source))
    setSource(field.getString());
  else if (attr.isA(Tags::tag_minshipment))
    setMinShipment(field.getDouble());
  else if (attr.isA(Tags::tag_maxlateness))
    setMaxLateness(field.getTimeperiod());
  else if (attr.isA(Tags::tag_owner))
  {
    if (!field.check(Demand::metadata))
    {
      PyErr_SetString(PythonDataException, "demand owner must be of type demand");
      return -1;
    }
    Demand* y = static_cast<Demand*>(static_cast<PyObject*>(field));
    setOwner(y);
  }
  else if (attr.isA(Tags::tag_operation))
  {
    if (!field.check(Operation::metadata))
    {
      PyErr_SetString(PythonDataException, "demand operation must be of type operation");
      return -1;
    }
    Operation* y = static_cast<Operation*>(static_cast<PyObject*>(field));
    setOperation(y);
  }
  else if (attr.isA(Tags::tag_hidden))
    setHidden(field.getBool());
  else
    return -1;  // Error
  return 0;  // OK
}


int DemandPlanIterator::initialize()
{
  // Initialize the type
  PythonType& x = PythonExtension<DemandPlanIterator>::getType();
  x.setName("demandplanIterator");
  x.setDoc("frePPLe iterator for demand delivery operationplans");
  x.supportiter();
  return x.typeReady();
}


PyObject* DemandPlanIterator::iternext()
{
  if (i == dem->getDelivery().end()) return NULL;
  PyObject* result = const_cast<OperationPlan*>(&**(i++));
  Py_INCREF(result);
  return result;
}

} // end namespace
