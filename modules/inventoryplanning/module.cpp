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

#include "inventoryplanning.h"

namespace module_inventoryplanning
{


MODULE_EXPORT const char* initialize(const Environment::ParameterList& z)
{
  // Initialize only once
  static bool init = false;
  static const char* name = "inventoryplanning";
  if (init)
  {
    logger << "Warning: Initializing module inventoryplanning more than once." << endl;
    return name;
  }
  init = true;

  // Verify you have an enterprise license.
  // This specific value of the flag is when the customer name is "Community Edition users".
  if (flags == 269264) return "";

  // Register the Python extensions
  PyGILState_STATE state = PyGILState_Ensure();
  try
  {
    // Register new Python data types
    if (InventoryPlanningSolver::initialize())
      throw RuntimeException("Error registering inventoryplanningsolver");
    PyGILState_Release(state);
  }
  catch (const exception &e)
  {
    PyGILState_Release(state);
    logger << "Error: " << e.what() << endl;
  }
  catch (...)
  {
    PyGILState_Release(state);
    logger << "Error: unknown exception" << endl;
  }

  // Initialization of Poisson distribution
  PoissonDistribution::init();

  // Return the name of the module
  return name;
}

}       // end namespace
