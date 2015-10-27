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

#ifndef SOLVER_H
#define SOLVER_H

#include "frepple/model.h"
#ifndef DOXYGEN
#include <deque>
#include <cmath>
#endif

namespace frepple
{


/** @brief A lightweight solver class to remove excess material.
  *
  * The class works in a single thread only.
  */
class OperatorDelete : public Solver
{
  public:
	/** Constructor. */
    DECLARE_EXPORT OperatorDelete(const string& n, CommandManager* c = NULL) :
        Solver(n), cmds(c) { initType(metadata); }

    /** Destructor. */
    virtual DECLARE_EXPORT ~OperatorDelete() {}

    /** Python method for running the solver. */
    static PyObject* solve(PyObject*, PyObject*);

    /** Remove all entities for excess material that can be removed. */
    void solve(void *v = NULL);

    /** Remove an operationplan and all its upstream supply.<br>
      * The argument operationplan is invalid when this function returns!
      */
    void solve(OperationPlan*, void* = NULL);

    /** Remove excess from a buffer and all its upstream colleagues. */
    void solve(const Buffer*, void* = NULL);

    /** Remove excess starting from a single demand. */
    void solve(const Demand*, void* = NULL);

    /** Remove excess operations on a resource. */
    void solve(const Resource*, void* = NULL);

    static int initialize();
    virtual const MetaClass& getType() const {return *metadata;}
    static const MetaClass* metadata;
    virtual size_t getSize() const {return sizeof(OperatorDelete);}

  private:
    /** Auxilary function to push consuming or producing buffers of an
      * operationplan to the stack.<br>
      * When the argument is true, we push the consumers.
      * When the argument is false, we push the producers.
      */
	  void pushBuffers(OperationPlan*, bool);

	  /** A list of buffers still to scan for excess. */
	  vector<Buffer*> buffersToScan;   // TODO Use a different data structure to allow faster lookups and sorting?

	  /** A pointer to a command manager that takes care of the commit and
	    * rollback of all actions.
	    */
	  CommandManager* cmds;
};


/** @brief This solver implements a heuristic algorithm for planning demands.
  *
  * One by one the demands are processed. The demand will consume step by step
  * any upstream materials, respecting all constraints on its path.<br>
  * The solver supports all planning constraints as defined in Solver
  * class.<br>
  * See the documentation of the different solve methods to understand the
  * functionality in more detail.
  *
  * The logging levels have the following meaning:
  * - 0: Silent operation. Default logging level.
  * - 1: Show solver progress for each demand.
  * - 2: Show the complete ask&reply communication of the solver.
  * - 3: Trace the status of all entities.
  */
class SolverMRP : public Solver
{
  protected:
    /** This variable stores the constraint which the solver should respect.
      * By default no constraints are enabled. */
    short constrts;

    bool allowSplits;

    bool rotateResources;

    /** Behavior of this solver method is:
      *  - It will ask the consuming flows for the required quantity.
      *  - The quantity asked for takes into account the quantity_per of the
      *    producing flow.
      *  - The date asked for takes into account the post-operation time
      *    of the operation.
      */
    DECLARE_EXPORT void solve(const Operation*, void* = NULL);

    /** Behavior of this solver method is:
      *  - Asks each of the routing steps for the requested quantity, starting
      *    with the last routing step.<br>
      *    The time requested for the operation is based on the start date of
      *    the next routing step.
      */
    DECLARE_EXPORT void solve(const OperationRouting*, void* = NULL);

    /** Behavior of this solver method is:
      *  - The solver asks each alternate for the percentage of the requested
      *    quantity. We ask the operation with the highest percentage first,
      *    and only ask suboperations that are effective on the requested date.
      *  - The percentages don't need to add up to 100. We scale the proportiona
      *  - If an alternate replies more than requested (due to multiple and
      *    minimum size) this is considered when dividing the remaining
      *    quantity over the others.
      *  - If an alternate can't deliver the requested percentage of the
      *    quantity, we undo all previous alternates and retry planning
      *    for a rescaled total quantity.
      *    The split percentage is thus a hard constraint that must be
      *    respected - a constraint on a single alternate also constrains the
      *    planned quantity on all others.
      *    Obviously if an alternate replies 0 the total rescaled quantity
      *    remains 0.
      *  - A case not handled with this logic is when the split operations
      *    merge again upstream. If a shared upstream constraint is limiting
      *    the total quantity, the solver doesn't see this and can't react
      *    nicely to it. The solution would be that we a) detect this kind
      *    of situation and b) iteratively try to split an increasing total
      *    quantity. TODO...
      *  - For each effective alternate suboperation we create 1
      *    suboperationplan of the top operationplan.
      */
    DECLARE_EXPORT void solve(const OperationSplit*,void* = NULL);

