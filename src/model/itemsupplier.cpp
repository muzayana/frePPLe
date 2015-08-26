/***************************************************************************
 *                                                                         *
 * Copyright (C) 2015 by frePPLe bvba                                      *
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

DECLARE_EXPORT const MetaCategory* ItemSupplier::metacategory;
DECLARE_EXPORT const MetaClass* ItemSupplier::metadata;
DECLARE_EXPORT const MetaClass* OperationItemSupplier::metadata;


int ItemSupplier::initialize()
{
  // Initialize the metadata
  metacategory = MetaCategory::registerCategory<ItemSupplier>(
	  "itemsupplier", "itemsuppliers", MetaCategory::ControllerDefault
	  );
  metadata = MetaClass::registerClass<ItemSupplier>(
    "itemsupplier", "itemsupplier", Object::create<ItemSupplier>, true
  );
  registerFields<ItemSupplier>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  PythonType& x = FreppleCategory<ItemSupplier>::getPythonType();
  x.setName("itemsupplier");
  x.setDoc("frePPLe itemsupplier");
  x.supportgetattro();
  x.supportsetattro();
  x.supportcreate(create);
  x.addMethod("toXML", toXML, METH_VARARGS, "return a XML representation");
  const_cast<MetaClass*>(ItemSupplier::metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT ItemSupplier::~ItemSupplier()
{
  // Delete the association from the related objects
  if (getSupplier())
    getSupplier()->items.erase(this);
  if (getItem())
    getItem()->suppliers.erase(this);

  // Delete all owned purchase operations
  while (firstOperation)
    delete firstOperation;

  // Trigger level and cluster recomputation
  HasLevel::triggerLazyRecomputation();
}


DECLARE_EXPORT ItemSupplier::ItemSupplier() : loc(NULL),
  size_minimum(1.0), size_multiple(0.0), cost(0.0), firstOperation(NULL)
{
  initType(metadata);

  // Trigger level and cluster recomputation
  HasLevel::triggerLazyRecomputation();
}


DECLARE_EXPORT ItemSupplier::ItemSupplier(Supplier* s, Item* r, int u)
  : loc(NULL), size_minimum(1.0), size_multiple(0.0), cost(0.0), firstOperation(NULL)
{
  setSupplier(s);
  setItem(r);
  setPriority(u);
  initType(metadata);

  // Trigger level and cluster recomputation
  HasLevel::triggerLazyRecomputation();
}


DECLARE_EXPORT ItemSupplier::ItemSupplier(Supplier* s, Item* r, int u, DateRange e)
  : loc(NULL), size_minimum(1.0), size_multiple(0.0), cost(0.0), firstOperation(NULL)
{
  setSupplier(s);
  setItem(r);
  setPriority(u);
  setEffective(e);
  initType(metadata);

  // Trigger level and cluster recomputation
  HasLevel::triggerLazyRecomputation();
}


PyObject* ItemSupplier::create(PyTypeObject* pytype, PyObject* args, PyObject* kwds)
{
  try
  {
    // Pick up the supplier
    PyObject* sup = PyDict_GetItemString(kwds,"supplier");
    if (!sup)
      throw DataException("missing supplier on ItemSupplier");
    if (!PyObject_TypeCheck(sup, Supplier::metadata->pythonClass))
      throw DataException("ItemSupplier supplier must be of type supplier");

    // Pick up the item
    PyObject* it = PyDict_GetItemString(kwds,"item");
    if (!it)
      throw DataException("missing item on ItemSupplier");
    if (!PyObject_TypeCheck(it, Item::metadata->pythonClass))
      throw DataException("ItemSupplier item must be of type item");

    // Pick up the priority
    PyObject* q1 = PyDict_GetItemString(kwds,"priority");
    int q2 = q1 ? PythonData(q1).getInt() : 1;

    // Pick up the effective dates
    DateRange eff;
    PyObject* eff_start = PyDict_GetItemString(kwds,"effective_start");
    if (eff_start)
    {
      PythonData d(eff_start);
      eff.setStart(d.getDate());
    }
    PyObject* eff_end = PyDict_GetItemString(kwds,"effective_end");
    if (eff_end)
    {
      PythonData d(eff_end);
      eff.setEnd(d.getDate());
    }

    // Create the ItemSupplier
    ItemSupplier *l = new ItemSupplier(
      static_cast<Supplier*>(sup),
      static_cast<Item*>(it),
      q2, eff
    );

    // Iterate over extra keywords, and set attributes.   @todo move this responsibility to the readers...
    if (l)
    {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(kwds, &pos, &key, &value))
      {
        PythonData field(value);
        PyObject* key_utf8 = PyUnicode_AsUTF8String(key);
        DataKeyword attr(PyBytes_AsString(key_utf8));
        Py_DECREF(key_utf8);
        if (!attr.isA(Tags::effective_end) && !attr.isA(Tags::effective_start)
          && !attr.isA(Tags::supplier) && !attr.isA(Tags::item)
          && !attr.isA(Tags::type) && !attr.isA(Tags::action))
        {
          const MetaFieldBase* fmeta = l->getType().findField(attr.getHash());
          if (!fmeta && l->getType().category)
            fmeta = l->getType().category->findField(attr.getHash());
          if (fmeta)
            // Update the attribute
            fmeta->setField(l, field);
          else
            PyErr_Format(PyExc_AttributeError,
                "attribute '%S' on '%s' can't be updated",
                key, Py_TYPE(l)->tp_name);
        }
      };
    }

    // Return the object
    Py_INCREF(l);
    return static_cast<PyObject*>(l);
  }
  catch (...)
  {
    PythonType::evalException();
    return NULL;
  }
}


DECLARE_EXPORT void ItemSupplier::validate(Action action)
{
  // Catch null supplier and item pointers
  Supplier *sup = getSupplier();
  Item *it = getItem();
  Location *loc = getLocation();
  if (!sup || !it)
  {
    // Invalid ItemSupplier model
    if (!sup && !it)
      throw DataException("Missing supplier and item on a itemsupplier");
    else if (!sup)
      throw DataException("Missing supplier on a itemsupplier on item '"
          + it->getName() + "'");
    else if (!it)
      throw DataException("Missing item on a itemsupplier on supplier '"
          + sup->getName() + "'");
  }

  // Check if a ItemSupplier with 1) identical supplier, 2) identical item
  // 3) identical location, and 4) overlapping effectivity dates already exists
  Supplier::itemlist::const_iterator i = sup->getItems().begin();
  for (; i != sup->getItems().end(); ++i)
    if (i->getItem() == it
        && i->getEffective().overlap(getEffective())
        && i->getLocation() == loc
        && &*i != this)
      break;

  // Apply the appropriate action
  switch (action)
  {
    case ADD:
      if (i != sup->getItems().end())
      {
        throw DataException("ItemSupplier of '" + sup->getName() + "' and '"
            + it->getName() + "' already exists");
      }
      break;
    case CHANGE:
      throw DataException("Can't update a itemsupplier");
    case ADD_CHANGE:
      // ADD is handled in the code after the switch statement
      if (i == sup->getItems().end()) break;
      throw DataException("Can't update a itemsupplier");
    case REMOVE:
      // This ItemSupplier was only used temporarily during the reading process
      delete this;
      if (i == sup->getItems().end())
        // Nothing to delete
        throw DataException("Can't remove nonexistent itemsupplier of '"
            + sup->getName() + "' and '" + it->getName() + "'");
      delete &*i;
      return;
  }
}


DECLARE_EXPORT void ItemSupplier::deleteOperationPlans(bool b)
{
  for (OperationItemSupplier* i = firstOperation; i; i = i->nextOperation)
    i->deleteOperationPlans(b);
}


int OperationItemSupplier::initialize()
{
  // Initialize the metadata
  metadata = MetaClass::registerClass<OperationItemSupplier>(
    "operation", "operation_itemsupplier"
    );
  registerFields<OperationItemSupplier>(const_cast<MetaClass*>(metadata));

  // Initialize the Python class
  PythonType& x = FreppleCategory<OperationItemSupplier>::getPythonType();
  x.setName("operation_itemsupplier");
  x.setDoc("frePPLe operation_itemsupplier");
  x.addMethod("createOrder", createOrder,
    METH_STATIC | METH_VARARGS | METH_KEYWORDS,
    "Create an operationplan representing a purchase order");
  x.supportgetattro();
  const_cast<MetaClass*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


DECLARE_EXPORT OperationItemSupplier* OperationItemSupplier::findOrCreate(
  ItemSupplier* i, Buffer *b
  )
{
  if (!i || !b || !i->getSupplier())
    throw LogicException(
      "An OperationItemSupplier always needs to point to "
      "a itemsupplier and a buffer"
      );
  stringstream o;
  o << "Purchase " << b->getName() << " from " << i->getSupplier()->getName();
  Operation *oper = Operation::find(o.str());
  if (oper)
  {
    // Reuse existing operation
    if (oper->getType() == *OperationItemSupplier::metadata)
      return static_cast<OperationItemSupplier*>(oper);
    else
      throw DataException("Unexpected operation type for item supplier operation");
  }
  else
    // Create new operation
    return new OperationItemSupplier(i, b);
}


DECLARE_EXPORT OperationItemSupplier::OperationItemSupplier(
  ItemSupplier* i, Buffer *b
  ) : supitem(i)
{
  if (!i || !b || !i->getSupplier())
    throw LogicException(
      "An OperationItemSupplier always needs to point to "
      "a itemsupplier and a buffer"
      );
  stringstream o;
  o << "Purchase " << b->getName() << " from " << i->getSupplier()->getName();
  setName(o.str());
  setDuration(i->getLeadTime());
  setSizeMultiple(i->getSizeMultiple());
  setSizeMinimum(i->getSizeMinimum());
  setLocation(b->getLocation());
  setSource(i->getSource());
  setCost(i->getCost());
  setHidden(true);
  FlowEnd* fl = new FlowEnd(this, b, 1);
  initType(metadata);

  // Insert in the list of ItemSupplier operations.
  // We keep the list sorted by the operation name.
  if (!i->firstOperation || getName() < i->firstOperation->getName())
  {
    // New head of the list
    nextOperation = i->firstOperation;
    i->firstOperation = this;
  }
  else
  {
    // Insert in the middle or at the tail
    OperationItemSupplier* o = i->firstOperation;
    while (o->nextOperation)
    {
      if (b->getName() < o->nextOperation->getName())
        break;
      o = o->nextOperation;
    }
    nextOperation = o->nextOperation;
    o->nextOperation = this;
  }
}


OperationItemSupplier::~OperationItemSupplier()
{
  // Remove from the list of operations of this supplier item
  if (supitem)
  {
    if (supitem->firstOperation == this)
    {
      // We were at the head
      supitem->firstOperation = nextOperation;
    }
    else
    {
      // We were in the middle
      OperationItemSupplier* i = supitem->firstOperation;
      while (i->nextOperation != this && i->nextOperation)
        i = i->nextOperation;
      if (!i)
        throw LogicException("ItemSupplier operation list corrupted");
      else
        i->nextOperation = nextOperation;
    }
  }
}


DECLARE_EXPORT Buffer* OperationItemSupplier::getBuffer() const
{
  return getFlows().begin()->getBuffer();
}


extern "C" PyObject* OperationItemSupplier::createOrder(
  PyObject *self, PyObject *args, PyObject *kwdict
  )
{
  // Parse the Python arguments
  PyObject* pylocation = NULL;
  unsigned long id = 0;
  const char* ref = NULL;
  PyObject* pyitem = NULL;
  PyObject* pysupplier = NULL;
  double qty = 0;
  PyObject* pystart = NULL;
  PyObject* pyend = NULL;
  const char* status = NULL;
  const char* source = NULL;
  static const char *kwlist[] = {
    "location", "id", "reference", "item", "supplier", "quantity", "start",
    "end", "status", "source", NULL
    };
  int ok = PyArg_ParseTupleAndKeywords(
    args, kwdict, "|OkzOOdOOzz:createOrder", const_cast<char**>(kwlist),
    &pylocation, &id, &ref, &pyitem, &pysupplier, &qty, &pystart,
    &pyend, &status, &source
    );
  if (!ok)
    return NULL;
  Date start = pystart ? PythonData(pystart).getDate() : Date::infinitePast;
  Date end = pyend ? PythonData(pyend).getDate() : Date::infinitePast;

  // Validate all arguments
  if (!pylocation || !pyitem)
  {
    PyErr_SetString(PythonDataException, "item and location arguments are mandatory");
    return NULL;
  }
  PythonData location_tmp(pylocation);
  if (!location_tmp.check(Location::metadata))
  {
    PyErr_SetString(PythonDataException, "location argument must be of type location");
    return NULL;
  }
  PythonData item_tmp(pyitem);
  if (!item_tmp.check(Item::metadata))
  {
    PyErr_SetString(PythonDataException, "item argument must be of type item");
    return NULL;
  }
  PythonData supplier_tmp(pysupplier);
  if (pysupplier && !supplier_tmp.check(Supplier::metadata))
  {
    PyErr_SetString(PythonDataException, "supplier argument must be of type supplier");
    return NULL;
  }
  Item *item = static_cast<Item*>(item_tmp.getObject());
  Location *location = static_cast<Location*>(location_tmp.getObject());
  Supplier *supplier = pysupplier ? static_cast<Supplier*>(supplier_tmp.getObject()) : NULL;

  // Find or create the destination buffer.
  Buffer* destbuffer = NULL;
  for (Buffer::iterator bufiter = Buffer::begin(); bufiter != Buffer::end(); ++bufiter)
  {
    if (bufiter->getLocation() == location && bufiter->getItem() == item)
    {
      if (destbuffer)
      {
        stringstream o;
        o << "Multiple buffers found for item '" << item << "'' and location'" << location << "'";
        throw DataException(o.str());
      }
      destbuffer = &*bufiter;
    }
  }
  if (!destbuffer)
  {
    // Create the destination buffer
    destbuffer = new BufferDefault();
    stringstream o;
    o << item << " @ " << location;
    destbuffer->setName(o.str());
    destbuffer->setItem(item);
    destbuffer->setLocation(location);
  }

  // Look for a matching matching supplying operation on this buffer.
  // Here we also trigger the creation of its producing operation, which
  // contains the logic to build possible transfer operations.
  Operation *oper = NULL;
  Operation* prodOper = destbuffer->getProducingOperation();
  if (prodOper && prodOper->getType() == *OperationItemSupplier::metadata)
  {
    if (supplier)
    {
      if (supplier->isMemberOf(static_cast<OperationItemSupplier*>(prodOper)->getItemSupplier()->getSupplier()))
        oper = prodOper;
    }
    else
      oper = prodOper;
  }
  else if (prodOper && prodOper->getType() == *OperationAlternate::metadata)
  {
    SubOperation::iterator soperiter = prodOper->getSubOperationIterator();
    while (SubOperation *soper = soperiter.next())
    {
      if (soper->getType() == *OperationItemSupplier::metadata)
      {
        if (supplier)
        {
          if (supplier->isMemberOf(static_cast<OperationItemSupplier*>(prodOper)->getItemSupplier()->getSupplier()))
          {
            oper = soper->getOperation();
            break;
          }
        }
        else
        {
          oper = prodOper;
          break;
        }
      }
    }
  }

  // No matching operation is found.
  if (!oper)
  {
    // We'll create one now, but that requires that we have a supplier defined.
    if (!supplier)
      throw DataException("Supplier is needed on this purchase order");
    // Note: We know that we need to create a new one. An existing one would
    // have created an operation on the buffer already.
    ItemSupplier *itemsupplier = new ItemSupplier();
    itemsupplier->setSupplier(supplier);
    itemsupplier->setItem(item);
    itemsupplier->setLocation(location);
    oper = new OperationItemSupplier(itemsupplier, destbuffer);
    new ProblemInvalidData(oper, "Purchase orders on unauthorized supplier", "operation",
      Date::infinitePast, Date::infiniteFuture, 1);
  }

  // Finally, create the operationplan
  OperationPlan *opplan = oper->createOperationPlan(qty, start, end);
  if (status)
    opplan->setStatus(status);
  if (ref)
    opplan->setReference(ref);

  // Return result
  Py_INCREF(opplan);
  return opplan;
}

}
