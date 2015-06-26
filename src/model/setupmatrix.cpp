/***************************************************************************
 *                                                                         *
 * Copyright (C) 2009-2015 by frePPLe bvba                                 *
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

template<class SetupMatrix> DECLARE_EXPORT Tree utils::HasName<SetupMatrix>::st;
DECLARE_EXPORT const MetaCategory* SetupMatrix::metadata;
DECLARE_EXPORT const MetaClass* SetupMatrixDefault::metadata;
DECLARE_EXPORT const MetaCategory* SetupMatrixRule::metadata;


int SetupMatrix::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<SetupMatrix>("setupmatrix", "setupmatrices", reader, writer, finder);
  registerFields<SetupMatrix>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python class
  FreppleCategory<SetupMatrix>::getPythonType().addMethod("addRule",
    addPythonRule, METH_VARARGS | METH_KEYWORDS, "add a new setup rule");
  return FreppleCategory<SetupMatrix>::initialize()
      + SetupMatrixRuleIterator::initialize();
}


int SetupMatrixRule::initialize()
{
  // Initialize the metadata
  metadata = MetaCategory::registerCategory<SetupMatrixRule>("setupmatrixrule", "setupmatrixrules");
  registerFields<SetupMatrixRule>(const_cast<MetaCategory*>(metadata));

  // Initialize the Python class
  PythonType& x = PythonExtension<SetupMatrixRule>::getPythonType();
  x.setName("setupmatrixrule");
  x.setDoc("frePPLe setupmatrixrule");
  x.supportgetattro();
  x.supportsetattro();
  const_cast<MetaCategory*>(metadata)->pythonClass = x.type_object();
  return x.typeReady();
}


int SetupMatrixDefault::initialize()
{
  // Initialize the metadata
  SetupMatrixDefault::metadata = MetaClass::registerClass<SetupMatrixDefault>(
    "setupmatrix",
    "setupmatrix_default",
    Object::create<SetupMatrixDefault>, true);

  // Initialize the Python class
  return FreppleClass<SetupMatrixDefault,SetupMatrix>::initialize();
}


DECLARE_EXPORT SetupMatrix::~SetupMatrix()
{
  // Destroy the rules.
  // Note that the rule destructor updates the firstRule field.
  while (firstRule) delete firstRule;

  // Remove all references to this setup matrix from resources
  for (Resource::iterator m = Resource::begin(); m != Resource::end(); ++m)
    if (m->getSetupMatrix() == this) m->setSetupMatrix(NULL);
}


DECLARE_EXPORT SetupMatrixRule* SetupMatrix::createRule(const DataValueDict& atts)
{
  // Pick up the start, end and name attributes
  int priority = atts.get(Tags::priority)->getInt();

  // Check for existence of a rule with the same priority
  SetupMatrixRule* result = firstRule;
  while (result && priority > result->priority)
    result = result->nextRule;
  if (result && result->priority != priority) result = NULL;

  // Pick up the action attribute and update the rule accordingly
  switch (MetaClass::decodeAction(atts))
  {
    case ADD:
      // Only additions are allowed
      if (result)
      {
        ostringstream o;
        o << "Rule with priority "  << priority
          << " already exists in setup matrix '" << getName() << "'";
        throw DataException(o.str());
      }
      result = new SetupMatrixRule(this, priority);
      return result;
    case CHANGE:
      // Only changes are allowed
      if (!result)
      {
        ostringstream o;
        o << "No rule with priority " << priority
          << " exists in setup matrix '" << getName() << "'";
        throw DataException(o.str());
      }
      return result;
    case REMOVE:
      // Delete the entity
      if (!result)
      {
        ostringstream o;
        o << "No rule with priority " << priority
          << " exists in setup matrix '" << getName() << "'";
        throw DataException(o.str());
      }
      else
      {
        // Delete it
        delete result;
        return NULL;
      }
    case ADD_CHANGE:
      if (!result)
        // Adding a new rule
        result = new SetupMatrixRule(this, priority);
      return result;
  }

  // This part of the code isn't expected not be reached
  throw LogicException("Unreachable code reached");
}


DECLARE_EXPORT PyObject* SetupMatrix::addPythonRule(PyObject* self, PyObject* args, PyObject* kwdict)
{
  try
  {
    // Pick up the setup matrix
    SetupMatrix *matrix = static_cast<SetupMatrix*>(self);
    if (!matrix) throw LogicException("Can't add a rule to a NULL setupmatrix");

    // Parse the arguments
    int prio = 0;
    PyObject *pyfrom = NULL;
    PyObject *pyto = NULL;
    long duration = 0;
    double cost = 0;
    static const char *kwlist[] = {"priority", "fromsetup", "tosetup", "duration", "cost", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwdict,
        "i|ssld:addRule",
        const_cast<char**>(kwlist), &prio, &pyfrom, &pyto, &duration, &cost))
      return NULL;

    // Add the new rule
    SetupMatrixRule *r = new SetupMatrixRule(matrix, prio);
    if (pyfrom) r->setFromSetup(PythonData(pyfrom).getString());
    if (pyto) r->setToSetup(PythonData(pyfrom).getString());
    r->setDuration(duration);
    r->setCost(cost);
    return PythonData(r);
  }
  catch(...)
  {
    PythonType::evalException();
    return NULL;
  }
}


DECLARE_EXPORT SetupMatrixRule::SetupMatrixRule(SetupMatrix *s, int p)
  : cost(0), priority(p), matrix(s), nextRule(NULL), prevRule(NULL)
{
  // Validate the arguments
  if (!matrix) throw DataException("Can't add a rule to NULL setup matrix");

  // Find the right place in the list
  SetupMatrixRule *next = matrix->firstRule, *prev = NULL;
  while (next && p > next->priority)
  {
    prev = next;
    next = next->nextRule;
  }

  // Duplicate priority
  if (next && next->priority == p)
    throw DataException("Multiple rules with identical priority in setup matrix");

  // Maintain linked list
  nextRule = next;
  prevRule = prev;
  if (prev) prev->nextRule = this;
  else matrix->firstRule = this;
  if (next) next->prevRule = this;

  // Initialize the Python type
  initType(metadata);
}


DECLARE_EXPORT SetupMatrixRule::~SetupMatrixRule()
{
  // Maintain linked list
  if (nextRule) nextRule->prevRule = prevRule;
  if (prevRule) prevRule->nextRule = nextRule;
  else matrix->firstRule = nextRule;
}


DECLARE_EXPORT void SetupMatrixRule::setPriority(const int n)
{
  // Update the field
  priority = n;

  // Check ordering on the left
  while (prevRule && priority < prevRule->priority)
  {
    SetupMatrixRule* next = nextRule;
    SetupMatrixRule* prev = prevRule;
    if (prev && prev->prevRule) prev->prevRule->nextRule = this;
    else matrix->firstRule = this;
    if (prev) prev->nextRule = nextRule;
    nextRule = prev;
    prevRule = prev ? prev->prevRule : NULL;
    if (next && next->nextRule) next->nextRule->prevRule = prev;
    if (next) next->prevRule = prev;
    if (prev) prev->prevRule = this;
  }

  // Check ordering on the right
  while (nextRule && priority > nextRule->priority)
  {
    SetupMatrixRule* next = nextRule;
    SetupMatrixRule* prev = prevRule;
    nextRule = next->nextRule;
    if (next && next->nextRule) next->nextRule->prevRule = this;
    if (prev) prev->nextRule = next;
    if (next) next->nextRule = this;
    if (next) next->prevRule = prev;
    prevRule = next;
  }

  // Check for duplicate priorities
  if ((prevRule && prevRule->priority == priority)
      || (nextRule && nextRule->priority == priority))
  {
    ostringstream o;
    o << "Duplicate priority " << priority << " in setup matrix '"
      << matrix->getName() << "'";
    throw DataException(o.str());
  }
}


int SetupMatrixRuleIterator::initialize()
{
  // Initialize the type
  PythonType& x = PythonExtension<SetupMatrixRuleIterator>::getPythonType();
  x.setName("setupmatrixRuleIterator");
  x.setDoc("frePPLe iterator for setupmatrix rules");
  x.supportiter();
  return x.typeReady();
}


PyObject* SetupMatrixRuleIterator::iternext()
{
  if (currule == matrix->endRules()) return NULL;
  PyObject *result = &*(currule++);
  Py_INCREF(result);
  return result;
}


DECLARE_EXPORT SetupMatrixRule* SetupMatrix::calculateSetup
(const string oldsetup, const string newsetup) const
{
  // No need to look
  if (oldsetup == newsetup) return NULL;

  // Loop through all rules
  for (SetupMatrixRule *curRule = firstRule; curRule; curRule = curRule->nextRule)
  {
    // Need a match on the fromsetup
    if (!curRule->getFromSetup().empty()
        && !matchWildcard(curRule->getFromSetup().c_str(), oldsetup.c_str()))
      continue;
    // Need a match on the tosetup
    if (!curRule->getToSetup().empty()
        && !matchWildcard(curRule->getToSetup().c_str(), newsetup.c_str()))
      continue;
    // Found a match
    return curRule;
  }

  // No matching rule was found
  logger << "Warning: Conversion from '" << oldsetup << "' to '" << newsetup
      << "' undefined in setup matrix '" << getName() << endl;
  return NULL;
}

} // end namespace