    /** Behavior of this solver method is:
      *  - The solver loops through each alternate operation in order of
      *    priority. On each alternate operation, the solver will try to plan
      *    the quantity that hasn't been planned on higher priority alternates.
      *  - As a special case, operations with zero priority are skipped in the
      *    loop. These operations are considered to be temporarily unavailable.
      *  - The requested operation can be planned over multiple alternates.
      *    We don't garantuee that a request is planned using a single alternate
      *    operation.
      *  - The solver properly considers the quantity_per of all flows producing
      *    into the requested buffer, if such a buffer is specified.
      */
    DECLARE_EXPORT void solve(const OperationAlternate*,void* = NULL);

    /** Behavior of this solver method:
      *  - No propagation to upstream buffers at all, even if a producing
      *    operation has been specified.
      *  - Always give an answer for the full quantity on the requested date.
      */
    DECLARE_EXPORT void solve(const BufferInfinite*,void* = NULL);

    /** Behavior of this solver method:
      *  - Consider 0 as the hard minimum limit. It is not possible
      *    to plan with a 'hard' safety stock reservation.
      *  - Minimum inventory is treated as a 'wish' inventory. When replenishing
      *    a buffer we try to satisfy the minimum target. If that turns out
      *    not to be possible we use whatever available supply for satisfying
      *    the demand first.
      *  - Planning for the minimum target is part of planning a demand. There
      *    is no planning run independent of demand to satisfy the minimum
      *    target.<br>
      *    E.g. If a buffer has no demand on it, the solver won't try to
      *    replenish to the minimum target.<br>
      *    E.g. If the minimum target increases after the latest date required
      *    for satisfying a certain demand that change will not be considered.
      *  - The solver completely ignores the maximum target.
      */
    DECLARE_EXPORT void solve(const Buffer*, void* = NULL);

    /** Called by the previous method to solve for safety stock only. */
    DECLARE_EXPORT void solveSafetyStock(const Buffer*, void* = NULL);

    /** Behavior of this solver method:
      *  - When the inventory drops below the minimum inventory level, a new
      *    replenishment is triggered.
      *    The replenishment brings the inventory to the maximum level again.
      *  - The minimum and maximum inventory are soft-constraints. The actual
      *    inventory can go lower than the minimum or exceed the maximum.
      *  - The minimum, maximum and multiple size of the replenishment are
      *    hard constraints, and will always be respected.
      *  - A minimum and maximum interval between replenishment is also
      *    respected as a hard constraint.
      *  - No propagation to upstream buffers at all, even if a producing
      *    operation has been specified.
      *  - The minimum calendar isn't used by the solver.
      *
      * @todo Optimize the solver method as follows for the common case of infinite
      * buying capability (ie no max quantity + min time):
      *  - beyond lead time: always reply OK, without rearranging the operation plans
      *  - at the end of the solver loop, we revisit the procurement buffers to establish
      *    the final purchasing profile
      */
    DECLARE_EXPORT void solve(const BufferProcure*, void* = NULL);

    /** Behavior of this solver method:
      *  - This method simply passes on the request to the referenced buffer.
      *    It is called from a solve(Operation*) method and passes on the
      *    control to a solve(Buffer*) method.
      * @see checkOperationMaterial
      */
    DECLARE_EXPORT void solve(const Flow*, void* = NULL);

    /** Behavior of this solver method:
      *  - The operationplan is checked for a capacity overload. When detected
      *    it is moved to an earlier date.
      *  - This move can be repeated until no capacity is found till a suitable
      *    time slot is found. If the fence and/or leadtime constraints are
      *    enabled they can restrict the feasible moving time.<br>
      *    If a feasible timeslot is found, the method exits here.
      *  - If no suitable time slot can be found at all, the operation plan is
      *    put on its original date and we now try to move it to a feasible
      *    later date. Again, successive moves are possible till a suitable
      *    slot is found or till we reach the end of the horizon.
      *    The result of the search is returned as the answer-date to the
      *    solver.
      */
    DECLARE_EXPORT void solve(const Resource*, void* = NULL);

