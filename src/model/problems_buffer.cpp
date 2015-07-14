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


DECLARE_EXPORT void Buffer::updateProblems()
{
  // Delete existing problems for this buffer
  Problem::clearProblems(*this);

  // Problem detection disabled on this buffer
  if (!getDetectProblems()) return;

  // Loop through the flowplans
  Date excessProblemStart;
  Date shortageProblemStart;
  bool shortageProblem = false;
  bool excessProblem = false;
  double curMax(0.0);
  double shortageQty(0.0);
  double curMin(0.0);
  double excessQty(0.0);
  for (flowplanlist::const_iterator iter = flowplans.begin();
      iter != flowplans.end(); )
  {
    // Process changes in the maximum or minimum targets
    if (iter->getEventType() == 4)
      curMax = iter->getMax();
    else if (iter->getEventType() == 3)
      curMin = iter->getMin();

    // Only consider the last flowplan for a certain date
    const TimeLine<FlowPlan>::Event *f = &*(iter++);
    if (iter!=flowplans.end() && iter->getDate()==f->getDate()) continue;

    // Check against minimum target
    double delta = f->getOnhand() - curMin;
    if (delta < -ROUNDING_ERROR)
    {
      if (!shortageProblem)
      {
        // Start of a problem
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
          new ProblemMaterialShortage
          (this, shortageProblemStart, f->getDate(), -shortageQty);
        shortageProblem = false;
      }
    }

    // Check against maximum target
    delta = f->getOnhand() - (curMin<curMax ? curMax : curMin);
    if (delta > ROUNDING_ERROR)
    {
      if (!excessProblem)
      {
        // New problem starts here
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
        // New excess qty
        // New problem now ends
        if (f->getDate() != excessProblemStart)
          new ProblemMaterialExcess
          (this, excessProblemStart, f->getDate(), excessQty);
        excessProblem = false;
      }
    }

  }  // End of for-loop through the flowplans

  // The excess lasts till the end of the horizon...
  if (excessProblem)
    new ProblemMaterialExcess
    (this, excessProblemStart, Date::infiniteFuture, excessQty);

  // The shortage lasts till the end of the horizon...
  if (shortageProblem)
    new ProblemMaterialShortage
    (this, shortageProblemStart, Date::infiniteFuture, -shortageQty);
}



DECLARE_EXPORT string ProblemMaterialExcess::getDescription() const
{
  ostringstream ch;
  ch << "Buffer '" << getBuffer() << "' has material excess of " << qty;
  return ch.str();
}


DECLARE_EXPORT string ProblemMaterialShortage::getDescription() const
{
  ostringstream ch;
  ch << "Buffer '" << getBuffer() << "' has material shortage of " << qty;
  return ch.str();
}


}
