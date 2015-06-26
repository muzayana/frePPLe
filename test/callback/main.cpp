/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2013 by frePPLe bvba                                 *
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


class SignalSniffer
{
  public:
    static bool callback(Buffer* l, const Signal a)
    {
      logger << "  Buffer '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(BufferInfinite* l, const Signal a)
    {
      logger << "  BufferInfinite '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(BufferDefault* l, const Signal a)
    {
      logger << "  BufferDefault '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(Operation* l, const Signal a)
    {
      logger << "  Operation '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(OperationFixedTime* l, const Signal a)
    {
      logger << "  OperationFixedTime '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(Item* l, const Signal a)
    {
      logger << "  Item '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(Flow* l, const Signal a)
    {
      logger << "  Flow between '" << l->getBuffer() << "' and '"
          << l->getOperation() << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(Demand* l, const Signal a)
    {
      logger << "  Demand '" << l << "' receives signal " << a << endl;
      return true;
    }
    static bool callback(Calendar* l, const Signal a)
    {
      logger << "  Calendar '" << l << "' receives signal " << a << endl;
      return true;
    }
};


int main (int argc, char *argv[])
{
  try
  {
    // 0: Initialize
    FreppleInitialize();

    // 1: Create subscriptions
    // a) buffers
    FunctorStatic<Buffer,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<Buffer,SignalSniffer>::connect(SIG_REMOVE);
    FunctorStatic<BufferDefault,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<BufferDefault,SignalSniffer>::connect(SIG_REMOVE);
    FunctorStatic<BufferInfinite,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<BufferInfinite,SignalSniffer>::connect(SIG_REMOVE);

    // b) operations
    FunctorStatic<Operation,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<Operation,SignalSniffer>::connect(SIG_REMOVE);
    FunctorStatic<OperationFixedTime,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<OperationFixedTime,SignalSniffer>::connect(SIG_REMOVE);

    // c) items
    FunctorStatic<Item,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<Item,SignalSniffer>::connect(SIG_REMOVE);

    // d) flows
    FunctorStatic<Flow,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<Flow,SignalSniffer>::connect(SIG_REMOVE);

    // e) demands
    FunctorStatic<Demand,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<Demand,SignalSniffer>::connect(SIG_REMOVE);

    // f) calendars
    FunctorStatic<Calendar,SignalSniffer>::connect(SIG_ADD);
    FunctorStatic<Calendar,SignalSniffer>::connect(SIG_REMOVE);

    // 2: Read and the model
    logger << "Create the model with callbacks:" << endl;
    FreppleReadXMLFile("callback.xml",true,false);

    // 3: Plan erase the model
    logger << "Plan the model:" << endl;
    utils::PythonInterpreter::execute("frepple.solver_mrp(name=\"MRP\", constraints=0).solve()");

    // 4: Plan erase the model
    logger << "Erase the model:" << endl;
    utils::PythonInterpreter::execute("frepple.erase(True)");

    // 5: Remove the subscriptions
    // a) buffers
    FunctorStatic<Buffer,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<Buffer,SignalSniffer>::disconnect(SIG_REMOVE);
    FunctorStatic<BufferDefault,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<BufferDefault,SignalSniffer>::disconnect(SIG_REMOVE);
    FunctorStatic<BufferInfinite,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<BufferInfinite,SignalSniffer>::disconnect(SIG_REMOVE);

    // b) operations
    FunctorStatic<Operation,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<Operation,SignalSniffer>::disconnect(SIG_REMOVE);
    FunctorStatic<OperationFixedTime,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<OperationFixedTime,SignalSniffer>::disconnect(SIG_REMOVE);

    // c) items
    FunctorStatic<Item,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<Item,SignalSniffer>::disconnect(SIG_REMOVE);

    // d) flows
    FunctorStatic<Flow,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<Flow,SignalSniffer>::disconnect(SIG_REMOVE);

    // e) demands
    FunctorStatic<Demand,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<Demand,SignalSniffer>::disconnect(SIG_REMOVE);

    // f) calendars
    FunctorStatic<Calendar,SignalSniffer>::disconnect(SIG_ADD);
    FunctorStatic<Calendar,SignalSniffer>::disconnect(SIG_REMOVE);

    // 6: Reread the model
    logger << "Recreate the model without callbacks:" << endl;
    FreppleReadXMLFile("callback.xml",true,false);
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