    /** Behavior of this solver method:
      *  - Always return OK.
      */
    DECLARE_EXPORT void solve(const ResourceInfinite*,void* = NULL);

    /** Behavior of this solver method:
      *  - The operationplan is checked for a capacity in the time bucket
      *    where its start date falls.
      *  - If no capacity is found in that bucket, we check in the previous
      *    buckets (until we hit the limit defined by the maxearly field).
      *    We move the operationplan such that it starts one second before
      *    the end of the earlier bucket.
      *  - If no available time bucket is found in the allowed time fence,
      *    we scan for the first later bucket which still has capacity left.
      *    And we return the start date of that bucket as the answer-date to
      *    the solver.
      */
    DECLARE_EXPORT void solve(const ResourceBuckets*,void* = NULL);

    /** Behavior of this solver method:
      *  - This method simply passes on the request to the referenced resource.
      *    With the current model structure it could easily be avoided (and
      *    thus gain a bit in performance), but we wanted to include it anyway
      *    to make the solver as generic and future-proof as possible.
      * @see checkOperationCapacity
      */
    DECLARE_EXPORT void solve(const Load*, void* = NULL);

    /** Behavior of this solver method:
      *  - Respects the following demand planning policies:<br>
      *     1) Maximum allowed lateness
      *     2) Minimum shipment quantity
      * This method is normally called from within the main solve method, but
      * it can also be called independently to plan a certain demand.
      * @see solve
      */
    DECLARE_EXPORT void solve(const Demand*, void* = NULL);

    /** Choose a resource.<br>
      * Normally the chosen resource is simply the resource specified on the
      * load.<br>
      * When the load specifies a certain skill and an aggregate resource, then
      * we search for appropriate child resources.
      */
    DECLARE_EXPORT void chooseResource(const Load*, void*);

  public:
    /** This is the main solver method that will appropriately call the other
      * solve methods.<br>
      * The demands in the model will all be sorted with the criteria defined in
      * the demand_comparison() method. For each of demand the solve(Demand*)
      * method is called to plan it.
      */
    DECLARE_EXPORT void solve(void *v = NULL);

    /** Constructor. */
    DECLARE_EXPORT SolverMRP(const string& n) : Solver(n), constrts(15),
      allowSplits(true), rotateResources(true), plantype(1), lazydelay(86400L),
      iteration_threshold(1), iteration_accuracy(0.01), iteration_max(0),
      autocommit(true), planSafetyStockFirst(false)
    { initType(metadata); }

    /** Destructor. */
    virtual DECLARE_EXPORT ~SolverMRP() {}

    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const {return sizeof(SolverMRP);}

    /** Static constant for the LEADTIME constraint type.<br>
      * The numeric value is 1.
      * @see MATERIAL
      * @see CAPACITY
      * @see FENCE
      */
    static const short LEADTIME = 1;

    /** Static constant for the MATERIAL constraint type.<br>
      * The numeric value is 2.
      * @see LEADTIME
      * @see CAPACITY
      * @see FENCE
      */
    static const short MATERIAL = 2;

    /** Static constant for the CAPACITY constraint type.<br>
      * The numeric value is 4.
      * @see MATERIAL
      * @see LEADTIME
      * @see FENCE
      */
    static const short CAPACITY = 4;

    /** Static constant for the FENCE constraint type.<br>
      * The numeric value is 8.
      * @see MATERIAL
      * @see CAPACITY
      * @see LEADTIME
      */
    static const short FENCE = 8;

    /** Update the constraints to be considered by this solver. This field may
      * not be applicable for all solvers. */
    void setConstraints(short i) {constrts = i;}

    /** Returns the constraints considered by the solve. */
    short getConstraints() const {return constrts;}

    /** Returns true if this solver respects the operation release fences.
      * The solver isn't allowed to create any operation plans within the
      * release fence.
      */
    bool isFenceConstrained() const {return (constrts & FENCE)>0;}

