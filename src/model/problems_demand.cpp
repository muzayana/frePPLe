/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2015 by frePPLe bvba                                 *
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


DECLARE_EXPORT void Demand::updateProblems()
{
  // The relation between the demand and the related problem classes is such
  // that the demand object is the only active one. The problem objects are
  // fully controlled and managed by the associated demand object.

  // A flag for each problem type that may need to be created
  bool needsNotPlanned(false);
  bool needsEarly(false);
  bool needsLate(false);
  bool needsShort(false);
  bool needsExcess(false);

  // Problem detection disabled on this demand
  if (!getDetectProblems()) return;

  // Check which problems need to be created
  if (deli.empty())
  {
    // Check if a new ProblemDemandNotPlanned needs to be created
    if (getQuantity()>0.0) needsNotPlanned = true;
  }
  else
  {
    // Loop through the deliveries
    for (OperationPlan_list::iterator i = deli.begin(); i!=deli.end(); ++i)
    {
      // Check for ProblemLate problem
      long d(getDue() - (*i)->getDates().getEnd());
      if (d < 0L) needsLate = true;
      // Check for ProblemEarly problem
      else if (d > 0L) needsEarly = true;
    }

    // Check for ProblemShort problem
    double plannedqty = getPlannedQuantity();
    if (plannedqty + ROUNDING_ERROR < qty) needsShort = true;

    // Check for ProblemExcess Problem
    if (plannedqty - ROUNDING_ERROR > qty) needsExcess = true;
  }

  // Loop through the existing problems
  for (Problem::iterator j = Problem::begin(this, false);
      j!=Problem::end(); )
  {
    // Need to increment now and define a pointer to the problem, since the
    // problem can be deleted soon (which invalidates the iterator).
    Problem& curprob = *j;
    ++j;
    // The if-statement keeps the problem detection code concise and
    // concentrated. However, a drawback of this design is that a new Problem
    // subclass will also require a new Demand subclass. I think such a link
    // is acceptable.
    if (typeid(curprob) == typeid(ProblemEarly))
    {
      // if: problem needed and it exists already
      if (needsEarly) needsEarly = false;
      // else: problem not needed but it exists already
      else delete &curprob;
    }
    else if (typeid(curprob) == typeid(ProblemDemandNotPlanned))
    {
      if (needsNotPlanned) needsNotPlanned = false;
      else delete &curprob;
    }
    else if (typeid(curprob) == typeid(ProblemLate))
    {
      if (needsLate) needsLate = false;
      else delete &curprob;
    }
    else if (typeid(curprob) == typeid(ProblemShort))
    {
      if (needsShort) needsShort = false;
      else delete &curprob;
    }
    else if (typeid(curprob) == typeid(ProblemExcess))
    {
      if (needsExcess) needsExcess = false;
      else delete &curprob;
    }
    // Note that there may be other demand exceptions that are not caught in
    // this loop. These are problems defined and managed by subclasses.
  }

  // Create the problems that are required but aren't existing yet.
  if (needsNotPlanned) new ProblemDemandNotPlanned(this);
  if (needsLate) new ProblemLate(this);
  if (needsEarly) new ProblemEarly(this);
  if (needsShort) new ProblemShort(this);
  if (needsExcess) new ProblemExcess(this);
}


DECLARE_EXPORT string ProblemLate::getDescription() const
{
  assert(getDemand() && !getDemand()->getDelivery().empty());
  Duration t(getDemand()->getLatestDelivery()->getDates().getEnd()
      - getDemand()->getDue());
  return string("Demand '") + getDemand()->getName() + "' planned "
      + string(t) + " after its due date";
}


DECLARE_EXPORT string ProblemEarly::getDescription() const
{
  assert(getDemand() && !getDemand()->getDelivery().empty());
  Duration t(getDemand()->getDue()
      - getDemand()->getEarliestDelivery()->getDates().getEnd());
  return string("Demand '") + getDemand()->getName() + "' planned "
      + string(t) + " before its due date";
}

}
