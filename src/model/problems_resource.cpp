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


DECLARE_EXPORT void Resource::updateProblems()
{
  // Delete existing problems for this resource
  Problem::clearProblems(*this);

  // Problem detection disabled on this resource
  if (!getDetectProblems()) return;

  // Loop through the loadplans
  Date excessProblemStart;
  Date shortageProblemStart;
  bool excessProblem = false;
  bool shortageProblem = false;
  double curMax(0.0);
  double shortageQty(0.0);
  double curMin(0.0);
  double excessQty(0.0);
  for (loadplanlist::const_iterator iter = loadplans.begin();
      iter != loadplans.end(); )
  {
    // Process changes in the maximum or minimum targets
    if (iter->getType() == 4)
      curMax = iter->getMax();
    else if (iter->getType() == 3)
      curMin = iter->getMin();

    // Only consider the last loadplan for a certain date
    const TimeLine<LoadPlan>::Event *f = &*(iter++);
    if (iter!=loadplans.end() && iter->getDate()==f->getDate()) continue;

    // Check against minimum target
    double delta = f->getOnhand() - curMin;
    if (delta < -ROUNDING_ERROR)
    {
      if (!shortageProblem)
      {
        shortageProblemStart = f->getDate();
        shortageQty = delta;
        shortageProblem = true;
      }
      else if (delta < shortageQty)
        // New shortage qty
        shortageQty = delta;
    }
    else
    {
      if (shortageProblem)
      {
        // New problem now ends
        if (f->getDate() != shortageProblemStart)
          new ProblemCapacityUnderload(this, DateRange(shortageProblemStart,
              f->getDate()), -shortageQty);
        shortageProblem = false;
      }
    }

    // Note that theoretically we can have a minimum and a maximum problem for
    // the same moment in time.

    // Check against maximum target
    delta = f->getOnhand() - curMax;
    if (delta > ROUNDING_ERROR)
    {
      if (!excessProblem)
      {
        excessProblemStart = f->getDate();
        excessQty = delta;
        excessProblem = true;
      }
      else if (delta > excessQty)
        excessQty = delta;
    }
    else
    {
      if (excessProblem)
      {
        // New problem now ends
        if (f->getDate() != excessProblemStart)
          new ProblemCapacityOverload(this, excessProblemStart,
              f->getDate(), excessQty);
        excessProblem = false;
      }
    }

  }  // End of for-loop through the loadplans

  // The excess lasts till the end of the horizon...
  if (excessProblem)
    new ProblemCapacityOverload(this, excessProblemStart,
        Date::infiniteFuture, excessQty);

  // The shortage lasts till the end of the horizon...
  if (shortageProblem)
    new ProblemCapacityUnderload(this, DateRange(shortageProblemStart,
        Date::infiniteFuture), -shortageQty);
}


DECLARE_EXPORT void ResourceBuckets::updateProblems()
{
  // Delete existing problems for this resource
  Problem::clearProblems(*this);

  // Problem detection disabled on this resource
  if (!getDetectProblems()) return;

  // Loop over all events
  Date startdate = Date::infinitePast;
  double capa = 0.0;
  double load = 0.0;
  for (loadplanlist::const_iterator iter = loadplans.begin();
      iter != loadplans.end(); iter++)
  {
    if (iter->getType() != 2)
      load = iter->getOnhand();
    else
    {
      // Evaluate previous bucket
      if (load < 0.0)
        new ProblemCapacityOverload(this, startdate,
          iter->getDate(), -load);
      // Reset evaluation for the new bucket
      capa = iter->getOnhand();
      startdate = iter->getDate();
      load = 0.0;
    }
  }
  // Evaluate the final bucket
  if (load < 0.0)
    new ProblemCapacityOverload(this, startdate,
      Date::infiniteFuture, -load);
}


DECLARE_EXPORT string ProblemCapacityUnderload::getDescription() const
{
  ostringstream ch;
  ch << "Resource '" << getResource() << "' has excess capacity of " << qty;
  return ch.str();
}


DECLARE_EXPORT string ProblemCapacityOverload::getDescription() const
{
  ostringstream ch;
  ch << "Resource '" << getResource() << "' has capacity shortage of " << qty;
  return ch.str();
}

}