    /** Returns true if the solver respects the current time of the plan.
      * The solver isn't allowed to create any operation plans in the past.
      */
    bool isLeadtimeConstrained() const {return (constrts & LEADTIME)>0;}

    /** Returns true if the solver respects the material procurement
      * constraints on procurement buffers.
      */
    bool isMaterialConstrained() const {return (constrts & MATERIAL)>0;}

    /** Returns true if the solver respects capacity constraints. */
    bool isCapacityConstrained() const {return (constrts & CAPACITY)>0;}

    /** Returns true if any constraint is relevant for the solver. */
    bool isConstrained() const {return constrts>0;}

    /** Returns the plan type:
      *  - 1: Constrained plan.<br>
      *       This plan doesn't not violate any constraints.<br>
      *       In case of material or capacity shortages the demand is delayed
      *       or planned short.
      *  - 2: Unconstrained plan with alternate search.<br>
      *       This unconstrained plan leaves material, capacity and operation
      *       problems when shortages are found. Availability is searched across
      *       alternates and the remaining shortage is shown on the primary
      *       alternate.<br>
      *       The demand is always fully met on time.
      *  - 3: Unconstrained plan without alternate search.<br>
      *       This unconstrained plan leaves material, capacity and operation
      *       problems when shortages are found. It doesn't evaluate availability
      *       on alternates.<br>
      *       The demand is always fully met on time.
      * The default is 1.
      */
    short getPlanType() const {return plantype;}

    void setPlanType(short b)
    {
      if (b < 1 || b > 3)
        throw DataException("Invalid plan type");
      plantype = b;
    }

    /** This function defines the order in which the demands are being
      * planned.<br>
      * The following sorting criteria are appplied in order:
      *  - demand priority: smaller priorities first
      *  - demand due date: earlier due dates first
      *  - demand quantity: smaller quantities first
      */
    static DECLARE_EXPORT bool demand_comparison(const Demand*, const Demand*);

    /** Return the time increment between requests when the answered reply
      * date isn't usable. */
    TimePeriod getLazyDelay() const {return lazydelay;}

    /** Update the time increment between requests when the answered reply
      * date isn't usable. */
    void setLazyDelay(TimePeriod l)
    {
      if (l <= 0L) throw DataException("Invalid lazy delay");
      lazydelay = l;
    }

    /** Get the threshold to stop iterating when the delta between iterations
      * is less than this absolute threshold.
      */
    double getIterationThreshold() const {return iteration_threshold;}

    /** Set the threshold to stop iterating when the delta between iterations
      * is less than this absolute threshold.<br>
      * The value must be greater than or equal to zero and the default is 1.
      */
    void setIterationThreshold(double d)
    {
      if (d<0.0)
        throw DataException("Invalid iteration threshold: must be >= 0");
      iteration_threshold = d;
    }

    /** Get the threshold to stop iterating when the delta between iterations
      * is less than this percentage threshold.
      */
    double getIterationAccuracy() const {return iteration_accuracy;}

    /** Set the threshold to stop iterating when the delta between iterations
      * is less than this percentage threshold.<br>
      * The value must be between 0 and 100 and the default is 1%.
      */
    void setIterationAccuracy(double d)
    {
      if (d<0.0 || d>100.0)
        throw DataException("Invalid iteration accuracy: must be >=0 and <= 100");
      iteration_accuracy = d;
    }

    /** Return the maximum number of asks allowed to plan a demand.
      * If the can't plan a demand within this limit, we consider it
      * unplannable.
      */
    unsigned long getIterationMax() const {return iteration_max;}

    /** Update the maximum number of asks allowed to plan a demand.
      * If the can't plan a demand within this limit, we consider it
      * unplannable.
      */
    void setIterationMax(unsigned long d)
    {
      iteration_max = d;
    }

    /** Return whether or not we automatically commit the changes after
      * planning a demand. */
    bool getAutocommit() const {return autocommit;}

    /** Update whether or not we automatically commit the changes after
      * planning a demand. */
    void setAutocommit(const bool b) {autocommit = b;}

    /** Specify a Python function that is called before solving a flow. */
    DECLARE_EXPORT void setUserExitFlow(const string& n) {userexit_flow = n;}

    /** Specify a Python function that is called before solving a flow. */
    DECLARE_EXPORT void setUserExitFlow(PyObject* p) {userexit_flow = p;}

