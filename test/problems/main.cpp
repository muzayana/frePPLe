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


#include "frepple.h"
#include "freppleinterface.h"
using namespace frepple;


void reportProblems(string when)
{
  logger << "Problems after " << when << ":" << endl;
  for (Problem::const_iterator i = Problem::begin(); i != Problem::end(); ++i)
    logger << "   " << i->getDates() << " - " << i->getDescription() << endl;
  logger << endl;
}


int main (int argc, char *argv[])
{
  try
  {
    // 0: Initialize
    FreppleInitialize(0,NULL);

    // 1: Read the model
    FreppleReadXMLFile("problems.xml",true,false);
    reportProblems("reading input");

    // 2: Plan the model
    FreppleReadXMLData(
      "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" \
      "<plan xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n" \
      "<?python\n" \
      "frepple.solver_mrp(name=\"MRP\", constraints=0).solve()\n"  \
      "?>\n" \
      "</plan>", true, false
    );
    reportProblems("planning");

    // Define variables for each of the 2 operation_plans
    Operation *buildoper = Operation::find("make end item");
    OperationPlan *build = &*OperationPlan::iterator(buildoper);
    Operation *deliveroper = Operation::find("delivery end item");
    OperationPlan *deliver = &*OperationPlan::iterator(deliveroper);
    if (!deliver || !build) throw DataException("Can't find operationplans");

    // 3: Increase quantity of the delivery & report
    float oldqty = deliver->getQuantity();
    deliver->setQuantity(100);
    reportProblems("increasing delivery quantity");

    // 4: Reduce the quantity of the delivey & report
    deliver->setQuantity(1);
    reportProblems("decreasing delivery quantity");

    // 5: Move the delivery early & report
    Date oldstart = deliver->getDates().getStart();
    deliver->setStart(oldstart - TimePeriod(86400));
    reportProblems("moving delivery early");

    // 6: Move the delivery late & report
    deliver->setStart(oldstart + TimePeriod(86400));
    reportProblems("moving delivery late");

    // 7: Restoring original delivery plan & report
    deliver->setQuantity(oldqty);
    deliver->setStart(oldstart);
    reportProblems("restoring original delivery plan");

    // 8: Deleting delivery
    delete deliver;
    reportProblems("deleting delivery plan");

    // 9: Move the make operation before current & report
    oldstart = build->getDates().getStart();
    build->setStart(Plan::instance().getCurrent() - TimePeriod(1));
    reportProblems("moving build early");

    // 10: Restoring the original build plan & report
    build->setStart(oldstart);
    reportProblems("restoring original build plan");
  }
  catch (...)
  {
    logger << "Error: Caught an exception in main routine:" <<  endl;
    try { throw; }
    catch (const exception& e) {logger << "  " << e.what() << endl;}
    catch (...) {logger << "  Unknown type" << endl;}
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
