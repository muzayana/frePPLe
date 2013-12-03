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
#include "frepple/solver.h"

namespace frepple
{

bool sortFlow(const Flow* lhs, const Flow* rhs)
{
  return lhs->getPriority() < rhs->getPriority();
}


DECLARE_EXPORT void SolverMRP::solve(const Flow* fl, void* v)  // @todo implement search mode
{
  // Note: This method is only called for consuming flows and for the leading
  // flow of an alternate group. See SolverMRP::checkOperation

  SolverMRPdata* data = static_cast<SolverMRPdata*>(v);
  if (fl->hasAlternates())
  {
    // CASE I: It is an alternate flow.
    // We ask each alternate flow in order of priority till we find a flow
    // that has a non-zero reply.

    // 1) collect a list of alternates
    list<const Flow*> thealternates;
    const Flow *x = fl->hasAlternates() ? fl : fl->getAlternate();
    for (Operation::flowlist::const_iterator i = fl->getOperation()->getFlows().begin();
        i != fl->getOperation()->getFlows().end(); ++i)
      if ((i->getAlternate() == x || &*i == x)
          && i->getEffective().within(data->state->q_flowplan->getDate()))
        thealternates.push_front(&*i);

    // 2) Sort the list
    thealternates.sort(sortFlow);

    // 3) Control the planning mode
    bool originalPlanningMode = data->constrainedPlanning;
    data->constrainedPlanning = true;
    const Flow *firstAlternate = NULL;
    double firstQuantity = 0.0;

    // Remember the top constraint
    bool originalLogConstraints = data->logConstraints;
    //Problem* topConstraint = data->planningDemand->getConstraints().top();

    // 4) Loop through the alternates till we find a non-zero reply
    Date min_next_date(Date::infiniteFuture);
    double ask_qty;
    FlowPlan *flplan = data->state->q_flowplan;
    for (list<const Flow*>::const_iterator i = thealternates.begin();
        i != thealternates.end();)
    {
      const Flow *curflow = *i;
      data->state->q_flowplan = flplan; // because q_flowplan can change

      // 4a) Switch to this flow
      if (data->state->q_flowplan->getFlow() != curflow)
        data->state->q_flowplan->setFlow(curflow);

      // 4b) Call the Python user exit if there is one
      if (userexit_flow)
      {
        PythonObject result = userexit_flow.call(data->state->q_flowplan, PythonObject(data->constrainedPlanning));
        if (!result.getBool())
        {
          // Return value is false, alternate rejected
          if (data->getSolver()->getLogLevel()>1)
            logger << indent(curflow->getOperation()->getLevel())
                << "   User exit disallows consumption from '"
                << (*i)->getBuffer()->getName() << "'" << endl;
          // Move to the next alternate
          if (++i != thealternates.end() && data->getSolver()->getLogLevel()>1)
            logger << indent(curflow->getOperation()->getLevel()) << "   Alternate flow switches from '"
                << curflow->getBuffer()->getName() << "' to '"
                << (*i)->getBuffer()->getName() << "'" << endl;
          continue;
        }
      }

      // Remember the first alternate
      if (!firstAlternate)
      {
        firstAlternate = *i;
        firstQuantity = data->state->q_flowplan->getQuantity();
      }

      // Constraint tracking
      if (*i != firstAlternate)
        // Only enabled on first alternate
        data->logConstraints = false;
      else
        // Keep track of constraints, if enabled
        data->logConstraints = originalLogConstraints;

      // 4c) Ask the buffer
      data->state->q_qty = ask_qty = - data->state->q_flowplan->getQuantity();
      data->state->q_date = data->state->q_flowplan->getDate();
      CommandManager::Bookmark* topcommand = data->setBookmark();
      curflow->getBuffer()->solve(*this,data);

      // 4d) A positive reply: exit the loop
      if (data->state->a_qty > ROUNDING_ERROR)
      {
        // Update the opplan, which is required to (1) update the flowplans
        // and to (2) take care of lot sizing constraints of this operation.
        if (data->state->a_qty < ask_qty - ROUNDING_ERROR)
        {
          flplan->setQuantity(-data->state->a_qty, true);
          data->state->a_qty = -flplan->getQuantity();
        }
        if (data->state->a_qty > ROUNDING_ERROR)
        {
          data->constrainedPlanning = originalPlanningMode;
          data->logConstraints = originalLogConstraints;
          return;
        }
      }

      // 4e) Undo the plan on the alternate
      data->rollback(topcommand);

      // 4f) Prepare for the next alternate
      if (data->state->a_date < min_next_date)
        min_next_date = data->state->a_date;
      if (++i != thealternates.end() && data->getSolver()->getLogLevel()>1)
        logger << indent(curflow->getOperation()->getLevel()) << "   Alternate flow switches from '"
            << curflow->getBuffer()->getName() << "' to '"
            << (*i)->getBuffer()->getName() << "'" << endl;
    }

    // 5) No reply found, all alternates are infeasible
    if (!originalPlanningMode)
    {
      assert(firstAlternate);
      // Unconstrained plan: Plan on the primary alternate
      // Switch to this flow
      if (flplan->getFlow() != firstAlternate)
        flplan->setFlow(firstAlternate);
      // Message
      if (data->getSolver()->getLogLevel()>1)
        logger << indent(fl->getOperation()->getLevel())
            << "   Alternate flow plans unconstrained on alternate '"
            << firstAlternate->getBuffer()->getName() << "'" << endl;
      // Plan unconstrained
      data->constrainedPlanning = false;
      data->state->q_flowplan = flplan; // because q_flowplan can change
      flplan->setQuantity(firstQuantity, true);
      data->state->q_qty = ask_qty = - flplan->getQuantity();
      data->state->q_date = flplan->getDate();
      firstAlternate->getBuffer()->solve(*this,data);
      data->state->a_qty = -flplan->getQuantity();
      // Restore original planning mode
      data->constrainedPlanning = originalPlanningMode;
    }
    else
    {
      // Constrained plan: Return 0
      data->state->a_date = min_next_date;
      data->state->a_qty = 0;
      if (data->getSolver()->getLogLevel()>1)
        logger << indent(fl->getOperation()->getLevel()) <<
            "   Alternate flow doesn't find supply on any alternate : "
            << data->state->a_qty << "  " << data->state->a_date << endl;
    }
  }
  else
  {
    // CASE II: Not an alternate flow.
    // In this case, this method is passing control on to the buffer.
    data->state->q_qty = - data->state->q_flowplan->getQuantity();
    data->state->q_date = data->state->q_flowplan->getDate();
    if (data->state->q_qty != 0.0)
    {
      fl->getBuffer()->solve(*this,data);
      if (data->state->a_date > fl->getEffective().getEnd())
      {
        // The reply date must be less than the effectivity end date: after
        // that date the flow in question won't consume any material any more.
        if (data->getSolver()->getLogLevel()>1
            && data->state->a_qty < ROUNDING_ERROR)
          logger << indent(fl->getBuffer()->getLevel()) << "  Buffer '"
              << fl->getBuffer()->getName() << "' answer date is adjusted to "
              << fl->getEffective().getEnd()
              << " because of a date effective flow" << endl;
        data->state->a_date = fl->getEffective().getEnd();
      }
    }
    else
    {
      // It's a zero quantity flowplan.
      // E.g. because it is not effective.
      data->state->a_date = data->state->q_date;
      data->state->a_qty = 0.0;
    }
  }
}


}