    /** Return the Python function that is called before solving a flow. */
    PythonFunction getUserExitFlow() const {return userexit_flow;}

    /** Specify a Python function that is called before solving a demand. */
    DECLARE_EXPORT void setUserExitDemand(const string& n) {userexit_demand = n;}

    /** Specify a Python function that is called before solving a demand. */
    DECLARE_EXPORT void setUserExitDemand(PyObject* p) {userexit_demand = p;}

    /** Return the Python function that is called before solving a demand. */
    PythonFunction getUserExitDemand() const {return userexit_demand;}

    /** Specify a Python function that is called before solving a buffer. */
    DECLARE_EXPORT void setUserExitBuffer(const string& n) {userexit_buffer = n;}

    /** Specify a Python function that is called before solving a buffer. */
    DECLARE_EXPORT void setUserExitBuffer(PyObject* p) {userexit_buffer = p;}

    /** Return the Python function that is called before solving a buffer. */
    PythonFunction getUserExitBuffer() const {return userexit_buffer;}

    /** Specify a Python function that is called before solving a resource. */
    DECLARE_EXPORT void setUserExitResource(const string& n) {userexit_resource = n;}

    /** Specify a Python function that is called before solving a resource. */
    DECLARE_EXPORT void setUserExitResource(PyObject* p) {userexit_resource = p;}

    /** Return the Python function that is called before solving a resource. */
    PythonFunction getUserExitResource() const {return userexit_resource;}

    /** Specify a Python function that is called before solving a operation. */
    DECLARE_EXPORT void setUserExitOperation(const string& n) {userexit_operation = n;}

    /** Specify a Python function that is called before solving a operation. */
    DECLARE_EXPORT void setUserExitOperation(PyObject* p) {userexit_operation = p;}

    /** Return the Python function that is called before solving a operation. */
    PythonFunction getUserExitOperation() const {return userexit_operation;}

    /** Python method for running the solver. */
    static DECLARE_EXPORT PyObject* solve(PyObject*, PyObject*);

    /** Python method for commiting the plan changes. */
    static DECLARE_EXPORT PyObject* commit(PyObject*, PyObject*);

    /** Python method for undoing the plan changes. */
    static DECLARE_EXPORT PyObject* rollback(PyObject*, PyObject*);

    bool getRotateResources() const
    {
      return rotateResources;
    }

    void setRotateResources(bool b)
    {
      rotateResources = b;
    }

    bool getAllowSplits() const {return allowSplits;}
    void setAllowSplits(bool b) {allowSplits = b;}

    bool getPlanSafetyStockFirst() const {return planSafetyStockFirst;}

    void setPlanSafetyStockFirst(bool b) {planSafetyStockFirst = b;}

  private:
    typedef vector< deque<Demand*> > classified_demand;
    typedef classified_demand::iterator cluster_iterator;
    classified_demand demands_per_cluster;

    static const Keyword tag_rotateresources;

    /** Type of plan to be created. */
    short plantype;

    /** Time increments for a lazy replan.<br>
      * The solver is expected to return always a next-feasible date when the
      * request can't be met. The solver can then retry the request with an
      * updated request date. In some corner cases and in case of a bug it is
      * possible that no valid date is returned. The solver will then try the
      * request with a request date incremented by this value.<br>
      * The default value is 1 day.
      */
    TimePeriod lazydelay;

    /** Threshold to stop iterating when the delta between iterations is
      * less than this absolute limit.
      */
    double iteration_threshold;

    /** Threshold to stop iterating when the delta between iterations is
      * less than this percentage limit.
      */
    double iteration_accuracy;

    /** Maximum number of asks allowed to plan a demand.
      * If the can't plan a demand within this limit, we consider it
      * unplannable.
      */
    unsigned long iteration_max;

    /** Enable or disable automatically committing the changes in the plan
      * after planning each demand.<br>
      * The flag is only respected when planning incremental changes, and
      * is ignored when doing a complete replan.
      */
    bool autocommit;

    /** A Python callback function that is called for each alternate
      * flow. If the callback function returns false, that alternate
      * flow is an invalid choice.
      */
    PythonFunction userexit_flow;

    /** A Python callback function that is called for each demand. The return
      * value is not used.
      */
    PythonFunction userexit_demand;

