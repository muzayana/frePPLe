/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2015 by Johan De Taeye, frePPLe bvba                 *
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
#include <sys/stat.h>

namespace frepple
{

void LibraryModel::initialize()
{
  // Initialize only once
  static bool init = false;
  if (init)
  {
    logger << "Warning: Calling frepple::LibraryModel::initialize() more "
        << "than once." << endl;
    return;
  }
  init = true;

  // Register new types in Python
  int nok = 0;
  nok += Plan::initialize();

  // Initialize the solver metadata.
  nok += Solver::initialize();

  // Initialize the location metadata.
  nok += Location::initialize();
  nok += LocationDefault::initialize();
  nok += LocationIterator::initialize();

  // Initialize the customer metadata.
  nok += Customer::initialize();
  nok += CustomerDefault::initialize();
  nok += CustomerIterator::initialize();

  // Initialize the supplier metadata.
  nok += Supplier::initialize();
  nok += SupplierDefault::initialize();
  nok += SupplierIterator::initialize();

  // Initialize the calendar metadata.
  nok += CalendarBucket::initialize();
  nok += Calendar::initialize();
  nok += CalendarDefault::initialize();
  nok += CalendarIterator::initialize();

  // Initialize the operation metadata.
  nok += Operation::initialize();
  nok += OperationAlternate::initialize();
  nok += OperationSplit::initialize();
  nok += OperationFixedTime::initialize();
  nok += OperationTimePer::initialize();
  nok += OperationRouting::initialize();
  nok += OperationSetup::initialize();
  nok += SubOperation::initialize();
  nok += OperationIterator::initialize();
  nok += SubOperationIterator::initialize();

  // Initialize the item metadata.
  nok += Item::initialize();
  nok += ItemDefault::initialize();
  nok += ItemIterator::initialize();

  // Initialize the supplieritem metadata
  nok += SupplierItem::initialize();
  nok += SupplierItemIterator::initialize();

  // Initialize the buffer metadata.
  nok += Buffer::initialize();
  nok += BufferDefault::initialize();
  nok += BufferInfinite::initialize();
  nok += BufferProcure::initialize();
  nok += BufferIterator::initialize();

  // Initialize the demand metadata.
  nok += Demand::initialize();
  nok += DemandIterator::initialize();
  nok += DemandDefault::initialize();
  nok += DemandPlanIterator::initialize();

  // Initialize the setupmatrix metadata.
  nok += SetupMatrixRule::initialize();
  nok += SetupMatrix::initialize();
  nok += SetupMatrixDefault::initialize();
  nok += SetupMatrixIterator::initialize();

  // Initialize the skill metadata
  nok += Skill::initialize();
  nok += SkillDefault::initialize();
  nok += SkillIterator::initialize();

  // Initialize the resource metadata.
  nok += Resource::initialize();
  nok += ResourceDefault::initialize();
  nok += ResourceInfinite::initialize();
  nok += ResourceIterator::initialize();
  nok += Resource::PlanIterator::initialize();
  nok += ResourceBuckets::initialize();

  // Initialize the resourceskill metadata
  nok += ResourceSkill::initialize();
  nok += ResourceSkillIterator::initialize();

  // Initialize the load metadata.
  nok += Load::initialize();
  nok += LoadIterator::initialize();
  nok += LoadPlan::initialize();
  nok += LoadPlanIterator::initialize();

  // Initialize the flow metadata.
  nok += Flow::initialize();
  nok += FlowIterator::initialize();
  nok += FlowPlan::initialize();
  nok += FlowPlanIterator::initialize();

  // Initialize the operationplan metadata.
  nok += OperationPlan::initialize();
  nok += OperationPlanIterator::initialize();

  // Initialize the problem metadata.
  nok += Problem::initialize();
  nok += ProblemIterator::initialize();

  // Initialize the pegging metadata.
  nok += PeggingIterator::initialize();

  // Exit if errors were found
  if (nok) throw RuntimeException("Error registering new Python types");

  // Register new methods in Python
  PythonInterpreter::registerGlobalMethod(
    "printsize", printModelSize, METH_NOARGS,
    "Print information about the memory consumption.");
  PythonInterpreter::registerGlobalMethod(
    "erase", eraseModel, METH_VARARGS,
    "Removes the plan data from memory, and optionally the static info too.");
  PythonInterpreter::registerGlobalMethod(
    "readXMLdata", readXMLdata, METH_VARARGS,
    "Processes a XML string passed as argument.");
  PythonInterpreter::registerGlobalMethod(
    "readXMLfile", readXMLfile, METH_VARARGS,
    "Read an XML file.");
  PythonInterpreter::registerGlobalMethod(
    "saveXMLfile", saveXMLfile, METH_VARARGS,
    "Save the model to a XML file.");
  PythonInterpreter::registerGlobalMethod(
    "saveplan", savePlan, METH_VARARGS,
    "Save the main plan information to a file.");
  PythonInterpreter::registerGlobalMethod(
    "buffers", BufferIterator::create, METH_NOARGS,
    "Returns an iterator over the buffers.");
  PythonInterpreter::registerGlobalMethod(
    "locations", LocationIterator::create, METH_NOARGS,    // TODO Replace with a attribute?
    "Returns an iterator over the locations.");
  PythonInterpreter::registerGlobalMethod(
    "customers", CustomerIterator::create, METH_NOARGS,
    "Returns an iterator over the customers.");
  PythonInterpreter::registerGlobalMethod(
    "suppliers", SupplierIterator::create, METH_NOARGS,
    "Returns an iterator over the suppliers.");
  PythonInterpreter::registerGlobalMethod(
    "items", ItemIterator::create, METH_NOARGS,
    "Returns an iterator over the items.");
  PythonInterpreter::registerGlobalMethod(
    "calendars", CalendarIterator::create, METH_NOARGS,
    "Returns an iterator over the calendars.");
  PythonInterpreter::registerGlobalMethod(
    "demands", DemandIterator::create, METH_NOARGS,
    "Returns an iterator over the demands.");
  PythonInterpreter::registerGlobalMethod(
    "resources", ResourceIterator::create, METH_NOARGS,
    "Returns an iterator over the resources.");
  PythonInterpreter::registerGlobalMethod(
    "operations", OperationIterator::create, METH_NOARGS,
    "Returns an iterator over the operations.");
  PythonInterpreter::registerGlobalMethod(
    "operationplans", OperationPlanIterator::create, METH_NOARGS,
    "Returns an iterator over the operationplans.");
  PythonInterpreter::registerGlobalMethod(
    "problems", ProblemIterator::create, METH_NOARGS,
    "Returns an iterator over the problems.");
  PythonInterpreter::registerGlobalMethod(
    "setupmatrices", SetupMatrixIterator::create, METH_NOARGS,
    "Returns an iterator over the setup matrices.");
  PythonInterpreter::registerGlobalMethod(
    "skills", SkillIterator::create, METH_NOARGS,
    "Returns an iterator over the skills.");
}


}
