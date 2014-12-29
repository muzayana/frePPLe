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

//
// READ XML INPUT FILE
//


DECLARE_EXPORT PyObject* readXMLfile(PyObject* self, PyObject* args)
{
  // Pick up arguments
  char *filename = NULL;
  int validate(1), validate_only(0);
  PyObject *userexit = NULL;
  int ok = PyArg_ParseTuple(args, "|siiO:readXMLfile",
    &filename, &validate, &validate_only, &userexit);
  if (!ok) return NULL;

  // Execute and catch exceptions
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    if (!filename)
    {
      // Read from standard input
      xercesc::StdInInputSource in;
      XMLInput p;
      if (userexit) p.setUserExit(userexit);
      if (validate_only!=0)
        // When no root object is passed, only the input validation happens
        p.parse(in, NULL, true);
      else
        p.parse(in, &Plan::instance(), validate!=0);
    }
    else
    {
      XMLInputFile p(filename);
      if (userexit) p.setUserExit(userexit);
      if (validate_only!=0)
        // Read and validate a file
        p.parse(NULL, true);
      else
        // Read, execute and optionally validate a file
        p.parse(&Plan::instance(),validate!=0);
    }
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}


//
// READ XML INPUT STRING
//


DECLARE_EXPORT PyObject* readXMLdata(PyObject *self, PyObject *args)
{
  // Pick up arguments
  char *data;
  int validate(1), validate_only(0);
  PyObject *userexit = NULL;
  int ok = PyArg_ParseTuple(args, "s|iiO:readXMLdata",
    &data, &validate, &validate_only, &userexit);
  if (!ok) return NULL;

  // Free Python interpreter for other threads
  Py_BEGIN_ALLOW_THREADS

  // Execute and catch exceptions
  try
  {
    if (!data) throw DataException("No input data");
    XMLInputString p(data);
    if (userexit) p.setUserExit(userexit);
    if (validate_only!=0)
      p.parse(NULL, true);
    else
      p.parse(&Plan::instance(), validate!=0);
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");  // Safer than using Py_None, which is not portable across compilers
}


//
// SAVE MODEL TO XML
//


PyObject* saveXMLfile(PyObject* self, PyObject* args)
{
  // Pick up arguments
  char *filename;
  char *content = NULL;
  int ok = PyArg_ParseTuple(args, "s|s:save", &filename, &content);
  if (!ok) return NULL;

  // Execute and catch exceptions
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    XMLOutputFile o(filename);
    if (content)
    {
      if (!strcmp(content,"STANDARD"))
        o.setContentType(XMLOutput::STANDARD);
      else if (!strcmp(content,"PLAN"))
        o.setContentType(XMLOutput::PLAN);
      else if (!strcmp(content,"PLANDETAIL"))
        o.setContentType(XMLOutput::PLANDETAIL);
      else
        throw DataException("Invalid content type '" + string(content) + "'");
    }
    o.writeElementWithHeader(Tags::tag_plan, &Plan::instance());
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}


//
// SAVE PLAN SUMMARY TO TEXT FILE
//


DECLARE_EXPORT PyObject* savePlan(PyObject* self, PyObject* args)
{
  // Pick up arguments
  const char *filename = "plan.out";
  int ok = PyArg_ParseTuple(args, "s:saveplan", &filename);
  if (!ok) return NULL;

  // Free Python interpreter for other threads
  Py_BEGIN_ALLOW_THREADS

  // Execute and catch exceptions
  ofstream textoutput;
  try
  {
    // Open the output file
    textoutput.open(filename, ios::out);

    // Write the buffer summary
    for (Buffer::iterator gbuf = Buffer::begin();
        gbuf != Buffer::end(); ++gbuf)
    {
      if (!gbuf->getHidden())
        for (Buffer::flowplanlist::const_iterator
            oo=gbuf->getFlowPlans().begin();
            oo!=gbuf->getFlowPlans().end();
            ++oo)
          if (oo->getType() == 1 && oo->getQuantity() != 0.0)
          {
            textoutput << "BUFFER\t" << *gbuf << '\t'
                << oo->getDate() << '\t'
                << oo->getQuantity() << '\t'
                << oo->getOnhand() << endl;
          }
    }

    // Write the demand summary
    for (Demand::iterator gdem = Demand::begin();
        gdem != Demand::end(); ++gdem)
    {
      if (!gdem->getHidden())
      {
        const Demand::OperationPlan_list &deli = gdem->getDelivery();
        for (Demand::OperationPlan_list::const_iterator pp = deli.begin();
            pp != deli.end(); ++pp)
          textoutput << "DEMAND\t" << (*gdem) << '\t'
              << (*pp)->getDates().getEnd() << '\t'
              << (*pp)->getQuantity() << endl;
      }
    }

    // Write the resource summary
    for (Resource::iterator gres = Resource::begin();
        gres != Resource::end(); ++gres)
    {
      if (!gres->getHidden())
        for (Resource::loadplanlist::const_iterator
            qq=gres->getLoadPlans().begin();
            qq!=gres->getLoadPlans().end();
            ++qq)
          if (qq->getType() == 1 && qq->getQuantity() != 0.0)
          {
            textoutput << "RESOURCE\t" << *gres << '\t'
                << qq->getDate() << '\t'
                << qq->getQuantity() << '\t'
                << qq->getOnhand() << endl;
          }
    }

    // Write the operationplan summary.
    for (OperationPlan::iterator rr = OperationPlan::begin();
        rr != OperationPlan::end(); ++rr)
    {
      if (rr->getOperation()->getHidden()) continue;
      textoutput << "OPERATION\t" << rr->getOperation() << '\t'
          << rr->getDates().getStart() << '\t'
          << rr->getDates().getEnd() << '\t'
          << rr->getQuantity()
          << (rr->getLocked() ? "\tlocked" : "")
          << endl;
    }

    // Write the problem summary.
    for (Problem::const_iterator gprob = Problem::begin();
        gprob != Problem::end(); ++gprob)
    {
      textoutput << "PROBLEM\t" << gprob->getType().type << '\t'
          << gprob->getDescription() << '\t'
          << gprob->getDates() << endl;
    }

    // Write the constraint summary
    for (Demand::iterator gdem = Demand::begin();
        gdem != Demand::end(); ++gdem)
    {
      if (!gdem->getHidden())
      {
        for (Problem::const_iterator i = gdem->getConstraints().begin();
            i != gdem->getConstraints().end();
            ++i)
          textoutput << "DEMAND CONSTRAINT\t" << (*gdem) << '\t'
              << i->getDescription() << '\t'
              << i->getDates() << '\t' << endl;
      }
    }

    // Close the output file
    textoutput.close();
  }
  catch (...)
  {
    if (textoutput.is_open())
      textoutput.close();
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}


//
// MOVE OPERATIONPLAN
//

DECLARE_EXPORT CommandMoveOperationPlan::CommandMoveOperationPlan
(OperationPlan* o) : opplan(o), firstCommand(NULL)
{
  if (!o)
  {
    originalqty = 0;
    return;
  }
  originalqty = opplan->getQuantity();
  originaldates = opplan->getDates();

  // Construct a subcommand for all suboperationplans
  for (OperationPlan::iterator x(o); x != o->end(); ++x)
    if (x->getOperation() != OperationSetup::setupoperation)
    {
      CommandMoveOperationPlan *n = new CommandMoveOperationPlan(o);
      n->owner = this;
      if (firstCommand)
      {
        n->next = firstCommand;
        firstCommand->prev = n;
      }
      firstCommand = n;
    }
}


DECLARE_EXPORT CommandMoveOperationPlan::CommandMoveOperationPlan
(OperationPlan* o, Date newstart, Date newend, double newQty)
  : opplan(o), firstCommand(NULL)
{
  if (!opplan) return;

  // Store current settings
  originalqty = opplan->getQuantity();
  if (newQty == -1.0) newQty = originalqty;
  originaldates = opplan->getDates();

  // Update the settings
  assert(opplan->getOperation());
  opplan->getOperation()->setOperationPlanParameters(
    opplan, newQty, newstart, newend
  );

  // Construct a subcommand for all suboperationplans
  for (OperationPlan::iterator x(o); x != o->end(); ++x)
    if (x->getOperation() != OperationSetup::setupoperation)
    {
      CommandMoveOperationPlan *n = new CommandMoveOperationPlan(o);
      n->owner = this;
      if (firstCommand)
      {
        n->next = firstCommand;
        firstCommand->prev = n;
      }
      firstCommand = n;
    }
}


DECLARE_EXPORT void CommandMoveOperationPlan::redo()  // @todo not implemented
{
}


DECLARE_EXPORT void CommandMoveOperationPlan::restore(bool del)
{
  // Restore all suboperationplans and (optionally) delete the subcommands
  for (Command *c = firstCommand; c; )
  {
    CommandMoveOperationPlan *tmp = static_cast<CommandMoveOperationPlan*>(c);
    tmp->restore(del);
    c = c->next;
    if (del) delete tmp;
  }

  // Restore the original dates
  if (!opplan) return;
  opplan->getOperation()->setOperationPlanParameters(
    opplan, originalqty, originaldates.getStart(), originaldates.getEnd()
  );
}


//
// DELETE OPERATIONPLAN
//

DECLARE_EXPORT CommandDeleteOperationPlan::CommandDeleteOperationPlan
(OperationPlan* o) : opplan(o)
{
  // Validate input
  if (!o) return;

  // Avoid deleting locked operationplans
  if (o->getLocked())
  {
    opplan = NULL;
    throw DataException("Can't delete a locked operationplan");
  }

  // Deletion always should apply to a top level operationplan
  opplan = opplan->getTopOwner();

  // Delete all flowplans and loadplans, and unregister from operationplan list
  redo();
}


//
// DELETE MODEL
//


DECLARE_EXPORT PyObject* eraseModel(PyObject* self, PyObject* args)
{
  // Pick up arguments
  PyObject *obj = NULL;
  int ok = PyArg_ParseTuple(args, "|O:erase", &obj);
  if (!ok) return NULL;

  // Validate the argument
  bool deleteStaticModel = false;
  if (obj) deleteStaticModel = PythonObject(obj).getBool();

  // Execute and catch exceptions
  Py_BEGIN_ALLOW_THREADS   // Free Python interpreter for other threads
  try
  {
    if (deleteStaticModel)
    {
      // Delete all entities.
      // The order is chosen to minimize the work of the individual destructors.
      // E.g. the destructor of the item class recurses over all demands and
      // all buffers. It is much faster if there are none already.
      Operation::clear();
      Demand::clear();
      Buffer::clear();
      Resource::clear();
      SetupMatrix::clear();
      Location::clear();
      Customer::clear();
      Calendar::clear();
      Solver::clear();
      Item::clear();
      // The setup operation is a static singleton and should always be around
      OperationSetup::setupoperation = Operation::add(new OperationSetup("setup operation"));
    }
    else
      // Delete the operationplans only
      for (Operation::iterator gop = Operation::begin();
          gop != Operation::end(); ++gop)
        gop->deleteOperationPlans();
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}


//
// PRINT MODEL SIZE
//


DECLARE_EXPORT PyObject* printModelSize(PyObject* self, PyObject* args)
{
  // Free Python interpreter for other threads
  Py_BEGIN_ALLOW_THREADS

  // Execute and catch exceptions
  size_t count, memsize;
  try
  {

    // Intro
    logger << endl << "Size information of frePPLe " << PACKAGE_VERSION
#ifdef _WIN64
      << " 64-bit"
#endif
      << " (" << __DATE__ << ")" << endl << endl;

    // Print loaded modules
    Environment::printModules();

    // Print the number of clusters
    logger << "Clusters: " << HasLevel::getNumberOfClusters() << endl << endl;

    // Header for memory size
    logger << "Memory usage:" << endl;
    logger << "Model        \tNumber\tMemory" << endl;
    logger << "-----        \t------\t------" << endl;

    // Plan
    size_t total = Plan::instance().getSize();
    logger << "Plan         \t1\t"<< Plan::instance().getSize() << endl;

    // Locations
    memsize = 0;
    for (Location::iterator l = Location::begin(); l != Location::end(); ++l)
      memsize += l->getSize();
    logger << "Location     \t" << Location::size() << "\t" << memsize << endl;
    total += memsize;

    // Customers
    memsize = 0;
    for (Customer::iterator c = Customer::begin(); c != Customer::end(); ++c)
      memsize += c->getSize();
    logger << "Customer     \t" << Customer::size() << "\t" << memsize << endl;
    total += memsize;

    // Suppliers
    memsize = 0;
    for (Supplier::iterator c = Supplier::begin(); c != Supplier::end(); ++c)
      memsize += c->getSize();
    logger << "Supplier     \t" << Supplier::size() << "\t" << memsize << endl;
    total += memsize;

    // Buffers
    memsize = 0;
    for (Buffer::iterator b = Buffer::begin(); b != Buffer::end(); ++b)
      memsize += b->getSize();
    logger << "Buffer       \t" << Buffer::size() << "\t" << memsize << endl;
    total += memsize;

    // Setup matrices
    memsize = 0;
    for (SetupMatrix::iterator s = SetupMatrix::begin(); s != SetupMatrix::end(); ++s)
      memsize += s->getSize();
    logger << "Setup matrix \t" << SetupMatrix::size() << "\t" << memsize << endl;
    total += memsize;

    // Resources
    memsize = 0;
    for (Resource::iterator r = Resource::begin(); r != Resource::end(); ++r)
      memsize += r->getSize();
    logger << "Resource     \t" << Resource::size() << "\t" << memsize << endl;
    total += memsize;

    // Skills and resourceskills
    size_t countResourceSkills(0), memResourceSkills(0);
    memsize = 0;
    for (Skill::iterator sk = Skill::begin(); sk != Skill::end(); ++sk)
    {
      memsize += sk->getSize();
      for (Skill::resourcelist::const_iterator rs = sk->getResources().begin();
          rs != sk->getResources().end(); ++rs)
      {
        ++countResourceSkills;
        memResourceSkills += rs->getSize();
      }
    }
    logger << "Skill     \t" << Skill::size() << "\t" << memsize << endl;
    logger << "ResourceSkill     \t" << countResourceSkills << "\t" << memResourceSkills << endl;
    total += memsize;

    // Operations, flows and loads
    size_t countFlows(0), memFlows(0), countLoads(0), memLoads(0);
    memsize = 0;
    for (Operation::iterator o = Operation::begin(); o != Operation::end(); ++o)
    {
      memsize += o->getSize();
      for (Operation::flowlist::const_iterator fl = o->getFlows().begin();
          fl != o->getFlows().end(); ++ fl)
      {
        ++countFlows;
        memFlows += fl->getSize();
      }
      for (Operation::loadlist::const_iterator ld = o->getLoads().begin();
          ld != o->getLoads().end(); ++ ld)
      {
        ++countLoads;
        memLoads += ld->getSize();
      }
    }
    logger << "Operation    \t" << Operation::size() << "\t" << memsize << endl;
    logger << "Flow         \t" << countFlows << "\t" << memFlows  << endl;
    logger << "Load         \t" << countLoads << "\t" << memLoads  << endl;
    total += memsize + memFlows + memLoads;

    // Calendars (which includes the buckets)
    memsize = 0;
    for (Calendar::iterator cl = Calendar::begin(); cl != Calendar::end(); ++cl)
      memsize += cl->getSize();
    logger << "Calendar     \t" << Calendar::size() << "\t" << memsize  << endl;
    total += memsize;

    // Items
    memsize = 0;
    for (Item::iterator i = Item::begin(); i != Item::end(); ++i)
      memsize += i->getSize();
    logger << "Item         \t" << Item::size() << "\t" << memsize  << endl;
    total += memsize;

    // Demands
    memsize = 0;
    size_t c_count = 0, c_memsize = 0;
    for (Demand::iterator dm = Demand::begin(); dm != Demand::end(); ++dm)
    {
      memsize += dm->getSize();
      for (Problem::const_iterator cstrnt(dm->getConstraints().begin());
        cstrnt != dm->getConstraints().end(); ++cstrnt)
      {
        ++c_count;
        c_memsize += cstrnt->getSize();
      }
    }
    logger << "Demand       \t" << Demand::size() << "\t" << memsize  << endl;
    logger << "Constraints  \t" << c_count << "\t" << c_memsize  << endl;
    total += memsize + c_memsize;

    // Operationplans
    size_t countloadplans(0), countflowplans(0);
    memsize = count = 0;
    for (OperationPlan::iterator j = OperationPlan::begin();
        j!=OperationPlan::end(); ++j)
    {
      ++count;
      memsize += sizeof(*j);
      countloadplans += j->sizeLoadPlans();
      countflowplans += j->sizeFlowPlans();
    }
    total += memsize;
    logger << "OperationPlan\t" << count << "\t" << memsize << endl;

    // Flowplans
    memsize = countflowplans * sizeof(FlowPlan);
    total +=  memsize;
    logger << "FlowPlan     \t" << countflowplans << "\t" << memsize << endl;

    // Loadplans
    memsize = countloadplans * sizeof(LoadPlan);
    total +=  memsize;
    logger << "LoadPlan     \t" << countloadplans << "\t" << memsize << endl;

    // Problems
    memsize = count = 0;
    for (Problem::const_iterator pr = Problem::begin(); pr!=Problem::end(); ++pr)
    {
      ++count;
      memsize += pr->getSize();
    }
    total += memsize;
    logger << "Problem      \t" << count << "\t" << memsize << endl;

    // TOTAL
    logger << "Total        \t\t" << total << endl << endl;
  }
  catch (...)
  {
    Py_BLOCK_THREADS;
    PythonType::evalException();
    return NULL;
  }
  Py_END_ALLOW_THREADS   // Reclaim Python interpreter
  return Py_BuildValue("");
}

} // end namespace