    /** A Python callback function that is called for each buffer. The return
      * value is not used.
      */
    PythonFunction userexit_buffer;

    /** A Python callback function that is called for each resource. The return
      * value is not used.
      */
    PythonFunction userexit_resource;

    /** A Python callback function that is called for each operation. The return
      * value is not used.
      */
    PythonFunction userexit_operation;

    /** A flag that determines how we plan safety stock.<br/>
      *
      * By default the flag is FALSE and we get the following behavior:
      *  - When planning demands, we already try to replenish towards the
      *    safety stock level.
      *  - After planning all demands, we do another loop over all buffers
      *    to replenish to the safety stock level. This will replenish eg
      *    buffers without any (direct or indirect) demand on them.
      *
      * If the flag is set to TRUE, replenishing to the safety stock level
      * is more important than planning the demand on time. We get the
      * following behavior:
      *  - Before planning any demand, we try to replenish any buffer to its
      *    safety stock level.
      *    Buffers closer to the end item demand are replenished first.
      *  - When planning demands, we try to replenish towards the safety
      *    stock level.
      */
    bool planSafetyStockFirst;

  protected:
    /** @brief This class is used to store the solver status during the
      * ask-reply calls of the solver.
      */
    struct State
    {
      /** Points to the demand being planned.<br>
        * This field is only non-null when planning the delivery operation.
        */
      Demand* curDemand;

      /** Points to the current owner operationplan. This is used when
        * operations are nested. */
      OperationPlan* curOwnerOpplan;

      /** Points to the current buffer. */
      Buffer* curBuffer;

      /** A flag to force the resource solver to move the operationplan to
        * a later date where it is feasible.
        */
      bool forceLate;

      /** This is the quantity we are asking for. */
      double q_qty;

      /** This is the date we are asking for. */
      Date q_date;

      /** This is the maximum date we are asking for.<br>
        * In case of a post-operation time there is a difference between
        * q_date and q_date_max.
        */
      Date q_date_max;

      /** This is the quantity we can get by the requested Date. */
      double a_qty;

      /** This is the Date when we can get extra availability. */
      Date a_date;

      /** This is a pointer to a LoadPlan. It is used for communication
        * between the Operation-Solver and the Resource-Solver. */
      LoadPlan* q_loadplan;

      /** This is a pointer to a FlowPlan. It is used for communication
        * between the Operation-Solver and the Buffer-Solver. */
      FlowPlan* q_flowplan;

      /** A pointer to an operationplan currently being solved. */
      OperationPlan* q_operationplan;

      /** Cost of the reply.<br>
        * Only the direct cost should be returned in this field.
        */
      double a_cost;

      /** Penalty associated with the reply.<br>
        * This field contains indirect costs and other penalties that are
        * not strictly related to the request. Examples are setup costs,
        * inventory carrying costs, ...
        */
      double a_penalty;

      /** Motive of the current solver. */
      Plannable* motive;
    };

    /** @brief This class is a helper class of the SolverMRP class.
      *
      * It stores the solver state maintained by each solver thread.
      * @see SolverMRP
      */
    class SolverMRPdata : public CommandManager
    {
        friend class SolverMRP;
      public:
        static void runme(void *args)
        {
          SolverMRP::SolverMRPdata* x = static_cast<SolverMRP::SolverMRPdata*>(args);
          x->commit();
          delete x;
        }

        /** Return the solver. */
        SolverMRP* getSolver() const {return sol;}

        /** Constructor. */
        SolverMRPdata(SolverMRP* s = NULL, int c = 0, deque<Demand*>* d = NULL)
          : sol(s), cluster(c), demands(d),
            constrainedPlanning(true), state(statestack),
            prevstate(statestack-1)
        {
          ostringstream n;
          n << "delete operator for " << this;
          operator_delete = new OperatorDelete(n.str(), this);
        }

        /** Destructor. */
        virtual ~SolverMRPdata()
        {
          delete operator_delete;
        };

        /** Verbose mode is inherited from the solver. */
        unsigned short getLogLevel() const {return sol ? sol->getLogLevel() : 0;}

        /** This function runs a single planning thread. Such a thread will loop
          * through the following steps:
          *    - Use the method next_cluster() to find another unplanned cluster.
          *    - Exit the thread if no more cluster is found.
          *    - Sort all demands in the cluster, using the demand_comparison()
          *      method.
          *    - Loop through the sorted list of demands and plan each of them.
          *      During planning the demands exceptions are caught, and the
          *      planning loop will simply move on to the next demand.
          *      In this way, an error in a part of the model doesn't ruin the
          *      complete plan.
          * @see demand_comparison
          * @see next_cluster
          */
        virtual DECLARE_EXPORT void commit();

        virtual const MetaClass& getType() const {return *SolverMRP::metadata;}
        virtual size_t getSize() const {return sizeof(SolverMRPdata);}

        bool getVerbose() const
        {
          throw LogicException("Use the method SolverMRPdata::getLogLevel() instead of SolverMRPdata::getVerbose()");
        }

        /** Add a new state to the status stack. */
        inline void push(double q = 0.0, Date d = Date::infiniteFuture)
        {
          if (state >= statestack + MAXSTATES)
            throw RuntimeException("Maximum recursion depth exceeded");
          ++state;
          ++prevstate;
          state->q_qty = q;
          state->q_date = d;
          state->q_date_max = d;
          state->curOwnerOpplan = NULL;
          state->q_loadplan = NULL;
          state->q_flowplan = NULL;
          state->q_operationplan = NULL;
          state->curDemand = NULL;
          state->a_cost = 0.0;
          state->a_penalty = 0.0;
        }

        /** Removes a state from the status stack. */
        inline void pop()
        {
          if (--state < statestack)
            throw LogicException("State stack empty");
          --prevstate;
        }

      private:
        static const int MAXSTATES = 256;

        /** Auxilary method to replenish safety stock in all buffers of a
          * cluster. This method is only intended to be called from the
          * commit() method.
          * @see SolverMRP::planSafetyStockFirst
          * @see SolverMRP::SolverMRPdata::commit
          */
        void solveSafetyStock(SolverMRP*);

        /** Points to the solver. */
        SolverMRP* sol;

        /** An identifier of the cluster being replanned. Note that it isn't
          * always the complete cluster that is being planned.
          */
        unsigned int cluster;

        /** Internal solver to remove material. */
        OperatorDelete *operator_delete;

        /** A deque containing all demands to be (re-)planned. */
        deque<Demand*>* demands;

        /** Stack of solver status information. */
        State statestack[MAXSTATES];

        /** True when planning in constrained mode. */
        bool constrainedPlanning;

        /** Flags whether or not constraints are being tracked. */
        bool logConstraints;

        /** Points to the demand being planned. */
        Demand* planningDemand;

        /** Internal flag that is set to true when solving for safety stock. */
        bool safety_stock_planning;

        /** Count the number of asks. */
        unsigned long iteration_count;

      public:
        /** Pointer to the current solver status. */
        State* state;

        /** Pointer to the solver status one level higher on the stack. */
        State* prevstate;
    };

    /** When autocommit is switched off, this command structure will contain
      * all plan changes.
      */
    SolverMRPdata commands;

    /** This function will check all constraints for an operationplan
      * and propagate it upstream. The check does NOT check eventual
      * sub operationplans.<br>
      * The return value is a flag whether the operationplan is
      * acceptable (sometimes in reduced quantity) or not.
      */
    DECLARE_EXPORT bool checkOperation(OperationPlan*, SolverMRPdata& data);

    /** Verifies whether this operationplan violates the leadtime
      * constraints. */
    DECLARE_EXPORT bool checkOperationLeadtime(OperationPlan*, SolverMRPdata&, bool);

    /** Verifies whether this operationplan violates the capacity constraint.<br>
      * In case it does the operationplan is moved to an earlier or later
      * feasible date.
      */
    DECLARE_EXPORT void checkOperationCapacity(OperationPlan*, SolverMRPdata&);

    /** Scan the operationplans that are about to be committed to verify that
      * they are not creating any excess.
      */
    DECLARE_EXPORT void scanExcess(CommandManager*);

    /** Scan the operationplans that are about to be committed to verify that
      * they are not creating any excess.
      */
    DECLARE_EXPORT void scanExcess(CommandList*);
};


/** @brief This class holds functions that used for maintenance of the solver
  * code.
  */
class LibrarySolver
{
  public:
    static void initialize();
};


} // end namespace


#endif
