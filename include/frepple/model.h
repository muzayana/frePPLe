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

#ifndef MODEL_H
#define MODEL_H

/** @mainpage
  * FrePPLe provides a framework for modeling a manufacturing environment
  * and generating production plans.<br>
  * This document describes its C++ API.<P>
  *
  * @namespace frepple
  * @brief Core namespace
  */

#include "frepple/utils.h"
#include "frepple/timeline.h"
using namespace frepple::utils;

namespace frepple
{

class Flow;
class FlowEnd;
class FlowFixedStart;
class FlowFixedEnd;
class FlowPlan;
class LoadPlan;
class Resource;
class ResourceInfinite;
class ResourceBuckets;
class Problem;
class Demand;
class OperationPlan;
class Item;
class Operation;
class OperationPlanState;
class OperationFixedTime;
class OperationTimePer;
class OperationRouting;
class OperationAlternate;
class Buffer;
class BufferInfinite;
class BufferProcure;
class Plan;
class Plannable;
class Calendar;
class Load;
class Location;
class Customer;
class HasProblems;
class Solvable;
class PeggingIterator;
class Skill;
class ResourceSkill;


/** @brief This class is used for initialization. */
class LibraryModel
{
  public:
    static void initialize();
};


/** @brief This is the class used to represent variables that are
  * varying over time.
  *
  * Some example usages for calendars:
  *  - A calendar defining the available capacity of a resource
  *    week by week.
  *  - The minimum inventory desired in a buffer week by week.
  *  - The working hours and holidays at a certain location.
  */
class Calendar : public HasName<Calendar>, public HasSource
{
  public:
    class BucketIterator; // Forward declaration
    class EventIterator; // Forward declaration

    /** @brief This class represents a time bucket as a part of a calendar.
      *
      * Manipulation of instances of this class need to be handled with the
      * methods on the friend class Calendar.
      * @see Calendar
      */
    class Bucket : public Object, public NonCopyable
    {
        friend class Calendar;
        friend class BucketIterator;
        friend class EventIterator;
      private:
        /** Unique identifier of the bucket within the calendar. */
        int id;

        /** Start date of the bucket. */
        Date startdate;

        /** End Date of the bucket. */
        Date enddate;

        /** A pointer to the next bucket. */
        Bucket* nextBucket;

        /** A pointer to the previous bucket. */
        Bucket* prevBucket;

        /** Priority of this bucket, compared to other buckets effective
          * at a certain time.
          */
        int priority;

        /** Weekdays on which the entry is effective.
          * - Bit 0: Sunday
          * - Bit 1: Monday
          * - Bit 2: Tueday
          * - Bit 3: Wednesday
          * - Bit 4: Thursday
          * - Bit 5: Friday
          * - Bit 6: Saturday
          */
        short days;

        /** Starting time on the effective days. */
        TimePeriod starttime;

        /** Ending time on the effective days. */
        TimePeriod endtime;

        /** A pointer to the owning calendar. */
        Calendar *cal;

        /** An internally managed data structure to keep the offsets
          * inside the week where the entry changes effectivity. */
        long offsets[14];

        /** An internal counter for the number of indices used in the
          * offset array. */
        short offsetcounter;

        /** Updates the offsets data structure. */
        DECLARE_EXPORT void updateOffsets();

        /** Increments an iterator to the next change event.<br>
          * A bucket will evaluate the current state of the iterator, and
          * update it if a valid next event can be generated.
          */
        DECLARE_EXPORT void nextEvent(EventIterator*, Date) const;

        /** Increments an iterator to the previous change event.<br>
          * A bucket will evaluate the current state of the iterator, and
          * update it if a valid previous event can be generated.
          */
        DECLARE_EXPORT void prevEvent(EventIterator*, Date) const;

        /** Keep all calendar buckets sorted in ascending order of start date
          * and use the priority as a tie breaker.
          */
        DECLARE_EXPORT void updateSort();

      protected:
        /** Constructor. */
        Bucket(Calendar *c, Date start, Date end, int ident=INT_MIN, int priority=0) :
          startdate(start), enddate(end), nextBucket(NULL),
          prevBucket(NULL), priority(priority), days(127), starttime(0L),
          endtime(86400L), cal(c)
        {
          setId(ident);
          if (c->firstBucket) c->firstBucket->prevBucket = this;
          nextBucket = c->firstBucket;
          c->firstBucket = this;
          updateOffsets();
          updateSort();
        }

        /** Auxilary function to write out the start of the XML. */
        DECLARE_EXPORT void writeHeader(XMLOutput *, const Keyword&) const;

      public:
        /** Return the calendar to whom the bucket belongs. */
        Calendar* getCalendar() const {return cal;}

        /** Get the identifier. */
        int getId() const {return id;}

        /** Generate the identfier.<br>
          * If a bucket with the given identifier already exists a unique
          * number is generated instead. This is done by incrementing the
          * value passed until it is unique.
          */
        DECLARE_EXPORT void setId(int ident=INT_MIN);

        /** Returns the end date of the bucket. */
        Date getEnd() const {return enddate;}

        /** Updates the end date of the bucket. */
        DECLARE_EXPORT void setEnd(const Date d);

        /** Returns the start date of the bucket. */
        Date getStart() const {return startdate;}

        /** Updates the start date of the bucket. */
        DECLARE_EXPORT void setStart(const Date d);

        /** Returns the priority of this bucket, compared to other buckets
          * effective at a certain time.<br>
          * Lower numbers indicate a higher priority level.<br>
          * The default value is 0.
          */
        int getPriority() const {return priority;}

        /** Updates the priority of this bucket, compared to other buckets
          * effective at a certain time.<br>
          * Lower numbers indicate a higher priority level.<br>
          * The default value is 0.
          */
        void setPriority(int f) {priority = f; updateSort();}

        /** Get the days on which the entry is valid.<br>
          * The value is a bit pattern with bit 0 representing sunday, bit 1
          * monday, ... and bit 6 representing saturday.<br>
          * The default value is 127.
          */
        short getDays() const {return days;}

        /** Update the days on which the entry is valid. */
        void setDays(short p)
        {
          if (p<0 || p>127)
            throw DataException("Calendar bucket days must be between 0 and 127");
          days = p;
          updateOffsets();
        }

        /** Return the time of the day when the entry becomes valid.<br>
          * The default value is 0 or midnight.
          */
        TimePeriod getStartTime() const {return starttime;}

        /** Update the time of the day when the entry becomes valid. */
        void setStartTime(TimePeriod t)
        {
          if (t > 86399L || t < 0L)
            throw DataException("Calendar bucket start time must be between 0 and 86399 seconds");
          starttime = t;
          updateOffsets();
        }

        /** Return the time of the day when the entry becomes invalid.<br>
          * The default value is 23h59m59s.
          */
        TimePeriod getEndTime() const {return endtime;}

        /** Update the time of the day when the entry becomes invalid. */
        void setEndTime(TimePeriod t)
        {
          if (t > 86400L || t < 0L)
            throw DataException("Calendar bucket end time must be between 0 and 86400 seconds");
          endtime = t;
          updateOffsets();
        }

        /** Convert the value of the bucket to a boolean value. */
        virtual bool getBool() const {return true;}

        virtual DECLARE_EXPORT void writeElement
        (XMLOutput*, const Keyword&, mode=DEFAULT) const;

        /** Reads the bucket information from the input. Only the fields "name"
          * and "start" are read in. Other fields as also written out but these
          * are information-only fields.
          */
        DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);

        virtual const MetaClass& getType() const {return *metadata;}
        virtual size_t getSize() const {return sizeof(Bucket);}
        static DECLARE_EXPORT const MetaCategory* metadata;
        virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
        virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
        static int initialize();
    };

    /** Default constructor. */
    explicit DECLARE_EXPORT Calendar(const string& n) :
      HasName<Calendar>(n), firstBucket(NULL) {}

    /** Destructor, which cleans up the buckets too and all references to the
      * calendar from the core model.
      */
    DECLARE_EXPORT ~Calendar();

    /** Convert the value of the calendar to a boolean value. */
    virtual bool getBool() const {return false;}

    /** This is a factory method that creates a new bucket using the start
      * date as the key field. The fields are passed as an array of character
      * pointers.<br>
      * This method is intended to be used to create objects when reading
      * XML input data.
      */
    DECLARE_EXPORT Bucket* createBucket(const AttributeList&);

    /** Adds a new bucket to the list. */
    DECLARE_EXPORT Bucket* addBucket(Date, Date, int = 1);

    /** Removes a bucket from the list. */
    DECLARE_EXPORT void removeBucket(Bucket* bkt);

    /** Returns the bucket where a certain date belongs to.
      * A NULL pointer is returned when no bucket is effective.
      */
    DECLARE_EXPORT Bucket* findBucket(Date d, bool fwd = true) const;

    /** Returns the bucket with a certain identifier.
      * A NULL pointer is returned in case no bucket can be found with the
      * given identifier.
      */
    DECLARE_EXPORT Bucket* findBucket(int ident) const;

    /** Find an existing bucket with a given identifier, or create a new one.
      * If no identifier is passed, we always create a new bucket and automatically
      * generate a unique identifier for it.
      */
    static DECLARE_EXPORT PyObject* addPythonBucket(PyObject*, PyObject*, PyObject*);

    /** @brief An iterator class to go through all dates where the calendar
      * value changes.*/
    class EventIterator
    {
        friend class Calendar::Bucket;
      protected:
        const Calendar* theCalendar;
        const Bucket* curBucket;
        const Bucket* lastBucket;
        Date curDate;
        int curPriority;
        int lastPriority;
      public:
        const Date& getDate() const {return curDate;}
        const Bucket* getBucket() const {return curBucket;}
        const Calendar* getCalendar() const {return theCalendar;}
        EventIterator(const Calendar* c = NULL, Date d = Date::infinitePast,
            bool forward = true) : theCalendar(c), curDate(d)
        {
          curBucket = lastBucket = c ? c->findBucket(d,forward) : NULL;
          curPriority = lastPriority = curBucket ? curBucket->priority : INT_MAX;
        };
        DECLARE_EXPORT EventIterator& operator++();
        DECLARE_EXPORT EventIterator& operator--();
        EventIterator operator++(int)
        {
          EventIterator tmp = *this; ++*this; return tmp;
        }
        EventIterator operator--(int)
        {
          EventIterator tmp = *this; --*this; return tmp;
        }
    };

    /** @brief An iterator class to go through all buckets of the calendar. */
    class BucketIterator
    {
      private:
        Bucket* curBucket;
      public:
        BucketIterator(Bucket* b = NULL) : curBucket(b) {}
        bool operator != (const BucketIterator &b) const
        {return b.curBucket != curBucket;}
        bool operator == (const BucketIterator &b) const
        {return b.curBucket == curBucket;}
        BucketIterator& operator++()
        {if (curBucket) curBucket = curBucket->nextBucket; return *this;}
        BucketIterator operator++(int)
        {BucketIterator tmp = *this; ++*this; return tmp;}
        BucketIterator& operator--()
        {if(curBucket) curBucket = curBucket->prevBucket; return *this;}
        BucketIterator operator--(int)
        {BucketIterator tmp = *this; --*this; return tmp;}
        Bucket* operator ->() const {return curBucket;}
        Bucket& operator *() const {return *curBucket;}
    };

    /** Returns an iterator to go through the list of buckets. */
    BucketIterator beginBuckets() const {return BucketIterator(firstBucket);}

    /** Returns an iterator to go through the list of buckets. */
    BucketIterator endBuckets() const {return BucketIterator(NULL);}

    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    static DECLARE_EXPORT PyObject* getEvents(PyObject*, PyObject*);

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

    virtual size_t getSize() const
    {
      size_t i = sizeof(Calendar) + getName().size();
      for (BucketIterator j = beginBuckets(); j!= endBuckets(); ++j)
        i += j->getSize();
      return i;
    }

  protected:
    /** Find the lowest priority of any bucket. */
    int lowestPriority() const
    {
      int min = 0;
      for (BucketIterator i = beginBuckets(); i != endBuckets(); ++i)
        if (i->getPriority() < min) min = i->getPriority();
      return min;
    }

  private:
    /** A pointer to the first bucket. The buckets are stored in a doubly
      * linked list. */
    Bucket* firstBucket;

    /** This is the factory method used to generate new buckets. Each subclass
      * should provide an override for this function. */
    virtual Bucket* createNewBucket(Date start, Date end, int id=1, int priority=0)
    {return new Bucket(this, start, end, id, priority);}
};


/** @brief A calendar storing double values in its buckets. */
class CalendarDouble : public Calendar
{
  public:
    /** @brief A special type of calendar bucket, designed to hold a
      * a value.
      * @see Calendar::Bucket
      */
    class BucketDouble : public Calendar::Bucket
    {
        friend class CalendarDouble;
      private:
        /** This is the value stored in this bucket. */
        double val;

        /** Constructor. */
        BucketDouble(CalendarDouble *c, Date start, Date end, int id=INT_MIN, int priority=0)
          : Bucket(c, start, end, id, priority), val(0) {initType(metadata);}

      public:
        /** Returns the value of this bucket. */
        double getValue() const {return val;}

        /** Convert the value of the bucket to a boolean value. */
        bool getBool() const {return val != 0;}

        /** Updates the value of this bucket. */
        void setValue(const double v) {val = v;}

        void writeElement
        (XMLOutput *o, const Keyword& tag, mode m = DEFAULT) const
        {
          assert(m == DEFAULT || m == FULL);
          writeHeader(o, tag);
          if (getPriority()) o->writeElement(Tags::tag_priority, getPriority());
          if (val) o->writeElement(Tags::tag_value, val);
          if (getDays() != 127) o->writeElement(Tags::tag_days, getDays());
          if (getStartTime())
            o->writeElement(Tags::tag_starttime, getStartTime());
          if (getEndTime() != TimePeriod(86400L))
            o->writeElement(Tags::tag_endtime, getEndTime());
          PythonDictionary::write(o, getDict());
          o->EndObject(tag);
        }

        void endElement (XMLInput& pIn, const Attribute& pAttr, const DataElement& pElement)
        {
          if (pAttr.isA(Tags::tag_value))
            pElement >> val;
          else
            Bucket::endElement(pIn, pAttr, pElement);
        }


        virtual size_t getSize() const
        {return sizeof(CalendarDouble::BucketDouble);}

        static DECLARE_EXPORT const MetaClass* metadata;
        virtual const MetaClass& getType() const {return *metadata;}
        static int initialize();
    };

    /** @brief A special event iterator, providing also access to the
      * current value. */
    class EventIterator : public Calendar::EventIterator
    {
      public:
        /** Constructor. */
        EventIterator(const Calendar* c, Date d = Date::infinitePast,
            bool f = true) : Calendar::EventIterator(c,d,f) {}

        /** Return the current value of the iterator at this date. */
        double getValue()
        {
          return curBucket ?
              static_cast<const CalendarDouble::BucketDouble*>(curBucket)->getValue() :
              static_cast<const CalendarDouble*>(theCalendar)->getDefault();
        }
    };

  public:
    /** Default constructor. */
    CalendarDouble(const string& n) : Calendar(n)
    {setDefault(0.0); initType(metadata);}

    /** Destructor. */
    DECLARE_EXPORT ~CalendarDouble();

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    static DECLARE_EXPORT PyObject* setPythonValue(PyObject*, PyObject*, PyObject*);

    void endElement(XMLInput&, const Attribute&, const DataElement&);
    void writeElement(XMLOutput*, const Keyword&, mode m=DEFAULT) const;

    /** Returns the value on the specified date. */
    double getValue(const Date d) const
    {
      BucketDouble* x = static_cast<BucketDouble*>(findBucket(d));
      return x ? x->getValue() : defaultValue;
    }

    /** Updates the value in a certain date range.<br>
      * This will create a new bucket if required.
      */
    void setValue(Date start, Date end, const double v);

    double getValue(Calendar::BucketIterator& i) const
    {
      return reinterpret_cast<BucketDouble&>(*i).getValue();
    }

    /** Returns the default calendar value when no entry is matching. */
    double getDefault() const {return defaultValue;}

    /** Convert the value of the calendar to a boolean value. */
    virtual bool getBool() const {return defaultValue != 0;}

    /** Update the default calendar value when no entry is matching. */
    virtual void setDefault(const double v) {defaultValue = v;}

  private:
    /** Factory method to add new buckets to the calendar.
      * @see Calendar::addBucket()
      */
    Bucket* createNewBucket(Date start, Date end, int id, int priority=0)
    {return new BucketDouble(this, start, end, id, priority);}

    /** Value when no bucket is matching a certain date. */
    double defaultValue;
};


/** @brief A problem represents infeasibilities, alerts and warnings in
  * the plan.
  *
  * Problems are maintained internally by the system. They are thus only
  * exported, meaning that you can't directly import or create problems.<br>
  * This class is the pure virtual base class for all problem types.<br>
  * The usage of the problem objects is based on the following principles:
  *  - Problems objects are passive. They don't actively change the model
  *    state.
  *  - Objects of the HasProblems class actively create and destroy Problem
  *    objects.
  *  - Problem objects are managed in a 'lazy' way, meaning they only are
  *    getting created when the list of problems is requested by the user.<br>
  *    During normal planning activities we merely mark the planning entities
  *    that have changed, so we can easily pick up which entities to recompute
  *    the problems for. In this way we can avoid the cpu and memory overhead
  *    of keeping the problem list up to date at all times, while still
  *    providing the user with the correct list of problems when required.
  *  - Given the above, Problems are lightweight objects that consume
  *    limited memory.
  */
class Problem : public NonCopyable, public Object
{
  public:
    class const_iterator;
    friend class const_iterator;
    class List;
    friend class List;

    /** Constructor.<br>
      * Note that this method can't manipulate the problem container, since
      * the problem objects aren't fully constructed yet.
      * @see addProblem
      */
    explicit Problem(HasProblems *p = NULL) : owner(p), nextProblem(NULL)
    {initType(metadata);}

    /** Initialize the class. */
    static int initialize();

    /** Destructor.
      * @see removeProblem
      */
    virtual ~Problem() {}

    /** Returns the duration of this problem. */
    virtual const DateRange getDates() const = 0;

    /** Returns a text description of this problem. */
    virtual string getDescription() const = 0;

    /** Returns the object type having this problem. */
    virtual string getEntity() const = 0;

    /** Returns true if the plan remains feasible even if it contains this
      * problem, i.e. if the problems flags only a warning.
      * Returns false if a certain problem points at an infeasibility of the
      * plan.
      */
    virtual bool isFeasible() const = 0;

    /** Returns a double number reflecting the magnitude of the problem. This
      * allows us to focus on the significant problems and filter out the
      * small ones.
      */
    virtual double getWeight() const = 0;

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    void endElement(XMLInput&, const Attribute&, const DataElement&) {}
    static DECLARE_EXPORT void writer(const MetaCategory*, XMLOutput*);

    PyObject* getattro(const Attribute&);

    PyObject* str() const
    {
      return PythonObject(getDescription());
    }

    /** Returns an iterator to the very first problem. The iterator can be
      * incremented till it points past the very last problem. */
    static DECLARE_EXPORT const_iterator begin();

    /** Return an iterator to the first problem of this entity. The iterator
      * can be incremented till it points past the last problem of this
      * plannable entity.<br>
      * The boolean argument specifies whether the problems need to be
      * recomputed as part of this method.
      */
    static DECLARE_EXPORT const_iterator begin(HasProblems*, bool = true);

    /** Return an iterator pointing beyond the last problem. */
    static DECLARE_EXPORT const const_iterator end();

    /** Erases the list of all problems. This methods can be used reduce the
      * memory consumption at critical points. The list of problems will be
      * recreated when the problem detection is triggered again.
      */
    static DECLARE_EXPORT void clearProblems();

    /** Erases the list of problems linked with a certain plannable object.<br>
      * If the second parameter is set to true, the problems will be
      * recreated when the next problem detection round is triggered.
      */
    static DECLARE_EXPORT void clearProblems(HasProblems& p, bool setchanged = true);

    /** Returns a pointer to the object that owns this problem. */
    virtual Object* getOwner() const = 0;

    /** Return a reference to the metadata structure. */
    virtual const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaCategory* metadata;

  protected:
    /** Each Problem object references a HasProblem object as its owner. */
    HasProblems *owner;

    /** Each Problem contains a pointer to the next pointer for the same
      * owner. This class implements thus an intrusive single linked list
      * of Problem objects. */
    Problem *nextProblem;

    /** Adds a newly created problem to the problem container.
      * This method needs to be called in the constructor of a problem
      * subclass. It can't be called from the constructor of the base
      * Problem class, since the object isn't fully created yet and thus
      * misses the proper information used by the compare method.
      * @see removeProblem
      */
    DECLARE_EXPORT void addProblem();

    /** Removes a problem from the problem container.
      * This method needs to be called from the destructor of a problem
      * subclass.<br>
      * Due to the single linked list data structure, this methods'
      * performance is linear with the number of problems of an entity.
      * This is acceptable since we don't expect entities with a huge amount
      * of problems.
      * @see addproblem
      */
    DECLARE_EXPORT void removeProblem();

    /** Comparison of 2 problems.<br>
      * To garantuee that the problems are sorted in a consistent and stable
      * way, the following sorting criteria are used (in order of priority):
      * <ol><li>Entity<br>
      *    This sort is to be ensured by the client. This method can't
      *    compare problems of different entities!</li>
      * <li>Type<br>
      *    Each problem type has a hashcode used for sorting.</li>
      * <li>Start date</li></ol>
      * The sorting is expected such that it can be used as a key, i.e. no
      * two problems of will ever evaluate to be identical.
      */
    DECLARE_EXPORT bool operator < (const Problem& a) const;
};


/** @brief Classes that keep track of problem conditions need to implement
  * this class.
  *
  * This class is closely related to the Problem class.
  * @see Problem
  */
class HasProblems
{
    friend class Problem::const_iterator;
    friend class Problem;
  public:
    class EntityIterator;

    /** Returns an iterator pointing to the first HasProblem object. */
    static DECLARE_EXPORT EntityIterator beginEntity();

    /** Returns an iterator pointing beyond the last HasProblem object. */
    static DECLARE_EXPORT EntityIterator endEntity();

    /** Constructor. */
    HasProblems() : firstProblem(NULL) {}

    /** Destructor. It needs to take care of making sure all problems objects
      * are being deleted as well. */
    virtual ~HasProblems() {Problem::clearProblems(*this, false);}

    /** Returns the plannable entity relating to this problem container. */
    virtual Plannable* getEntity() const = 0;

    /** Called to update the list of problems. The function will only be
      * called when:
      *  - the list of problems is being recomputed
      *  - AND, problem detection is enabled for this object
      *  - AND, the object has changed since the last problem computation
      */
    virtual void updateProblems() = 0;

  private:
    /** A pointer to the first problem of this object. Problems are maintained
      * in a single linked list. */
    Problem* firstProblem;
};


/** @brief This auxilary class is used to maintain a list of problem models. */
class Problem::List
{
  public:
    /** Constructor. */
    List() : first(NULL) {};

    /** Destructor. */
    ~List() {clear();}

    /** Empty the list.<br>
      * If a problem is passed as argument, that problem and all problems
      * following it in the list are deleted.<br>
      * If no argument is passed, the complete list is erased.
      */
    DECLARE_EXPORT void clear(Problem * = NULL);

    /** Add a problem to the list. */
    DECLARE_EXPORT Problem* push
    (const MetaClass*, const Object*, Date, Date, double);

    /** Remove all problems from the list that appear AFTER the one
      * passed as argument. */
    DECLARE_EXPORT void pop(Problem *);

    /** Get the last problem on the list. */
    DECLARE_EXPORT Problem* top() const;

    /** Cur the list in two parts . */
    DECLARE_EXPORT Problem* unlink(Problem* p)
    {
      Problem *tmp = p->nextProblem;
      p->nextProblem = NULL;
      return tmp;
    }

    /** Returns true if the list is empty. */
    bool empty() const {return first == NULL;}

    /** Return an iterator to the start of the list. */
    Problem::const_iterator begin() const;

    /** End iterator. */
    Problem::const_iterator end() const;

  private:
    /** Pointer to the head of the list. */
    Problem* first;
};


/** @brief This class is an implementation of the "visitor" design pattern.
  * It is intended as a basis for different algorithms processing the frePPLe
  * data.
  *
  * The goal is to decouple the solver/algorithms from the model/data
  * representation. Different solvers can be easily be plugged in to work on
  * the same data.
  */
class Solver : public HasName<Solver>
{
  public:
    /** Constrcutor. */
    explicit DECLARE_EXPORT Solver(const string& n) :
      HasName<Solver>(n), loglevel(0) {}

    /** Destructor. */
    virtual ~Solver() {}

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    static DECLARE_EXPORT PyObject* solve(PyObject*, PyObject*);

    virtual void solve(void* = NULL) = 0;
    virtual void solve(const Demand*,void* = NULL)
    {throw LogicException("Called undefined solve(Demand*) method");}
    virtual void solve(const Operation*,void* = NULL)
    {throw LogicException("Called undefined solve(Operation*) method");}
    virtual void solve(const OperationFixedTime* o, void* v = NULL)
    {solve(reinterpret_cast<const Operation*>(o),v);}
    virtual void solve(const OperationTimePer* o, void* v = NULL)
    {solve(reinterpret_cast<const Operation*>(o),v);}
    virtual void solve(const OperationRouting* o, void* v = NULL)
    {solve(reinterpret_cast<const Operation*>(o),v);}
    virtual void solve(const OperationAlternate* o, void* v = NULL)
    {solve(reinterpret_cast<const Operation*>(o),v);}
    virtual void solve(const Resource*,void* = NULL)
    {throw LogicException("Called undefined solve(Resource*) method");}
    virtual void solve(const ResourceInfinite* r, void* v = NULL)
    {solve(reinterpret_cast<const Resource*>(r),v);}
    virtual void solve(const ResourceBuckets* r, void* v = NULL)
    {solve(reinterpret_cast<const Resource*>(r),v);}
    virtual void solve(const Buffer*,void* = NULL)
    {throw LogicException("Called undefined solve(Buffer*) method");}
    virtual void solve(const BufferInfinite* b, void* v = NULL)
    {solve(reinterpret_cast<const Buffer*>(b),v);}
    virtual void solve(const BufferProcure* b, void* v = NULL)
    {solve(reinterpret_cast<const Buffer*>(b),v);}
    virtual void solve(const Load* b, void* v = NULL)
    {throw LogicException("Called undefined solve(Load*) method");}
    virtual void solve(const Flow* b, void* v = NULL)
    {throw LogicException("Called undefined solve(Flow*) method");}
    virtual void solve(const FlowEnd* b, void* v = NULL)
    {solve(reinterpret_cast<const Flow*>(b),v);}
    virtual void solve(const FlowFixedStart* b, void* v = NULL)
    {solve(reinterpret_cast<const Flow*>(b),v);}
    virtual void solve(const FlowFixedEnd* b, void* v = NULL)
    {solve(reinterpret_cast<const Flow*>(b),v);}
    virtual void solve(const Solvable*,void* = NULL)
    {throw LogicException("Called undefined solve(Solvable*) method");}

    /** Returns how elaborate and verbose output is requested.<br>
      * As a guideline solvers should respect the following guidelines:
      * - 0:<br>
      *   Completely silent.<br>
      *   This is the default value.
      * - 1:<br>
      *   Minimal and high-level messages on the progress that are sufficient
      *   for logging normal operation.<br>
      * - 2:<br>
      *   Higher numbers are solver dependent. These levels are typically
      *   used for debugging and tracing, and provide more detail on the
      *   solver's progress.
      */
    unsigned short getLogLevel() const {return loglevel;}

    /** Controls whether verbose output will be generated. */
    void setLogLevel(unsigned short v) {loglevel = v;}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

  private:
    /** Controls the amount of tracing and debugging messages. */
    unsigned short loglevel;
};


/** @brief This class needs to be implemented by all classes that implement
  * dynamic behavior, and which can be called by a solver.
  */
class Solvable
{
  public:
    /** This method is called by solver classes. The implementation of this
      * class simply calls the solve method on the solver class. Using the
      * polymorphism the solver can implement seperate methods for different
      * plannable subclasses.
      */
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    /** Destructor. */
    virtual ~Solvable() {}
};


/** @brief This class needs to be implemented by all classes that implement
  * dynamic behavior in the plan.
  *
  * The problem detection logic is implemented in the detectProblems() method.
  * For performance reasons, problem detection is "lazy", i.e. problems are
  * computed only when somebody really needs the access to the list of
  * problems.
  */
class Plannable : public HasProblems, public Solvable
{
  public:
    /** Constructor. */
    Plannable() : useProblemDetection(true), changed(true)
    {anyChange = true;}

    /** Specify whether this entity reports problems. */
    DECLARE_EXPORT void setDetectProblems(bool b);

    /** Returns whether or not this object needs to detect problems. */
    bool getDetectProblems() const {return useProblemDetection;}

    /** Loops through all plannable objects and updates their problems if
      * required. */
    static DECLARE_EXPORT void computeProblems();

    /** See if this entity has changed since the last problem
      * problem detection run. */
    bool getChanged() const {return changed;}

    /** Mark that this entity has been updated and that the problem
      * detection needs to be redone. */
    void setChanged(bool b = true) {changed=b; if (b) anyChange=true;}

    /** Implement the pure virtual function from the HasProblem class. */
    Plannable* getEntity() const {return const_cast<Plannable*>(this);}

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);

  private:
    /** Stores whether this entity should be skip problem detection, or not. */
    bool useProblemDetection;

    /** Stores whether this entity has been updated since the last problem
      * detection run. */
    bool changed;

    /** Marks whether any entity at all has changed its status since the last
      * problem detection round.
      */
    static DECLARE_EXPORT bool anyChange;

    /** This flag is set to true during the problem recomputation. It is
      * required to garantuee safe access to the problems in a multi-threaded
      * environment.
      */
    static DECLARE_EXPORT bool computationBusy;
};


/** @brief The purpose of this class is to compute the levels of all buffers,
  * operations and resources in the model, and to categorize them in clusters.
  *
  * Resources and buffers linked to the delivery operations of
  * the demand are assigned level 1. buffers one level upstream have
  * level 2, and so on...
  *
  * A cluster is group of planning entities (buffers, resources and operations)
  * that are linked together using loads and/or flows. Each cluster can be seen
  * as a completely independent part of the model and the planning problem.
  * There is no interaction possible between clusters.
  * Cluster 0 is a special case: it contains all entities not connected to any other
  * entity at all.
  * Clusters are helpful in multi-threading the planning problem, partial
  * replanning of the model, etc...
  */
class HasLevel
{
  private:
    /** Flags whether the current computation is still up to date or not.
      * The flag is set when new objects of this are created or updated.
      * Running the computeLevels function clears the flag.
      */
    static DECLARE_EXPORT bool recomputeLevels;

    /** This flag is set to true during the computation of the levels. It is
      * required to ensure safe access to the level information in a
      * multi-threaded environment.
      */
    static DECLARE_EXPORT bool computationBusy;

    /** Stores the total number of clusters in the model. */
    static DECLARE_EXPORT unsigned short numberOfClusters;

    /** Stores the maximum level number in the model. */
    static DECLARE_EXPORT unsigned short numberOfLevels;

    /** Stores the level of this entity. Higher numbers indicate more
      * upstream entities.
      * A value of -1 indicates an unused entity.
      */
    short lvl;

    /** Stores the cluster number of the current entity. */
    unsigned short cluster;

  protected:
    /** Default constructor. The initial level is -1 and basically indicates
      * that this HasHierarchy (either Operation, Buffer or Resource) is not
      * being used at all...
      */
    HasLevel() : lvl(0), cluster(0) {}

    /** Copy constructor. Since the characterictics of the new object are the
      * same as the original, the level and cluster are also the same.
      * No recomputation is required.
      */
    HasLevel(const HasLevel& o) : lvl(o.lvl), cluster(o.cluster) {}

    /** Destructor. Deleting a HasLevel object triggers recomputation of the
      * level and cluster computation, since the network now has changed.
      */
    ~HasLevel() {recomputeLevels = true;}

    /** This function recomputes all levels in the model.
      * It is called automatically when the getLevel or getCluster() function
      * on a Buffer, Resource or Operation are called while the
      * "recomputeLevels" flag is set.
      * Right, this is an example of a 'lazy' algorithm: only compute the
      * information when it is required. Note however that the computation
      * is triggered over the complete model, not a subset...
      * The runtime of the algorithm is pretty much linear with the total
      * number of operations in the model. The cluster size also has some
      * (limited) impact on the performance: a network with larger cluster
      * size will take longer to analyze.
      * @exception LogicException Generated when there are too many clusters in
      *     your model. The maximum limit is USHRT_MAX, i.e. the greatest
      *     number that can be stored in a variable of type "unsigned short".
      *     The limit is platform dependent. On 32-bit platforms it will
      *     typically be 65535.
      */
    static DECLARE_EXPORT void computeLevels();

  public:
    /** Returns the total number of levels.<br>
      * If not up to date the recomputation will be triggered.
      */
    static unsigned short getNumberOfLevels()
    {
      if (recomputeLevels || computationBusy) computeLevels();
      return numberOfLevels;
    }

    /** Returns the total number of clusters.<br>
      * If not up to date the recomputation will be triggered.
      */
    static unsigned short getNumberOfClusters()
    {
      if (recomputeLevels || computationBusy) computeLevels();
      return numberOfClusters;
    }

    /** Return the level (and recompute first if required). */
    short getLevel() const
    {
      if (recomputeLevels || computationBusy) computeLevels();
      return lvl;
    }

    /** Return the cluster number (and recompute first if required). */
    unsigned short getCluster() const
    {
      if (recomputeLevels || computationBusy) computeLevels();
      return cluster;
    }

    /** This function should be called when something is changed in the network
      * structure. The notification sets a flag, but does not immediately
      * trigger the recomputation.
      * @see computeLevels
      */
    static void triggerLazyRecomputation() {recomputeLevels = true;}
};


/** @brief This abstract class is used to associate buffers and resources with
  * a physical or logical location.
  *
  * The 'available' calendar is used to model the working hours and holidays
  * of resources, buffers and operations.
  */
class Location : public HasHierarchy<Location>, public HasDescription
{
  public:
    /** Constructor. */
    explicit DECLARE_EXPORT Location(const string& n) :
      HasHierarchy<Location>(n), available(NULL) {}

    /** Destructor. */
    virtual DECLARE_EXPORT ~Location();

    /** Returns the availability calendar of the location.<br>
      * The availability calendar models the working hours and holidays. It
      * applies to all operations, resources and buffers using this location.
      */
    CalendarDouble *getAvailable() const {return available;}

    /** Updates the availability calendar of the location. */
    void setAvailable(CalendarDouble* b) {available = b;}

    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    size_t extrasize() const
    {return getName().size() + HasDescription::extrasize();}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    static int initialize();

  private:
    /** The availability calendar models the working hours and holidays. It
      * applies to all operations, resources and buffers using this location.
      */
    CalendarDouble* available;
};


/** @brief This class implements the abstract Location class. */
class LocationDefault : public Location
{
  public:
    explicit LocationDefault(const string& str) : Location(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(LocationDefault) + Location::extrasize();}
    static int initialize();
};


/** @brief This abstracts class represents customers.
  *
  * Demands can be associated with a customer, but there is no planning
  * behavior directly linked to customers.
  */
class Customer : public HasHierarchy<Customer>, public HasDescription
{
  public:
    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    size_t extrasize() const
    {return getName().size() + HasDescription::extrasize();}

    /** Constructor. */
    explicit DECLARE_EXPORT Customer(const string& n) : HasHierarchy<Customer>(n) {}

    /** Destructor. */
    virtual DECLARE_EXPORT ~Customer();

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    static int initialize();
};


/** @brief This class implements the abstract Customer class. */
class CustomerDefault : public Customer
{
  public:
    explicit CustomerDefault(const string& str) : Customer(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(CustomerDefault) + Customer::extrasize();}
    static int initialize();
};


/** @brief An operation represents an activity: these consume and produce material,
  * take time and also require capacity.
  *
  * An operation consumes and produces material, modeled through flows.<br>
  * An operation requires capacity, modeled through loads.
  *
  * This is an abstract base class for all different operation types.
  */
class Operation : public HasName<Operation>,
  public HasLevel, public Plannable, public HasDescription
{
    friend class Flow;
    friend class Load;
    friend class OperationPlan;
    friend class OperationRouting;
    friend class OperationAlternate;

  protected:
    /** Constructor. Don't use it directly. */
    explicit DECLARE_EXPORT Operation(const string& str) :
      HasName<Operation>(str), loc(NULL), size_minimum(1.0), size_multiple(0.0),
      size_maximum(DBL_MAX), cost(0.0), hidden(false), first_opplan(NULL),
      last_opplan(NULL) {}

    /** Extra logic called when instantiating an operationplan.<br>
      * When the function returns false the creation of the operationplan
      * is denied and it is deleted.
      */
    virtual bool extraInstantiate(OperationPlan* o) {return true;}

  public:
    /** Destructor. */
    virtual DECLARE_EXPORT ~Operation();

    /** Returns a pointer to the operationplan being instantiated. */
    OperationPlan* getFirstOpPlan() const {return first_opplan;}

    /** Returns the delay before this operation.    TODO CURRENTLY NOT USED!
      * @see setPreTime
      */
    TimePeriod getPreTime() const {return pre_time;}

    /** Updates the delay before this operation.<br>
      * This delay is a soft constraint. This means that solvers should try to
      * respect this waiting time but can choose to leave a shorter time delay
      * if required.<br>
      * @see setPostTime
      */
    void setPreTime(TimePeriod t)
    {
      if (t<TimePeriod(0L))
        throw DataException("No negative pre-operation time allowed");
      pre_time=t;
      setChanged();
    }

    /** Returns the delay after this operation.
      * @see setPostTime
      */
    TimePeriod getPostTime() const {return post_time;}

    /** Updates the delay after this operation.<br>
      * This delay is a soft constraint. This means that solvers should try to
      * respect this waiting time but can choose to leave a shorter time delay
      * if required.
      * @see setPreTime
      */
    void setPostTime(TimePeriod t)
    {
      if (t<TimePeriod(0L))
        throw DataException("No negative post-operation time allowed");
      post_time=t;
      setChanged();
    }

    /** Return the operation cost.<br>
      * The cost of executing this operation, per unit of the
      * operation_plan.<br>
      * The default value is 0.0.
      */
    double getCost() const {return cost;}

    /** Update the operation cost.<br>
      * The cost of executing this operation, per unit of the operation_plan.
      */
    void setCost(const double c)
    {
      if (c >= 0) cost = c;
      else throw DataException("Operation cost must be positive");
    }

    typedef Association<Operation,Buffer,Flow>::ListA flowlist;
    typedef Association<Operation,Resource,Load>::ListA loadlist;

    /** This is the factory method which creates all operationplans of the
      * operation. */
    DECLARE_EXPORT OperationPlan* createOperationPlan(double, Date,
        Date, Demand* = NULL, OperationPlan* = NULL, unsigned long = 0,
        bool makeflowsloads=true) const;

    /** Calculates the daterange starting from (or ending at) a certain date
      * and using a certain amount of effective available time on the
      * operation.
      *
      * This calculation considers the availability calendars of:
      * - the availability calendar of the operation's location
      * - the availability calendar of all resources loaded by the operation @todo not implemented yet
      * - the availability calendar of the locations of all resources loaded @todo not implemented yet
      *   by the operation
      *
      * @param[in] thedate  The date from which to start searching.
      * @param[in] duration The amount of available time we are looking for.
      * @param[in] forward  The search direction
      * @param[out] actualduration This variable is updated with the actual
      *             amount of available time found.
      */
    DECLARE_EXPORT DateRange calculateOperationTime
    (Date thedate, TimePeriod duration, bool forward,
     TimePeriod* actualduration = NULL) const;

    /** Calculates the effective, available time between two dates.
      *
      * This calculation considers the availability calendars of:
      * - the availability calendar of the operation's location
      * - the availability calendar of all resources loaded by the operation @todo not implemented yet
      * - the availability calendar of the locations of all resources loaded @todo not implemented yet
      *   by the operation
      *
      * @param[in] start  The date from which to start searching.
      * @param[in] end    The date where to stop searching.
      * @param[out] actualduration This variable is updated with the actual
      *             amount of available time found.
      */
    DECLARE_EXPORT DateRange calculateOperationTime
    (Date start, Date end, TimePeriod* actualduration = NULL) const;

    /** This method stores ALL logic the operation needs to compute the
      * correct relationship between the quantity, startdate and enddate
      * of an operationplan.
      *
      * The parameters "startdate", "enddate" and "quantity" can be
      * conflicting if all are specified together.
      * Typically, one would use one of the following combinations:
      *  - specify quantity and start date, and let the operation compute the
      *    end date.
      *  - specify quantity and end date, and let the operation compute the
      *    start date.
      *  - specify both the start and end date, and let the operation compute
      *    the quantity.
      *  - specify quantity, start and end date. In this case, you need to
      *    be aware that the operationplan that is created can be different
      *    from the parameters you requested.
      *
      * The following priority rules apply upon conflicts.
      *  - respecting the end date has the first priority.
      *  - respecting the start date has second priority.
      *  - respecting the quantity should be done if the specified dates can
      *    be respected.
      *  - if the quantity is being computed to meet the specified dates, the
      *    quantity being passed as argument is to be treated as a maximum
      *    limit. The created operationplan can have a smaller quantity, but
      *    not bigger...
      *  - at all times, we expect to have an operationplan that is respecting
      *    the constraints set by the operation. If required, some of the
      *    specified parameters may need to be violated. In case of such a
      *    violation we expect the operationplan quantity to be 0.
      *
      * The pre- and post-operation times are NOT considered in this method.
      * This method only enforces "hard" constraints. "Soft" constraints are
      * considered as 'hints' by the solver.
      *
      * Subclasses need to override this method to implement the correct
      * logic.
      */
    virtual OperationPlanState setOperationPlanParameters
    (OperationPlan*, double, Date, Date, bool=true, bool=true) const = 0;

    /** Updates the quantity of an operationplan.<br>
      * This method considers the lot size constraints and also propagates
      * the new quantity to child operationplans.
      */
    virtual DECLARE_EXPORT double setOperationPlanQuantity
      (OperationPlan* oplan, double f, bool roundDown, bool upd, bool execute) const;

    /** Returns the location of the operation, which is used to model the
      * working hours and holidays. */
    Location* getLocation() const {return loc;}

    /** Updates the location of the operation, which is used to model the
      * working hours and holidays. */
    void setLocation(Location* l) {loc = l;}

    /** Returns an reference to the list of flows. */
    const flowlist& getFlows() const {return flowdata;}

    /** Returns an reference to the list of loads. */
    const loadlist& getLoads() const {return loaddata;}

    /** Return the flow that is associates a given buffer with this
      * operation. Returns NULL is no such flow exists. */
    Flow* findFlow(const Buffer* b, Date d) const
    {return flowdata.find(b,d);}

    /** Return the load that is associates a given resource with this
      * operation. Returns NULL is no such load exists. */
    Load* findLoad(const Resource* r, Date d) const
    {return loaddata.find(r,d);}

    /** Deletes all operationplans of this operation. The boolean parameter
      * controls whether we delete also locked operationplans or not.
      */
    DECLARE_EXPORT void deleteOperationPlans(bool deleteLockedOpplans = false);

    /** Sets the minimum size of operationplans.<br>
      * The default value is 1.0
      */
    void setSizeMinimum(double f)
    {
      if (f<0)
        throw DataException("Operation can't have a negative minimum size");
      size_minimum = f;
      setChanged();
    }

    /** Returns the minimum size for operationplans. */
    double getSizeMinimum() const {return size_minimum;}

    /** Sets the multiple size of operationplans. */
    void setSizeMultiple(double f)
    {
      if (f<0)
        throw DataException("Operation can't have a negative multiple size");
      size_multiple = f;
      setChanged();
    }

    /** Returns the mutiple size for operationplans. */
    double getSizeMultiple() const {return size_multiple;}

    /** Sets the maximum size of operationplans. */
    void setSizeMaximum(double f)
    {
      if (f < size_minimum)
        throw DataException("Operation maximum size must be higher than the minimum size");
      if (f <= 0)
        throw DataException("Operation maximum size must be greater than 0");
      size_maximum = f;
      setChanged();
    }

    /** Returns the maximum size for operationplans. */
    double getSizeMaximum() const {return size_maximum;}

    /** Add a new child operationplan.
      * By default an operationplan can have only a single suboperation,
      * representing a changeover.
      * Operation subclasses can implement their own restrictions on the
      * number and structure of the suboperationplans.
      * @see OperationAlternate::addSubOperationPlan
      * @see OperationRouting::addSubOperationPlan
      */
    virtual DECLARE_EXPORT void addSubOperationPlan(
      OperationPlan*, OperationPlan*, bool=true
      );

    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    size_t extrasize() const
    {return getName().size() + HasDescription::extrasize();}

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    typedef list<Operation*> Operationlist;

    /** Returns a reference to the list of sub operations of this operation. */
    virtual const Operationlist& getSubOperations() const {return nosubOperations;}

    /** Returns a reference to the list of super-operations, i.e. operations
      * using the current Operation as a sub-Operation.
      */
    const Operationlist& getSuperOperations() const {return superoplist;}

    /** Register a super-operation, i.e. an operation having this one as a
      * sub-operation. */
    void addSuperOperation(Operation * o) {superoplist.push_front(o);}

    /** Removes a sub-operation from the list. This method will need to be
      * overridden by all operation types that acts as a super-operation. */
    virtual void removeSubOperation(Operation *o) {}

    /** Removes a super-operation from the list. */
    void removeSuperOperation(Operation *o)
    {superoplist.remove(o); o->removeSubOperation(this);}

    /** Return the release fence of this operation. */
    TimePeriod getFence() const {return fence;}

    /** Update the release fence of this operation. */
    void setFence(TimePeriod t) {if (fence!=t) setChanged(); fence=t;}

    virtual DECLARE_EXPORT void updateProblems();

    void setHidden(bool b) {if (hidden!=b) setChanged(); hidden = b;}
    bool getHidden() const {return hidden;}

    static DECLARE_EXPORT const MetaCategory* metadata;

  protected:
    DECLARE_EXPORT void initOperationPlan(OperationPlan*, double,
        const Date&, const Date&, Demand*, OperationPlan*, unsigned long,
        bool = true) const;

  private:
    /** List of operations using this operation as a sub-operation */
    Operationlist superoplist;

    /** Empty list of operations.<br>
      * For operation types which have no suboperations this list is
      * used as the list of suboperations.
      */
    static DECLARE_EXPORT Operationlist nosubOperations;

    /** Location of the operation.<br>
      * The location is used to model the working hours and holidays.
      */
    Location* loc;

    /** Represents the time between this operation and a next one. */
    TimePeriod post_time;

    /** Represents the time between this operation and a previous one. */
    TimePeriod pre_time;

    /** Represents the release fence of this operation, i.e. a period of time
      * (relative to the current date of the plan) in which normally no
      * operationplan is allowed to be created.
      */
    TimePeriod fence;

    /** Singly linked list of all flows of this operation. */
    flowlist flowdata;

    /** Singly linked list of all resources Loaded by this operation. */
    loadlist loaddata;

    /** Minimum size for operationplans.<br>
      * The default value is 1.0
      */
    double size_minimum;

    /** Multiple size for operationplans. */
    double size_multiple;

    /** Maximum size for operationplans. */
    double size_maximum;

    /** Cost of the operation.<br>
      * The default value is 0.0.
      */
    double cost;

    /** Does the operation require serialization or not. */
    bool hidden;

    /** A pointer to the first operationplan of this operation.<br>
      * All operationplans of this operation are stored in a sorted
      * doubly linked list.
      */
    OperationPlan* first_opplan;

    /** A pointer to the last operationplan of this operation.<br>
      * All operationplans of this operation are stored in a sorted
      * doubly linked list.
      */
    OperationPlan* last_opplan;
};


/** @brief An operationplan is the key dynamic element of a plan. It
  * represents a certain quantity being planned along a certain operation
  * during a certain date range.
  *
  * From a coding perspective:
  *  - Operationplans are created by the factory method createOperationPlan()
  *    on the matching Operation class.
  *  - The createLoadAndFlowplans() can optionally be called to also create
  *    the loadplans and flowplans, to take care of the material and
  *    capacity consumption.
  *  - Once you're sure about creating the operationplan, the activate()
  *    method should be called. It will assign the operationplan a unique
  *    numeric identifier, register the operationplan in a container owned
  *    by the operation instance, and also create loadplans and flowplans
  *    if this hasn't been done yet.<br>
  *  - Operationplans can be organized in hierarchical structure, matching
  *    the operation hierarchies they belong to.
  */
class OperationPlan
  : public Object, public HasProblems, public NonCopyable, public HasSource
{
    friend class FlowPlan;
    friend class LoadPlan;
    friend class Demand;
    friend class Operation;
    friend class OperationAlternate;
    friend class OperationRouting;
    friend class ProblemPrecedence;

  public:
    class FlowPlanIterator;

    /** Returns an iterator pointing to the first flowplan. */
    FlowPlanIterator beginFlowPlans() const;

    /** Returns an iterator pointing beyond the last flowplan. */
    FlowPlanIterator endFlowPlans() const;

    /** Returns how many flowplans are created on an operationplan. */
    int sizeFlowPlans() const;

    class LoadPlanIterator;

    /** Returns an iterator pointing to the first loadplan. */
    LoadPlanIterator beginLoadPlans() const;

    /** Returns an iterator pointing beyond the last loadplan. */
    LoadPlanIterator endLoadPlans() const;

    /** Returns how many loadplans are created on an operationplan. */
    int sizeLoadPlans() const;

    /** @brief This class models an STL-like iterator that allows us to iterate over
      * the operationplans in a simple and safe way.
      *
      * Objects of this class are created by the begin() and end() functions.
      */
    class iterator
    {
      public:
        /** Constructor. The iterator will loop only over the operationplans
          * of the operation passed. */
        iterator(const Operation* x) : op(Operation::end()), mode(1)
        {
          opplan = x ? x->getFirstOpPlan() : NULL;
        }

        /** Constructor. The iterator will loop only over the suboperationplans
          * of the operationplan passed. */
        iterator(const OperationPlan* x) : op(Operation::end()), mode(2)
        {
          opplan = x ? x->firstsubopplan : NULL;
        }

        /** Constructor. The iterator will loop over all operationplans. */
        iterator() : op(Operation::begin()), mode(3)
        {
          // The while loop is required since the first operation might not
          // have any operationplans at all
          while (op!=Operation::end() && !op->getFirstOpPlan()) ++op;
          if (op!=Operation::end())
            opplan = op->getFirstOpPlan();
          else
            opplan = NULL;
        }

        /** Copy constructor. */
        iterator(const iterator& it) : opplan(it.opplan), op(it.op), mode(it.mode) {}

        /** Return the content of the current node. */
        OperationPlan& operator*() const {return *opplan;}

        /** Return the content of the current node. */
        OperationPlan* operator->() const {return opplan;}

        /** Pre-increment operator which moves the pointer to the next
          * element. */
        iterator& operator++()
        {
          if (mode == 2)
            opplan = opplan->nextsubopplan;
          else
            opplan = opplan->next;
          // Move to a new operation
          if (!opplan && mode == 3)
          {
            do ++op;
            while (op!=Operation::end() && (!op->getFirstOpPlan()));
            if (op!=Operation::end())
              opplan = op->getFirstOpPlan();
            else
              opplan = NULL;
          }
          return *this;
        }

        /** Post-increment operator which moves the pointer to the next
          * element. */
        iterator operator++(int)
        {
          iterator tmp(*this);
          if (mode == 2)
            opplan = opplan->nextsubopplan;
          else
            opplan = opplan->next;
          // Move to a new operation
          if (!opplan && mode==3)
          {
            do ++op; while (op!=Operation::end() && !op->getFirstOpPlan());
            if (op!=Operation::end())
              opplan = op->getFirstOpPlan();
            else
              opplan = NULL;
          }
          return tmp;
        }

        /** Comparison operator. */
        bool operator==(const iterator& y) const {return opplan == y.opplan;}

        /** Inequality operator. */
        bool operator!=(const iterator& y) const {return opplan != y.opplan;}

      private:
        /** A pointer to current operationplan. */
        OperationPlan* opplan;

        /** An iterator over the operations. */
        Operation::iterator op;

        /** Describes the type of iterator.<br>
          * 1) iterate over operationplan instances of operation
          * 2) iterate over suboperationplans of an operationplan
          * 3) iterate over all operationplans
          */
        short mode;
    };

    friend class iterator;

    static iterator end() {return iterator(static_cast<Operation*>(NULL));}

    static iterator begin() {return iterator();}

    /** Returns true when not a single operationplan object exists. */
    static bool empty() {return begin()==end();}

    /** Returns the number of operationplans in the system. This method
      * is linear with the number of operationplans in the model, and should
      * therefore be used only with care.
      */
    static unsigned long size()
    {
      unsigned long cnt = 0;
      for (OperationPlan::iterator i = begin(); i != end(); ++i) ++cnt;
      return cnt;
    }

    /** This is a factory method that creates an operationplan pointer based
      * on the name and id, which are passed as an array of character pointers.
      * This method is intended to be used to create objects when reading
      * XML input data.
      */
    static DECLARE_EXPORT Object* createOperationPlan(const MetaClass*, const AttributeList&);

    /** Destructor. */
    virtual DECLARE_EXPORT ~OperationPlan();

    /** Push all consuming flowplans on a stack. */
    void DECLARE_EXPORT pushConsumingBuffers(vector<Buffer*>*) const;

    virtual DECLARE_EXPORT void setChanged(bool b = true);

    /** Returns the quantity. */
    double getQuantity() const {return quantity;}

    /** Updates the quantity.<br>
      * The operationplan quantity is subject to the following rules:
      *  - The quantity must be greater than or equal to the minimum size.<br>
      *    The value is rounded up to the smallest multiple above the minimum
      *    size if required, or rounded down to 0.
      *  - The quantity must be a multiple of the multiple_size field.<br>
      *    The value is rounded up or down to meet this constraint.
      *  - The quantity must be smaller than or equal to the maximum size.<br>
      *    The value is limited to the smallest multiple below this limit.
      *  - Setting the quantity of an operationplan to 0 is always possible,
      *    regardless of the minimum, multiple and maximum values.
      * This method can only be called on top operationplans. Sub operation
      * plans should pass on a call to the parent operationplan.
      */
    DECLARE_EXPORT double setQuantity(double f,
        bool roundDown = false, bool update = true, bool execute = true)
    {
      return oper->setOperationPlanQuantity(this, f, roundDown, update, execute);
    }

    /** Returns a pointer to the demand for which this operationplan is a delivery.
      * If the operationplan isn't a delivery, this is a NULL pointer.
      */
    Demand* getDemand() const {return dmd;}

    /** Updates the demand to which this operationplan is a solution. */
    DECLARE_EXPORT void setDemand(Demand* l);

    /** Calculate the penalty of an operationplan.<br>
      * It is the sum of all setup penalties of the resources it loads. */
    DECLARE_EXPORT double getPenalty() const;

    /** Calculate the unavailable time during the operationplan. The regular
      * duration is extended with this amount.
      */
    DECLARE_EXPORT TimePeriod getUnavailable() const;

    /** Returns whether the operationplan is locked. A locked operationplan
      * is never changed.
      */
    bool getLocked() const {return flags & IS_LOCKED;}

    /** Returns true is this operationplan is allowed to consume material.
      * This field only has an impact for locked operationplans.
      */
    bool getConsumeMaterial() const {return !(flags & CONSUME_MATERIAL);}

    /** Returns true is this operationplan is allowed to produce material.
      * This field only has an impact for locked operationplans.
      */
    bool getProduceMaterial() const {return !(flags & PRODUCE_MATERIAL);}

    /** Returns true is this operationplan is allowed to consume capacity.
      * This field only has an impact for locked operationplans.
      */
    bool getConsumeCapacity() const {return !(flags & CONSUME_CAPACITY);}

    /** Deletes all operationplans of a certain operation. A boolean flag
      * allows to specify whether locked operationplans are to be deleted too.
      */
    static DECLARE_EXPORT void deleteOperationPlans(Operation* o, bool deleteLocked=false);

    /** Locks/unlocks an operationplan. A locked operationplan is never
      * changed.
      */
    virtual DECLARE_EXPORT void setLocked(bool b = true);

    /** Update flag which allow/disallows material consumption. */
    void setConsumeMaterial(bool b)
    {
      if (b) flags &= ~CONSUME_MATERIAL;
      else flags |= CONSUME_MATERIAL;
    }

    /** Update flag which allow/disallows material production. */
    void setProduceMaterial(bool b)
    {
      if (b) flags &= ~PRODUCE_MATERIAL;
      else flags |= PRODUCE_MATERIAL;
    }

    /** Update flag which allow/disallows capacity consumption. */
    void setConsumeCapacity(bool b)
    {
      if (b) flags &= ~CONSUME_CAPACITY;
      else flags |= CONSUME_CAPACITY;
    }

    /** Returns a pointer to the operation being instantiated. */
    Operation* getOperation() const {return oper;}

    /** Fixes the start and end date of an operationplan. Note that this
      * overrules the standard duration given on the operation, i.e. no logic
      * kicks in to verify the data makes sense. This is up to the user to
      * take care of.<br>
      * The methods setStart(Date) and setEnd(Date) are therefore preferred
      * since they properly apply all appropriate logic.
      */
    void setStartAndEnd(Date st, Date nd)
    {
      dates.setStartAndEnd(st,nd);
      update();
    }

    /** A method to restore a previous state of an operationplan.<br>
      * NO validity checks are done on the parameters.
      */
    void restore(const OperationPlanState& x);

    /** Updates the operationplan owning this operationplan.<br>
      * The optional extra argument specifies whether we need to complete
      * validation of the parent-child operation or not. Validation is
      * necessary to validate input from the user. But when the owner field
      * is set in the solver internally, we can skip validation to keep
      * performance high.
      * @see Operation::addSubOperationPlan
      * @see OperationAlternate::addSubOperationPlan
      * @see OperationRouting::addSubOperationPlan
      */
    void DECLARE_EXPORT setOwner(OperationPlan* o, bool=true);

    /** Returns a pointer to the operationplan for which this operationplan
      * a sub-operationplan.<br>
      * The method returns NULL if there is no owner defined.<br>
      * E.g. Sub-operationplans of a routing refer to the overall routing
      * operationplan.<br>
      * E.g. An alternate sub-operationplan refers to its parent.
      * @see getTopOwner
      */
    OperationPlan* getOwner() const {return owner;}

    /** Returns a pointer to the operationplan owning a set of
      * sub-operationplans. There can be multiple levels of suboperations.<br>
      * If no owner exists the method returns the current operationplan.
      * @see getOwner
      */
    const OperationPlan* getTopOwner() const
    {
      if (owner)
      {
        // There is an owner indeed
        OperationPlan* o(owner);
        while (o->owner) o = o->owner;
        return o;
      }
      else
        // This operationplan is itself the top of a hierarchy
        return this;
    }

    /** Returns the start and end date of this operationplan. */
    const DateRange & getDates() const {return dates;}

    /** Return true if the operationplan is redundant, ie all material
      * it produces is not used at all.<br>
      * If the optional argument is false (which is the default value), we
      * check with the minimum stock level of the buffers. If the argument
      * is true, we check with 0.
      */
    DECLARE_EXPORT bool isExcess(bool = false) const;

    /** Returns a unique identifier of the operationplan.<br>
      * The identifier can be specified in the data input (in which case
      * we check for the uniqueness during the read operation).<br>
      * For operationplans created during a solver run, the identifier is
      * assigned in the instantiate() function. The numbering starts with the
      * highest identifier read in from the input and is then incremented
      * for every operationplan that is registered.
      */
    DECLARE_EXPORT unsigned long getIdentifier()
    {
      if (id==ULONG_MAX) assignIdentifier(); // Lazy generation
      return id;
    }

    /** Return the identifier. This method can return the lazy identifier 1. */
    unsigned long getRawIdentifier() const {return id;}

    /** Updates the end date of the operationplan and compute the start
      * date.<br>
      * Locked operationplans are not updated by this function.<br>
      * Slack can be introduced between sub operationaplans by this method,
      * i.e. the sub operationplans are only moved if required to meet the
      * end date.
      */
    virtual DECLARE_EXPORT void setEnd(Date);

    /** Updates the start date of the operationplan and compute the end
      * date.<br>
      * Locked operation_plans are not updated by this function.<br>
      * Slack can be introduced between sub operationaplans by this method,
      * i.e. the sub operationplans are only moved if required to meet the
      * start date.
      */
    virtual DECLARE_EXPORT void setStart(Date);

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    PyObject* str() const
    {
      ostringstream ch;
      ch << id;
      return PythonObject(ch.str());
    }

    /** Python factory method. */
    static PyObject* create(PyTypeObject*, PyObject*, PyObject*);

    /** Initialize the operationplan. The initialization function should be
      * called when the operationplan is ready to be 'officially' added. The
      * initialization performs the following actions:
      * <ol>
      * <li> assign an identifier</li>
      * <li> create the flow and loadplans if these hadn't been created
      * before</li>
      * <li> add the operationplan to the global list of operationplans</li>
      * <li> create a link with a demand object if this is a delivery
      * operationplan</li>
      * </ol>
      * Every operationplan subclass that has sub-operations will normally
      * need to create an override of this function.<br>
      *
      * The return value indicates whether the initialization was successfull.
      * If the operationplan is invalid, it will be DELETED and the return value
      * is 'false'.
      */
    DECLARE_EXPORT bool activate();

    /** Remove an operationplan from the list of officially registered ones.<br>
      * The operationplan will keep its loadplans and flowplans after unregistration.
      */
    DECLARE_EXPORT void deactivate();

    /** This method links the operationplan in the list of all operationplans
      * maintained on the operation.<br>
      * In most cases calling this method is not required since it included
      * in the activate method. In exceptional cases the solver already
      * needs to see uncommitted operationplans in the list - eg for the
      * procurement buffer.
      * @see activate
      */
    DECLARE_EXPORT void insertInOperationplanList();

    /** This method remove the operationplan from the list of all operationplans
      * maintained on the operation.<br>
      * @see deactivate
      */
    DECLARE_EXPORT void removeFromOperationplanList();

    /** Remove a sub-operation_plan from the list. */
    virtual DECLARE_EXPORT void eraseSubOperationPlan(OperationPlan*);

    /** This function is used to create the loadplans, flowplans and
      * setup operationplans.
      */
    DECLARE_EXPORT void createFlowLoads();

    /** This function is used to delete the loadplans, flowplans and
      * setup operationplans.
      */
    DECLARE_EXPORT void deleteFlowLoads();

    bool getHidden() const {return getOperation()->getHidden();}

    /** Searches for an OperationPlan with a given identifier.<br>
      * Returns a NULL pointer if no such OperationPlan can be found.<br>
      * The method is of complexity O(n), i.e. involves a LINEAR search through
      * the existing operationplans, and can thus be quite slow in big models.<br>
      * The method is O(1), i.e. constant time regardless of the model size,
      * when the parameter passed is bigger than the operationplan counter.
      */
    static DECLARE_EXPORT OperationPlan* findId(unsigned long l);

    /** Problem detection is actually done by the Operation class. That class
      * actually "delegates" the responsability to this class, for efficiency.
      */
    virtual void updateProblems();

    /** Implement the pure virtual function from the HasProblem class. */
    Plannable* getEntity() const {return oper;}

    /** Return the metadata. We return the metadata of the operation class,
      * not the one of the operationplan class!
      */
    const MetaClass& getType() const {return *metadata;}

    static DECLARE_EXPORT const MetaClass* metadata;

    static DECLARE_EXPORT const MetaCategory* metacategory;

    virtual size_t getSize() const
    {return sizeof(OperationPlan) + getSource().size();}

    /** Handles the persistence of operationplan objects. */
    static DECLARE_EXPORT void writer(const MetaCategory*, XMLOutput*);

    /** Comparison of 2 OperationPlans.
      * To garantuee that the problems are sorted in a consistent and stable
      * way, the following sorting criteria are used (in order of priority):
      * <ol><li>Operation</li>
      * <li>Start date (earliest dates first)</li>
      * <li>Quantity (biggest quantities first)</li></ol>
      * Multiple operationplans for the same values of the above keys can exist.
      */
    DECLARE_EXPORT bool operator < (const OperationPlan& a) const;

    /** Copy constructor.<br>
      * If the optional argument is false, the new copy is not initialized
      * and won't have flowplans and loadplans.
      */
    DECLARE_EXPORT OperationPlan(const OperationPlan&, bool = true);

    /** Return the plannable object that caused the creation of this
      * operationplan. Usage of this field can vary by solver.
      * The information is normally not relevant for end users.
      */
    DECLARE_EXPORT Plannable* getMotive() const {return motive;}

    /** Update the plannable object that created this operationplan. */
    DECLARE_EXPORT void setMotive(Plannable* v) {motive = v;}

  private:
    /** Private copy constructor.<br>
      * It is used in the public copy constructor to make a deep clone of suboperationplans.
      * @see OperationPlan(const OperationPlan&, bool = true)
      */
    DECLARE_EXPORT OperationPlan(const OperationPlan&, OperationPlan*);

    /** Updates the operationplan based on the latest information of quantity,
      * date and locked flag.<br>
      * This method will also update parent and child operationplans.
      * @see resizeFlowLoadPlans
      */
    virtual DECLARE_EXPORT void update();

    /** Generates a unique identifier for the operationplan. */
    DECLARE_EXPORT bool assignIdentifier();

    /** Update the loadplans and flowplans of the operationplan based on the
      * latest information of quantity, date and locked flag.<br>
      * This method will NOT update parent or child operationplans.
      * @see update
      */
    DECLARE_EXPORT void resizeFlowLoadPlans();

    /** Default constructor.<br>
      * This way of creating operationplan objects is not intended for use by
      * any client applications. Client applications should use the factory
      * method on the operation class instead.<br>
      * Subclasses of the Operation class may use this constructor in their
      * own override of the createOperationPlan method.
      * @see Operation::createOperationPlan
      */
    OperationPlan() : owner(NULL), quantity(0.0), flags(0), dmd(NULL),
      id(0), oper(NULL), firstflowplan(NULL), firstloadplan(NULL),
      prev(NULL), next(NULL), firstsubopplan(NULL), lastsubopplan(NULL),
      nextsubopplan(NULL), prevsubopplan(NULL), motive(NULL)
    {initType(metadata);}

  private:
    static const short IS_LOCKED = 1;    // 0: no, 1: yes
    static const short IS_SETUP = 2;     // 0: no, 1: yes
    static const short HAS_SETUP = 4;    // 0: no, 1: yes
    static const short CONSUME_MATERIAL = 8;  // 0: yes, 1: no
    static const short PRODUCE_MATERIAL = 16; // 0: yes, 1: no
    static const short CONSUME_CAPACITY = 32; // 0: yes, 1: no

    /** Pointer to a higher level OperationPlan. */
    OperationPlan *owner;

    /** Quantity. */
    double quantity;

    /** Is this operationplan locked? A locked operationplan doesn't accept
      * any changes. This field is only relevant for top-operationplans. */
    short flags;

    /** Counter of OperationPlans, which is used to automatically assign a
      * unique identifier for each operationplan.<br>
      * The value of the counter is the first available identifier value that
      * can be used for a new operationplan.<br>
      * The first value is 1, and each operationplan increases it by 1.
      * @see assignIdentifier()
      */
    static DECLARE_EXPORT unsigned long counterMin;

    /** Pointer to the demand.<br>
      * Only delivery operationplans have this field set. The field is NULL
      * for all other operationplans.
      */
    Demand *dmd;

    /** Unique identifier.<br>
      * The field is 0 while the operationplan is not fully registered yet.
      * The field is 1 when the operationplan is fully registered but only a
      * temporary id is generated.
      * A unique value for each operationplan is created lazily when the
      * method getIdentifier() is called.
      */
    unsigned long id;

    /** Start and end date. */
    DateRange dates;

    /** Pointer to the operation. */
    Operation *oper;

    /** Root of a single linked list of flowplans. */
    FlowPlan* firstflowplan;

    /** Single linked list of loadplans. */
    LoadPlan* firstloadplan;

    /** Pointer to the previous operationplan.<br>
      * Operationplans are chained in a doubly linked list for each operation.
      * @see next
      */
    OperationPlan* prev;

    /** Pointer to the next operationplan.<br>
      * Operationplans are chained in a doubly linked list for each operation.
      * @see prev
      */
    OperationPlan* next;

    /** Pointer to the first suboperationplan of this operationplan. */
    OperationPlan* firstsubopplan;

    /** Pointer to the last suboperationplan of this operationplan. */
    OperationPlan* lastsubopplan;

    /** Pointer to the next suboperationplan of the parent operationplan. */
    OperationPlan* nextsubopplan;

    /** Pointer to the previous suboperationplan of the parent operationplan. */
    OperationPlan* prevsubopplan;

    /** Pointer to the demand that caused the creation of this operationplan. */
    Plannable* motive;
};


/** @brief A simple class to easily remember the date, quantity and owner of
  * an operationplan. */
class OperationPlanState  // @todo should also be able to remember and restore suboperationplans!!!
{
  public:
    Date start;
    Date end;
    double quantity;

    /** Default constructor. */
    OperationPlanState() : quantity(0.0) {}

    /** Constructor. */
    OperationPlanState(const OperationPlan* x)
    {
      if (!x)
      {
        quantity = 0.0;
        return;
      }
      else
      {
        start = x->getDates().getStart();
        end = x->getDates().getEnd();
        quantity = x->getQuantity();
      }
    }

    /** Constructor. */
    OperationPlanState(const Date x, const Date y, double q)
      : start(x), end(y), quantity(q) {}

    /** Constructor. */
    OperationPlanState(const DateRange& x, double q)
      : start(x.getStart()), end(x.getEnd()), quantity(q) {}
};


/** @brief Models an operation that takes a fixed amount of time, independent
  * of the quantity. */
class OperationFixedTime : public Operation
{
  public:
    /** Constructor. */
    explicit OperationFixedTime(const string& s) : Operation(s) {initType(metadata);}

    /** Returns the length of the operation. */
    const TimePeriod getDuration() const {return duration;}

    /** Updates the duration of the operation. Existing operation plans of this
      * operation are not automatically refreshed to reflect the change. */
    void setDuration(TimePeriod t)
    {
      if (t<0L)
        throw DataException("FixedTime operation can't have a negative duration");
      duration = t;
    }

    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(OperationFixedTime) + Operation::extrasize();}

    /** A operation of this type enforces the following rules on its
      * operationplans:
      *  - The duration is always constant.
      *  - If the end date is specified, we use that and ignore the start
      *    date that could have been passed.
      *  - If no end date but only a start date are specified, we'll use
      *    that date.
      *  - If no dates are specified, we don't update the dates of the
      *    operationplan.
      *  - The quantity can be any positive number.
      *  - Locked operationplans can't be updated.
      * @see Operation::setOperationPlanParameters
      */
    DECLARE_EXPORT OperationPlanState setOperationPlanParameters
    (OperationPlan*, double, Date, Date, bool=true, bool=true) const;

  protected:
    DECLARE_EXPORT virtual bool extraInstantiate(OperationPlan* o);

  private:
    /** Stores the lengh of the Operation. */
    TimePeriod duration;
};


/** @brief Models an operation to convert a setup on a resource. */
class OperationSetup : public Operation
{
  public:
    /** Constructor. */
    explicit OperationSetup(const string& s) : Operation(s) {initType(metadata);}

    // Never write the setup operation
    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const {}
    static int initialize();

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(OperationSetup) + Operation::extrasize();}

    /** A operation of this type enforces the following rules on its
      * operationplans:
      *  - The duration is calculated based on the conversion type.
      */
    DECLARE_EXPORT OperationPlanState setOperationPlanParameters
    (OperationPlan*, double, Date, Date, bool=true, bool=true) const;

    /** A pointer to the operation that is instantiated for all conversions. */
    static DECLARE_EXPORT const Operation* setupoperation;
};


/** @brief Models an operation whose duration is the sum of a constant time,
  * plus a cetain time per unit.
  */
class OperationTimePer : public Operation
{
  public:
    /** Constructor. */
    explicit OperationTimePer(const string& s) : Operation(s), duration_per(0.0) {initType(metadata);}

    /** Returns the constant part of the operation time. */
    TimePeriod getDuration() const {return duration;}

    /** Sets the constant part of the operation time. */
    void setDuration(TimePeriod t)
    {
      if(t<0L)
        throw DataException("TimePer operation can't have a negative duration");
      duration = t;
    }

    /** Returns the time per unit of the operation time. */
    double getDurationPer() const {return duration_per;}

    /** Sets the time per unit of the operation time. */
    void setDurationPer(double t)
    {
      if(t<0.0)
        throw DataException("TimePer operation can't have a negative duration-per");
      duration_per = t;
    }

    /** A operation of this type enforces the following rules on its
      * operationplans:
      *   - If both the start and end date are specified, the quantity is
      *     computed to match these dates.
      *     If the time difference between the start and end date is too
      *     small to fit the fixed duration, the quantity is set to 0.
      *   - If only an end date is specified, it will be respected and we
      *     compute a start date based on the quantity.
      *   - If only a start date is specified, it will be respected and we
      *     compute an end date based on the quantity.
      *   - If no date is specified, we respect the quantity and the end
      *     date of the operation. A new start date is being computed.
      * @see Operation::setOperationPlanParameters
      */
    DECLARE_EXPORT OperationPlanState setOperationPlanParameters
    (OperationPlan*, double, Date, Date, bool=true, bool=true) const;

    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(OperationTimePer) + Operation::extrasize();}

  private:
    /** Constant part of the operation time. */
    TimePeriod duration;

    /** Variable part of the operation time.
      * We store the value as a double value rather than a TimePeriod to
      * be able to store fractional duration_per value. TimePeriod can only
      * represent seconds.
      */
    double duration_per;
};


/** @brief Represents a routing operation, i.e. an operation consisting of
  * multiple, sequential sub-operations.
  */
class OperationRouting : public Operation
{
  public:
    /** Constructor. */
    explicit OperationRouting(const string& c) : Operation(c) {initType(metadata);}

    /** Destructor. */
    DECLARE_EXPORT ~OperationRouting();

    /** Adds a new steps to routing at the start of the routing. */
    void addStepFront(Operation *o)
    {
      if (!o) throw DataException("Adding NULL operation to routing");
      steps.push_front(o);
      o->addSuperOperation(this);
    }

    /** Adds a new steps to routing at the end of the routing. */
    void addStepBack(Operation *o)
    {
      if (!o) throw DataException("Adding NULL operation to routing");
      steps.push_back(o);
      o->addSuperOperation(this);
    }

    /** Add one or more steps to a routing. */
    static DECLARE_EXPORT PyObject* addStep(PyObject*, PyObject*);

    /** Remove a step from a routing. */
    void removeSubOperation(Operation *o)
    {steps.remove(o); o->superoplist.remove(this);}

    /** A operation of this type enforces the following rules on its
      * operationplans:
      *  - If an end date is given, sequentially use this method on the
      *    different steps. The steps are stepped through starting from the
      *    last step, and each step will adjust to meet the requested end date.
      *    If there is slack between the routings' step operationplans, it can
      *    be used to "absorb" the change.
      *  - When a start date is given, the behavior is similar to the previous
      *    case, except that we step through the operationplans from the
      *    first step this time.
      *  - If both a start and an end date are given, we use only the end date.
      *  - If there are no sub operationplans yet, apply the requested changes
      *    blindly.
      * @see Operation::setOperationPlanParameters
      */
    DECLARE_EXPORT OperationPlanState setOperationPlanParameters
    (OperationPlan*, double, Date, Date, bool=true, bool=true) const;

    DECLARE_EXPORT double setOperationPlanQuantity
      (OperationPlan* oplan, double f, bool roundDown, bool upd, bool execute) const;

    /** Add a new child operationplan.
      * A routing operationplan has a series of suboperationplans:
      *   - A setup operationplan if the routing operation loads a resource
      *     which requires a specific setup.
      *   - A number of unlocked operationplans (one for each step in the
      *     routing) representing production not yet started.
      *   - A number of locked operationplan (one for each step in the routing)
      *     representing production which is already started or finished.
      * The sum of the quantity of the locked and unlocked operationplans of
      * each step should be equal to the quantity of top routing operationplan.<br>
      * The fast insert does insertion at the front of the unlocked operationplans.
      */
    virtual DECLARE_EXPORT void addSubOperationPlan(
      OperationPlan*, OperationPlan*, bool = true
      );

    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    static int initialize();

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    /** Return a list of all sub-operations. */
    virtual const Operationlist& getSubOperations() const {return steps;}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {
      return sizeof(OperationRouting) + Operation::extrasize()
          + steps.size() * 2 * sizeof(Operation*);
    }

  protected:
    /** Extra logic to be used when instantiating an operationplan. */
    virtual DECLARE_EXPORT bool extraInstantiate(OperationPlan* o);

  private:
    /** Stores a double linked list of all step operations. */
    Operationlist steps;
};


inline void OperationPlan::restore(const OperationPlanState& x)
{
  getOperation()->setOperationPlanParameters(this, x.quantity, x.start, x.end, true);
  if (quantity != x.quantity) quantity = x.quantity;
  assert(dates.getStart() == x.start || x.start!=x.end);
  assert(dates.getEnd() == x.end || x.start!=x.end);
}


/** This type defines what mode used to search the alternates. */
enum SearchMode
{
  /** Select the alternate with the lowest priority number.<br>
    * This is the default.
    */
  PRIORITY = 0,
  /** Select the alternate which gives the lowest cost. */
  MINCOST = 1,
  /** Select the alternate which gives the lowest penalty. */
  MINPENALTY = 2,
  /** Select the alternate which gives the lowest sum of the cost and
    * penalty. */
  MINCOSTPENALTY = 3
};


/** Writes a search mode to an output stream. */
inline ostream & operator << (ostream & os, const SearchMode & d)
{
  switch (d)
  {
    case PRIORITY: os << "PRIORITY"; return os;
    case MINCOST: os << "MINCOST"; return os;
    case MINPENALTY: os << "MINPENALTY"; return os;
    case MINCOSTPENALTY: os << "MINCOSTPENALTY"; return os;
    default: assert(false); return os;
  }
}


/** Translate a string to a search mode value. */
DECLARE_EXPORT SearchMode decodeSearchMode(const string& c);


/** @brief This class represents a choice between multiple operations. The
  * alternates are sorted in order of priority.
  */
class OperationAlternate : public Operation
{
  public:
    typedef pair<int,DateRange> alternateProperty;

    /** Constructor. */
    explicit OperationAlternate(const string& c)
      : Operation(c), search(PRIORITY) {initType(metadata);}

    /** Destructor. */
    DECLARE_EXPORT ~OperationAlternate();

    /** Add a new alternate operation.<br>
      * The lower the priority value, the more important this alternate
      * operation is. */
    DECLARE_EXPORT void addAlternate
    (Operation*, int = 1, DateRange = DateRange());

    /** Removes an alternate from the list. */
    DECLARE_EXPORT void removeSubOperation(Operation *);

    /** Returns the properties of a certain suboperation.
      * @exception LogicException Generated when the argument operation is
      *     null or when it is not a sub-operation of this alternate.
      */
    DECLARE_EXPORT const alternateProperty& getProperties(Operation* o) const;

    /** Updates the priority of a certain suboperation.
      * @exception DataException Generated when the argument operation is
      *     not null and not a sub-operation of this alternate.
      */
    DECLARE_EXPORT void setPriority(Operation*, int);

    /** Updates the effective daterange of a certain suboperation.
      * @exception DataException Generated when the argument operation is
      *     not null and not a sub-operation of this alternate.
      */
    DECLARE_EXPORT void setEffective(Operation*, DateRange);

    /** Return the search mode. */
    SearchMode getSearch() const {return search;}

    /** Update the search mode. */
    void setSearch(const string a) {search = decodeSearchMode(a);}

    /** A operation of this type enforces the following rules on its
      * operationplans:
      *  - Very simple, call the method with the same name on the alternate
      *    suboperationplan.
      * @see Operation::setOperationPlanParameters
      */
    DECLARE_EXPORT OperationPlanState setOperationPlanParameters
    (OperationPlan*, double, Date, Date, bool=true, bool=true) const;

    /** Add a new child operationplan.
      * An alternate operationplan plan can have a maximum of 2
      * suboperationplans:
      *  - A setup operationplan if the alternate top-operation loads a
      *    resource requiring a specific setup.
      *  - An operationplan of any of the allowed suboperations.
      */
    virtual DECLARE_EXPORT void addSubOperationPlan(
      OperationPlan*, OperationPlan*, bool=true
      );

    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}
    virtual const Operationlist& getSubOperations() const {return alternates;}
    static int initialize();

    /** Add an alternate to the operation.<br>
      * The keyword arguments are "operation", "priority", "effective_start"
      * and "effective_end"
      */
    static DECLARE_EXPORT PyObject* addAlternate(PyObject*, PyObject*, PyObject*);

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {
      return sizeof(OperationAlternate) + Operation::extrasize()
          + alternates.size() * (5*sizeof(Operation*)+sizeof(alternateProperty));
    }

  protected:
    /** Extra logic to be used when instantiating an operationplan. */
    virtual DECLARE_EXPORT bool extraInstantiate(OperationPlan* o);

  private:
    typedef list<alternateProperty> alternatePropertyList;

    /** List of the priorities of the different alternate operations. The list
      * is maintained such that it is sorted in ascending order of priority. */
    alternatePropertyList alternateProperties;

    /** List of all alternate operations. The list is sorted with the operation
      * with the highest priority at the start of the list.<br>
      * Note that the list of operations and the list of priorities go hand in
      * hand: they have an equal number of elements and the order of the
      * elements is matching in both lists.
      */
    Operationlist alternates;

    /** Mode to select the preferred alternates. */
    SearchMode search;
};


/** @brief An item defines the products being planned, sold, stored and/or
  * manufactured. Buffers and demands have a reference an item.
  *
  * This is an abstract class.
  */
class Item : public HasHierarchy<Item>, public HasDescription
{
  public:
    /** Constructor. Don't use this directly! */
    explicit DECLARE_EXPORT Item(const string& str) :
      HasHierarchy<Item>(str), deliveryOperation(NULL), price(0.0) {}

    /** Returns the delivery operation.<br>
      * This field is inherited from a parent item, if it hasn't been
      * specified.
      */
    Operation* getOperation() const
    {
      // Current item has a non-empty deliveryOperation field
      if (deliveryOperation) return deliveryOperation;

      // Look for a non-empty deliveryOperation field on owners
      for (Item* i = getOwner(); i; i=i->getOwner())
        if (i->deliveryOperation) return i->deliveryOperation;

      // The field is not specified on the item or any of its parents.
      return NULL;
    }

    /** Updates the delivery operation.<br>
      * If some demands have already been planned using the old delivery
      * operation they are left untouched and won't be replanned.
      */
    void setOperation(Operation* o) {deliveryOperation = o;}

    /** Return the selling price of the item.<br>
      * The default value is 0.0.
      */
    double getPrice() const {return price;}

    /** Update the selling price of the item. */
    void setPrice(const double c)
    {
      if (c >= 0) price = c;
      else throw DataException("Item price must be positive");
    }

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    /** Destructor. */
    virtual DECLARE_EXPORT ~Item();

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

  private:
    /** This is the operation used to satisfy a demand for this item.
      * @see Demand
      */
    Operation* deliveryOperation;

    /** Selling price of the item. */
    double price;
};


/** @brief This class is the default implementation of the abstract Item
  * class. */
class ItemDefault : public Item
{
  public:
    explicit ItemDefault(const string& str) : Item(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {
      return sizeof(ItemDefault) + getName().size()
          + HasDescription::extrasize();
    }
    static int initialize();
};


/** @brief A buffer represents a combination of a item and location.<br>
  * It is the entity for keeping modeling inventory.
  */
class Buffer : public HasHierarchy<Buffer>, public HasLevel,
  public Plannable, public HasDescription
{
    friend class Flow;
    friend class FlowPlan;

  public:
    typedef TimeLine<FlowPlan> flowplanlist;
    typedef Association<Operation,Buffer,Flow>::ListB flowlist;

    /** Constructor. Implicit creation of instances is disallowed. */
    explicit DECLARE_EXPORT Buffer(const string& str) :
      HasHierarchy<Buffer>(str), hidden(false), producing_operation(NULL),
      loc(NULL), it(NULL), min_val(0), max_val(default_max), min_cal(NULL),
      max_cal(NULL), carrying_cost(0.0) {}

    /** Returns the operation that is used to supply extra supply into this
      * buffer. */
    Operation* getProducingOperation() const {return producing_operation;}

    /** Updates the operation that is used to supply extra supply into this
      * buffer. */
    void setProducingOperation(Operation* o)
    {producing_operation = o; setChanged();}

    /** Returns the item stored in this buffer. */
    Item* getItem() const {return it;}

    /** Updates the Item stored in this buffer. */
    void setItem(Item* i) {it = i; setChanged();}

    /** Returns the Location of this buffer. */
    Location* getLocation() const {return loc;}

    /** Updates the location of this buffer. */
    void setLocation(Location* i) {loc = i;}

    /** Returns the minimum inventory level. */
    double getMinimum() const {return min_val;}

    /** Returns a pointer to a calendar for storing the minimum inventory
      * level. */
    CalendarDouble* getMinimumCalendar() const {return min_cal;}

    /** Returns the maximum inventory level. */
    double getMaximum() const {return max_val;}

    /** Returns a pointer to a calendar for storing the maximum inventory
      * level. */
    CalendarDouble* getMaximumCalendar() const {return max_cal;}

    /** Updates the minimum inventory target for the buffer. */
    DECLARE_EXPORT void setMinimum(double);

    /** Updates the minimum inventory target for the buffer. */
    DECLARE_EXPORT void setMinimumCalendar(CalendarDouble *);

    /** Updates the minimum inventory target for the buffer. */
    DECLARE_EXPORT void setMaximum(double);

    /** Updates the minimum inventory target for the buffer. */
    DECLARE_EXPORT void setMaximumCalendar(CalendarDouble *);

    /** Return the carrying cost.<br>
      * The cost of carrying inventory in this buffer. The value is a
      * percentage of the item sales price, per year and per unit.
      */
    double getCarryingCost() const {return carrying_cost;}

    /** Return the carrying cost.<br>
      * The cost of carrying inventory in this buffer. The value is a
      * percentage of the item sales price, per year and per unit.<br>
      * The default value is 0.0.
      */
    void setCarryingCost(const double c)
    {
      if (c >= 0) carrying_cost = c;
      else throw DataException("Buffer carrying_cost must be positive");
    }

    DECLARE_EXPORT virtual void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT virtual void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT virtual void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);

    size_t extrasize() const
    {return getName().size() + HasDescription::extrasize();}

    /** Initialize the class. */
    static int initialize();

    /** Destructor. */
    virtual DECLARE_EXPORT ~Buffer();

    /** Returns the available material on hand immediately after the
      * given date.
      */
    DECLARE_EXPORT double getOnHand(Date d = Date::infinitePast) const;

    /** Update the on-hand inventory at the start of the planning horizon. */
    DECLARE_EXPORT void setOnHand(double f);

    /** Returns minimum or maximum available material on hand in the given
      * daterange. The third parameter specifies whether we return the
      * minimum (which is the default) or the maximum value.
      * The computation is INclusive the start and end dates.
      */
    DECLARE_EXPORT double getOnHand(Date, Date, bool min = true) const;

    /** Returns a reference to the list of all flows of this buffer. */
    const flowlist& getFlows() const {return flows;}

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    /** Returns a reference to the list of all flow plans of this buffer. */
    const flowplanlist& getFlowPlans() const {return flowplans;}

    /** Returns a reference to the list of all flow plans of this buffer. */
    flowplanlist& getFlowPlans() {return flowplans;}

    /** Return the flow that is associates a given operation with this
      * buffer.<br>Returns NULL is no such flow exists. */
    Flow* findFlow(const Operation* o, Date d) const
    {return flows.find(o,d);}

    /** Deletes all operationplans consuming from or producing from this
      * buffer. The boolean parameter controls whether we delete also locked
      * operationplans or not.
      */
    DECLARE_EXPORT void deleteOperationPlans(bool deleteLockedOpplans = false);

    virtual DECLARE_EXPORT void updateProblems();

    void setHidden(bool b) {if (hidden!=b) setChanged(); hidden = b;}
    bool getHidden() const {return hidden;}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

    /** This function matches producing and consuming operationplans
      * with each other, and updates the pegging iterator accordingly.
      */
    virtual DECLARE_EXPORT void followPegging
    (PeggingIterator&, FlowPlan*, short, double, double);

    /** Clean up excess producing operationplans from the buffer. */
    virtual void removeExcess(vector<Buffer*>* buflist = NULL, CommandManager* = NULL) {}

  private:
    /** A constant defining the default max inventory target.\\
      * Theoretically we should set this to DBL_MAX, but then the results
      * are not portable across platforms.
      */
    static DECLARE_EXPORT const double default_max;

    /** This models the dynamic part of the plan, representing all planned
      * material flows on this buffer. */
    flowplanlist flowplans;

    /** This models the defined material flows on this buffer. */
    flowlist flows;

    /** Hide this entity from serialization or not. */
    bool hidden;

    /** This is the operation used to create extra material in this buffer. */
    Operation *producing_operation;

    /** Location of this buffer.<br>
      * This field is only used as information.<br>
      * The default is NULL.
      */
    Location* loc;

    /** Item being stored in this buffer.<br>
      * The default value is NULL.
      */
    Item* it;

    /** Minimum inventory target.<br>
      * If a minimum calendar is specified this field is ignored.
      * @see min_cal
      */
    double min_val;

    /** Maximum inventory target. <br>
      * If a maximum calendar is specified this field is ignored.
      * @see max_cal
      */
    double max_val;

    /** Points to a calendar to store the minimum inventory level.<br>
      * The default value is NULL, resulting in a constant minimum level
      * of 0.
      */
    CalendarDouble *min_cal;

    /** Points to a calendar to store the maximum inventory level.<br>
      * The default value is NULL, resulting in a buffer without excess
      * inventory problems.
      */
    CalendarDouble *max_cal;

    /** Carrying cost.<br>
      * The cost of carrying inventory in this buffer. The value is a
      * percentage of the item sales price, per year and per unit.
      */
    double carrying_cost;
};



/** @brief This class is the default implementation of the abstract Buffer class. */
class BufferDefault : public Buffer
{
  public:
    explicit BufferDefault(const string& str) : Buffer(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    virtual size_t getSize() const
    {return sizeof(BufferDefault) + Buffer::extrasize();}
    static DECLARE_EXPORT const MetaClass* metadata;
    static int initialize();
    
    /** Clean up excess producing operationplans from the buffer. */
    virtual DECLARE_EXPORT void removeExcess(vector<Buffer*>* buflist = NULL, CommandManager* = NULL);
};


/** @brief  This class represents a material buffer with an infinite supply of extra
  * material.
  *
  * In other words, it never constrains the plan and it doesn't propagate any
  * requirements upstream.
  */
class BufferInfinite : public Buffer
{
  public:
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual const MetaClass& getType() const {return *metadata;}
    virtual size_t getSize() const
    {return sizeof(BufferInfinite) + Buffer::extrasize();}
    explicit BufferInfinite(const string& c) : Buffer(c)
    {setDetectProblems(false); initType(metadata);}
    static DECLARE_EXPORT const MetaClass* metadata;
    static int initialize();
};


/** @brief This class models a buffer that is replenish by an external supplier
  * using a reorder-point policy.
  *
  * It represents a material buffer where a replenishment is triggered
  * whenever the inventory drops below the minimum level. The buffer is then
  * replenished to the maximum inventory level.<br>
  * A leadtime is taken into account for the replenishments.<br>
  * The following parameters control this replenishment:
  *  - <b>MinimumInventory</b>:<br>
  *    Inventory level triggering a new replenishment.<br>
  *    The actual inventory can drop below this value.
  *  - <b>MaximumInventory</b>:<br>
  *    Inventory level to which we try to replenish.<br>
  *    The actual inventory can exceed this value.
  *  - <b>Leadtime</b>:<br>
  *    Time taken between placing the purchase order with the supplier and the
  *    delivery of the material.
  *
  * Using the additional parameters described below the replenishments can be
  * controlled in more detail. The resulting inventory profile can end up
  * to be completely different from the classical saw-tooth pattern!
  *
  * The timing of the replenishments can be constrained by the following
  * parameters:
  *  - <b>MinimumInterval</b>:<br>
  *    Minimum time between replenishments.<br>
  *    The order quantity will be increased such that it covers at least
  *    the demand in the minimum interval period. The actual inventory can
  *    exceed the target set by the MinimumInventory parameter.
  *  - <b>MaximumInterval</b>:<br>
  *    Maximum time between replenishments.<br>
  *    The order quantity will replenish to an inventory value less than the
  *    maximum when this maximum interval is reached.
  * When the minimum and maximum interval are equal we basically define a fixed
  * schedule replenishment policy.
  *
  * The quantity of the replenishments can be constrained by the following
  * parameters:
  *  - <b>MinimumQuantity</b>:<br>
  *    Minimum quantity for a replenishment.<br>
  *    This parameter can cause the actual inventory to exceed the target set
  *    by the MinimumInventory parameter.
  *  - <b>MaximumQuantity</b>:<br>
  *    Maximum quantity for a replenishment.<br>
  *    This parameter can cause the maximum inventory target never to be
  *    reached.
  *  - <b>MultipleQuantity</b>:<br>
  *    All replenishments are rounded up to a multiple of this value.
  * When the minimum and maximum quantity are equal we basically define a fixed
  * quantity replenishment policy.
  */
class BufferProcure : public Buffer
{
  public:
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}
    virtual DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual const MetaClass& getType() const {return *metadata;}
    virtual size_t getSize() const
    {return sizeof(BufferProcure) + Buffer::extrasize();}
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    /** Constructor. */
    explicit BufferProcure(const string& c) : Buffer(c),
      size_minimum(0), size_maximum(DBL_MAX), size_multiple(0),
      oper(NULL) {initType(metadata);}
    static DECLARE_EXPORT const MetaClass* metadata;

    /** Return the purchasing leadtime. */
    TimePeriod getLeadtime() const {return leadtime;}

    /** Update the procurement leadtime. */
    void setLeadtime(TimePeriod p)
    {
      if (p<0L)
        throw DataException("Procurement buffer can't have a negative lead time");
      leadtime = p;
      static_cast<OperationFixedTime*>(getOperation())->setDuration(leadtime);
    }

    /** Return the release time fence. */
    TimePeriod getFence() const {return fence;}

    /** Update the release time fence. */
    void setFence(TimePeriod p)
    {
      fence = p;
      getOperation()->setFence(p);
    }

    /** Return the inventory level that will trigger creation of a
      * purchasing.
      */
    double getMinimumInventory() const
    {return getFlowPlans().getMin(Date::infiniteFuture);}

    /** Update the inventory level that will trigger the creation of a
      * replenishment.<br>
      * Because of the replenishment leadtime, the actual inventory will drop
      * below this value. It is up to the user to set an appropriate minimum
      * value.
      */
    void setMinimumInventory(double f)
    {
      if (f<0)
        throw DataException("Procurement buffer can't have a negative minimum inventory");
      flowplanlist::EventMinQuantity* min = getFlowPlans().getMinEvent(Date::infiniteFuture);
      if (min)
        min->setMin(f);
      else
      {
        // Create and insert a new minimum event
        min = new flowplanlist::EventMinQuantity(Date::infinitePast, f);
        getFlowPlans().insert(min);
      }
      // The minimum is increased over the maximum: auto-increase the maximum.
      if (getFlowPlans().getMax(Date::infiniteFuture) < f)
        setMaximumInventory(f);
    }

    /** Return the maximum inventory level to which we wish to replenish. */
    double getMaximumInventory() const
    {return getFlowPlans().getMax(Date::infiniteFuture);}

    /** Update the maximum inventory level to which we plan to replenish.<br>
      * This is not a hard limit - other parameters can make that the actual
      * inventory either never reaches this value or always exceeds it.
      */
    void setMaximumInventory(double f)
    {
      if (f<0)
        throw DataException("Procurement buffer can't have a negative maximum inventory");
      flowplanlist::EventMaxQuantity* max = getFlowPlans().getMaxEvent(Date::infiniteFuture);
      if (max)
        max->setMax(f);
      else
      {
        // Create and insert a new maximum event
        max = new flowplanlist::EventMaxQuantity(Date::infinitePast, f);
        getFlowPlans().insert(max);
      }
      // The maximum is lowered below the minimum: auto-decrease the minimum
      if (f < getFlowPlans().getMin(Date::infiniteFuture))
        setMinimumInventory(f);
    }

    /** Return the minimum interval between purchasing operations.<br>
      * This parameter doesn't control the timing of the first purchasing
      * operation, but only to the subsequent ones.
      */
    TimePeriod getMinimumInterval() const {return min_interval;}

    /** Update the minimum time between replenishments. */
    void setMinimumInterval(TimePeriod p)
    {
      if (p<0L)
        throw DataException("Procurement buffer can't have a negative minimum interval");
      min_interval = p;
      // Minimum is increased over the maximum: auto-increase the maximum
      if (max_interval && max_interval < min_interval)
        max_interval = min_interval;
    }

    /** Return the maximum time interval between sytem-generated replenishment
      * operations.
      */
    TimePeriod getMaximumInterval() const {return max_interval;}

    /** Update the minimum time between replenishments. */
    void setMaximumInterval(TimePeriod p)
    {
      if (p<0L)
        throw DataException("Procurement buffer can't have a negative maximum interval");
      max_interval = p;
      // Maximum is lowered below the minimum: auto-decrease the minimum
      if (min_interval && max_interval < min_interval)
        min_interval = max_interval;
    }

    /** Return the minimum quantity of a purchasing operation. */
    double getSizeMinimum() const {return size_minimum;}

    /** Update the minimum replenishment quantity. */
    void setSizeMinimum(double f)
    {
      if (f<0)
        throw DataException("Procurement buffer can't have a negative minimum size");
      size_minimum = f;
      getOperation()->setSizeMinimum(f);
      // minimum is increased over the maximum: auto-increase the maximum
      if (size_maximum < size_minimum) size_maximum = size_minimum;
    }

    /** Return the maximum quantity of a purchasing operation. */
    double getSizeMaximum() const {return size_maximum;}

    /** Update the maximum replenishment quantity. */
    void setSizeMaximum(double f)
    {
      if (f<0)
        throw DataException("Procurement buffer can't have a negative maximum size");
      size_maximum = f;
      getOperation()->setSizeMaximum(f);
      // maximum is lowered below the minimum: auto-decrease the minimum
      if (size_maximum < size_minimum) size_minimum = size_maximum;
    }

    /** Return the multiple quantity of a purchasing operation. */
    double getSizeMultiple() const {return size_multiple;}

    /** Update the multiple quantity. */
    void setSizeMultiple(double f)
    {
      if (f<0)
        throw DataException("Procurement buffer can't have a negative multiple size");
      size_multiple = f;
      getOperation()->setSizeMultiple(f);
    }

    /** Returns the operation that is automatically created to represent the
      * procurements.
      */
    DECLARE_EXPORT Operation* getOperation() const;

  private:
    /** Purchasing leadtime.<br>
      * Within this leadtime fence no additional purchase orders can be generated.
      */
    TimePeriod leadtime;

    /** Time window from the current date in which all procurements are expected
      * to be released.
      */
    TimePeriod fence;

    /** Minimum time interval between purchasing operations. */
    TimePeriod min_interval;

    /** Maximum time interval between purchasing operations. */
    TimePeriod max_interval;

    /** Minimum purchasing quantity.<br>
      * The default value is 0, meaning no minimum.
      */
    double size_minimum;

    /** Maximum purchasing quantity.<br>
      * The default value is 0, meaning no maximum limit.
      */
    double size_maximum;

    /** Purchases are always rounded up to a multiple of this quantity.<br>
      * The default value is 0, meaning no multiple needs to be applied.
      */
    double size_multiple;

    /** A pointer to the procurement operation. */
    Operation* oper;
};


/** @brief This class defines a material flow to/from a buffer, linked with an
  * operation. This default implementation plans the material flow at the
  * start of the operation.
  */
class Flow : public Object, public Association<Operation,Buffer,Flow>::Node,
  public Solvable, public HasSource
{
  public:
    /** Destructor. */
    virtual DECLARE_EXPORT ~Flow();

    /** Constructor. */
    explicit Flow(Operation* o, Buffer* b, double q)
      : quantity(q), hasAlts(false), altFlow(NULL), search(PRIORITY)
    {
      setOperation(o);
      setBuffer(b);
      initType(metadata);
      try { validate(ADD); }
      catch (...)
      {
        if (getOperation()) getOperation()->flowdata.erase(this);
        if (getBuffer()) getBuffer()->flows.erase(this);
        resetReferenceCount();
        throw;
      }
    }

    /** Constructor. */
    explicit Flow(Operation* o, Buffer* b, double q, DateRange e)
      : quantity(q), hasAlts(false), altFlow(NULL), search(PRIORITY)
    {
      setOperation(o);
      setBuffer(b);
      setEffective(e);
      initType(metadata);
      try { validate(ADD); }
      catch (...)
      {
        if (getOperation()) getOperation()->flowdata.erase(this);
        if (getBuffer()) getBuffer()->flows.erase(this);
        resetReferenceCount();
        throw;
      }
    }

    /** Returns the operation. */
    Operation* getOperation() const {return getPtrA();}

    /** Updates the operation of this flow. This method can be called only ONCE
      * for each flow. In case that doesn't suit you, delete the existing flow
      * and create a new one.
      */
    void setOperation(Operation* o) {if (o) setPtrA(o,o->getFlows());}

    /** Returns true if this flow consumes material from the buffer. */
    bool isConsumer() const {return quantity < 0;}

    /** Returns true if this flow produces material into the buffer. */
    bool isProducer() const {return quantity >= 0;}

    /** Returns the material flow PER UNIT of the operationplan. */
    double getQuantity() const {return quantity;}

    /** Updates the material flow PER UNIT of the operationplan. Existing
      * flowplans are NOT updated to take the new quantity in effect. Only new
      * operationplans and updates to existing ones will use the new quantity
      * value.
      */
    void setQuantity(double f) {quantity = f;}

    /** Returns the buffer. */
    Buffer* getBuffer() const {return getPtrB();}

    /** Updates the buffer of this flow. This method can be called only ONCE
      * for each flow. In case that doesn't suit you, delete the existing flow
      * and create a new one.
      */
    void setBuffer(Buffer* b) {if (b) setPtrB(b,b->getFlows());}

    /** Returns true if there are alternates for this flow. */
    bool hasAlternates() const {return hasAlts;}

    /** Returns the flow of which this one is an alternate.<br>
      * NULL is return where there is none.
      */
    Flow* getAlternate() const {return altFlow;}

    /** Define the flow of which this one is an alternate. */
    DECLARE_EXPORT void setAlternate(Flow *);

    /** Define the flow of which this one is an alternate. */
    DECLARE_EXPORT void setAlternate(const string& n);

    /** Return the search mode. */
    SearchMode getSearch() const {return search;}

    /** Update the search mode. */
    void setSearch(const string a) {search = decodeSearchMode(a);}

    /** A flow is considered hidden when either its buffer or operation
      * are hidden. */
    virtual bool getHidden() const
    {
      return (getBuffer() && getBuffer()->getHidden())
          || (getOperation() && getOperation()->getHidden());
    }

    /** This method holds the logic the compute the date of a flowplan. */
    virtual Date getFlowplanDate(const FlowPlan*) const;

    /** This method holds the logic the compute the quantity of a flowplan. */
    virtual double getFlowplanQuantity(const FlowPlan*) const;

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    static int initialize();
    static void writer(const MetaCategory*, XMLOutput*);

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    virtual size_t getSize() const {return sizeof(Flow) + getName().size();}

  protected:
    /** Default constructor. */
    explicit DECLARE_EXPORT Flow() : quantity(0.0), hasAlts(false),
      altFlow(NULL), search(PRIORITY) {initType(metadata);}

  private:
    /** Verifies whether a flow meets all requirements to be valid. <br>
      * An exception is thrown if the flow is invalid.
      */
    DECLARE_EXPORT void validate(Action action);

    /** Quantity of the flow. */
    double quantity;

    /** Flag that is set to true when a flow has alternates. */
    bool hasAlts;

    /** A flow representing the main flow of a set of alternate flows. */
    Flow* altFlow;

    /** Mode to select the preferred alternates. */
    SearchMode search;

    static PyObject* create(PyTypeObject* pytype, PyObject* args, PyObject* kwds);
    DECLARE_EXPORT PyObject* getattro(const Attribute&);
    DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
};


/** @brief This class defines a material flow to/from a buffer, linked with an
  * operation. This subclass represents a flow that is at the start date of
  * the operation.
  */
class FlowStart : public Flow
{
  public:
    /** Constructor. */
    explicit FlowStart(Operation* o, Buffer* b, double q) : Flow(o,b,q) {}

    /** Constructor. */
    explicit FlowStart(Operation* o, Buffer* b, double q, DateRange e) : Flow(o,b,q,e) {}

    /** This constructor is called from the plan begin_element function. */
    explicit FlowStart() {}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const {return sizeof(FlowStart);}
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}
};


/** @brief This class defines a material flow to/from a buffer, linked with an
  * operation. This subclass represents a flow that is at end date of the
  * operation.
  */
class FlowEnd : public Flow
{
  public:
    /** Constructor. */
    explicit FlowEnd(Operation* o, Buffer* b, double q) : Flow(o,b,q) {}

    /** Constructor. */
    explicit FlowEnd(Operation* o, Buffer* b, double q, DateRange e) : Flow(o,b,q,e) {}

    /** This constructor is called from the plan begin_element function. */
    explicit FlowEnd() {}

    /** This method holds the logic the compute the date of a flowplan. */
    virtual Date getFlowplanDate(const FlowPlan* fl) const;

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const {return sizeof(FlowEnd);}
};


/** @brief This class represents a flow at end date of the
  * operation and with a fiwed quantity.
  */
class FlowFixedEnd : public FlowEnd
{
  public:
    /** Constructor. */
    explicit FlowFixedEnd(Operation* o, Buffer* b, double q) : FlowEnd(o,b,q) {}

    /** Constructor. */
    explicit FlowFixedEnd(Operation* o, Buffer* b, double q, DateRange e) : FlowEnd(o,b,q,e) {}

    /** This constructor is called from the plan begin_element function. */
    explicit FlowFixedEnd() {}

    /** This method holds the logic the compute the quantity of a flowplan. */
    virtual double getFlowplanQuantity(const FlowPlan*) const;

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const {return sizeof(FlowFixedEnd);}
};


/** @brief This class represents a flow at start date of the
  * operation and with a fiwed quantity.
  */
class FlowFixedStart : public FlowStart
{
  public:
    /** Constructor. */
    explicit FlowFixedStart(Operation* o, Buffer* b, double q) : FlowStart(o,b,q) {}

    /** Constructor. */
    explicit FlowFixedStart(Operation* o, Buffer* b, double q, DateRange e) : FlowStart(o,b,q,e) {}

    /** This constructor is called from the plan begin_element function. */
    explicit FlowFixedStart() {}

    /** This method holds the logic the compute the quantity of a flowplan. */
    virtual double getFlowplanQuantity(const FlowPlan*) const;

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const {return sizeof(FlowFixedStart);}
};


/** @brief A flowplan represents a planned material flow in or out of a buffer.
  *
  * Flowplans are owned by operationplans, which manage a container to store
  * them.
  */
class FlowPlan : public TimeLine<FlowPlan>::EventChangeOnhand, public PythonExtensionBase
{
    friend class OperationPlan::FlowPlanIterator;
  private:
    /** Points to the flow instantiated by this flowplan. */
    const Flow *fl;

    /** Python interface method. */
    PyObject* getattro(const Attribute&);

    /** Points to the operationplan owning this flowplan. */
    OperationPlan *oper;

    /** Points to the next flowplan owned by the same operationplan. */
    FlowPlan *nextFlowPlan;

  public:

    static DECLARE_EXPORT const MetaCategory* metadata;
    static int initialize();

    /** Constructor. */
    explicit DECLARE_EXPORT FlowPlan(OperationPlan*, const Flow*);

    /** Returns the flow of which this is an plan instance. */
    const Flow* getFlow() const {return fl;}

    /** Returns the buffer. */
    const Buffer* getBuffer() const {return fl->getBuffer();}

    /** Update the flow of an already existing flowplan.<br>
      * The new flow must belong to the same operation.
      */
    DECLARE_EXPORT void setFlow(const Flow*);

    /** Returns the operationplan owning this flowplan. */
    OperationPlan* getOperationPlan() const {return oper;}

    /** Destructor. */
    virtual ~FlowPlan()
    {
      Buffer* b = getFlow()->getBuffer();
      b->setChanged();
      b->flowplans.erase(this);
    }

    /** Writing the element.
      * This method has the same prototype as a usual instance of the Object
      * class, but this is only superficial: FlowPlan isn't a subclass of
      * Object at all.
      */
    void DECLARE_EXPORT writeElement
    (XMLOutput*, const Keyword&, mode=DEFAULT) const;

    /** Updates the quantity of the flowplan by changing the quantity of the
      * operationplan owning this flowplan.<br>
      * The boolean parameter is used to control whether to round up (false)
      * or down (true) in case the operation quantity must be a multiple.<br>
      * The second parameter is to flag whether we want to actually perform
      * the resizing, or only to simulate it.
      */
    double setQuantity(double qty, bool b=false, bool u = true)
    {
      if (!getFlow()->getEffective().within(getDate())) return 0.0;
      if (getFlow()->getType() == *FlowFixedEnd::metadata
        || getFlow()->getType() == *FlowFixedStart::metadata)
      {
        // Fixed quantity flows only allow resizing to 0
        if (qty == 0.0 && oper->getQuantity()!= 0.0)
          return oper->setQuantity(0.0, b, u) ? getFlow()->getQuantity() : 0.0;
        else if (qty != 0.0 && oper->getQuantity()== 0.0)
          return oper->setQuantity(
            (oper->getOperation()->getSizeMinimum()<=0) ? 0.001
              : oper->getOperation()->getSizeMinimum(),
            b, u) ? getFlow()->getQuantity() : 0.0;
      }
      else
        // Normal, proportional flows
        return oper->setQuantity(qty / getFlow()->getQuantity(), b, u) * getFlow()->getQuantity();
      throw LogicException("Unreachable code reached");
    }

    /** This function needs to be called whenever the flowplan date or
      * quantity are changed.
      */
    DECLARE_EXPORT void update();

    /** Return a pointer to the timeline data structure owning this flowplan. */
    TimeLine<FlowPlan>* getTimeLine() const
    {return &(getFlow()->getBuffer()->flowplans);}

    /** Returns true when the flowplan is hidden.<br>
      * This is determined by looking at whether the flow is hidden or not.
      */
    bool getHidden() const {return fl->getHidden();}
};


inline double Flow::getFlowplanQuantity(const FlowPlan* fl) const
{
  return getEffective().within(fl->getDate()) ?
    fl->getOperationPlan()->getQuantity() * getQuantity() :
    0.0;
}


inline double FlowFixedStart::getFlowplanQuantity(const FlowPlan* fl) const
{
  return getEffective().within(fl->getDate()) ?
    getQuantity() :
    0.0;
}


inline double FlowFixedEnd::getFlowplanQuantity(const FlowPlan* fl) const
{
  return getEffective().within(fl->getDate()) ?
    getQuantity() :
    0.0;
}


inline Date Flow::getFlowplanDate(const FlowPlan* fl) const
{
  return fl->getOperationPlan()->getDates().getStart();
}


inline Date FlowEnd::getFlowplanDate(const FlowPlan* fl) const
{
  return fl->getOperationPlan()->getDates().getEnd();
}


/** @brief This class is used to represent a matrix defining the changeover
  * times between setups.
  */
class SetupMatrix : public HasName<SetupMatrix>, public HasSource
{
  public:
    class RuleIterator; // Forward declaration
    /** @brief An specific changeover rule in a setup matrix. */
    class Rule : public Object
    {
        friend class RuleIterator;
        friend class SetupMatrix;
      public:
        /** Constructor. */
        DECLARE_EXPORT Rule(SetupMatrix *s, int p = 0);

        /** Destructor. */
        DECLARE_EXPORT ~Rule();

        virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
        DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
        virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
        virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
        static int initialize();

        virtual const MetaClass& getType() const {return *metadata;}
        static DECLARE_EXPORT const MetaCategory* metadata;

        size_t getSize() const
        {return sizeof(Rule) + from.size() + to.size();}

        /** Update the priority.<br>
          * The priority value is a key field. If multiple rules have the
          * same priority a data exception is thrown.
          */
        DECLARE_EXPORT void setPriority(const int);

        /** Return the matrix owning this rule. */
        SetupMatrix* getSetupMatrix() const {return matrix;}

        /** Return the priority. */
        double getPriority() const {return priority;}

        /** Update the from setup. */
        void setFromSetup(const string f) {from = f;}

        /** Return the from setup. */
        const string& getFromSetup() const {return from;}

        /** Update the from setup. */
        void setToSetup(const string f) {to = f;}

        /** Return the from setup. */
        const string& getToSetup() const {return to;}

        /** Update the conversion duration. */
        void setDuration(const TimePeriod p) {duration = p;}

        /** Return the conversion duration. */
        TimePeriod getDuration() const {return duration;}

        /** Update the conversion cost. */
        void setCost(const double p) {cost = p;}

        /** Return the conversion cost. */
        double getCost() const {return cost;}

      private:
        /** Original setup. */
        string from;

        /** New setup. */
        string to;

        /** Changeover time. */
        TimePeriod duration;

        /** Changeover cost. */
        double cost;

        /** Priority of the rule.<br>
          * This field is the key field, i.e. within a setup matrix all rules
          * need to have different priorities.
          */
        int priority;

        /** Pointer to the owning matrix. */
        SetupMatrix *matrix;

        /** Pointer to the next rule in this matrix. */
        Rule *nextRule;

        /** Pointer to the previous rule in this matrix. */
        Rule *prevRule;
    };

    /** @brief An iterator class to go through all rules of a setup matrix. */
    class RuleIterator
    {
      private:
        Rule* curRule;
      public:
        /** Constructor. */
        RuleIterator(Rule* c = NULL) : curRule(c) {}
        bool operator != (const RuleIterator &b) const
        {return b.curRule != curRule;}
        bool operator == (const RuleIterator &b) const
        {return b.curRule == curRule;}
        RuleIterator& operator++()
        {if (curRule) curRule = curRule->nextRule; return *this;}
        RuleIterator operator++(int)
        {RuleIterator tmp = *this; ++*this; return tmp;}
        RuleIterator& operator--()
        {if(curRule) curRule = curRule->prevRule; return *this;}
        RuleIterator operator--(int)
        {RuleIterator tmp = *this; --*this; return tmp;}
        Rule* operator ->() const {return curRule;}
        Rule& operator *() const {return *curRule;}
    };

  public:
    /** Default constructor. */
    explicit DECLARE_EXPORT SetupMatrix(const string& n) :
      HasName<SetupMatrix>(n), firstRule(NULL) {}

    /** Destructor. */
    DECLARE_EXPORT ~SetupMatrix();

    /** This is a factory method that creates a new rule<br>
      * This method is intended to be used to create objects when reading
      * XML input data.
      */
    DECLARE_EXPORT Rule* createRule(const AttributeList&);

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

    virtual size_t getSize() const
    {
      size_t i = sizeof(SetupMatrix) + getName().size();
      for (RuleIterator j = beginRules(); j!= endRules(); ++j)
        i += j->getSize();
      return i;
    }

    size_t extrasize() const
    {
      size_t i = getName().size();
      for (RuleIterator j = beginRules(); j!= endRules(); ++j)
        i += j->getSize();
      return i;
    }

    /** Returns an iterator to go through the list of rules. */
    RuleIterator beginRules() const {return RuleIterator(firstRule);}

    /** Returns an iterator to go through the list of rules. */
    RuleIterator endRules() const {return RuleIterator(NULL);}

    /** Python interface to add a new rule. */
    static DECLARE_EXPORT PyObject* addPythonRule(PyObject*, PyObject*, PyObject*);

    /** Computes the changeover time and cost between 2 setup values.
      *
      * To compute the time of a changeover the algorithm will evaluate all
      * rules in sequence (in order of priority).<br>
      * For a rule to match the changeover between the original setup X to
      * a new setup Y, two conditions need to be fulfilled:
      *  - The original setup X must match with the fromsetup of the rule.<br>
      *    If the fromsetup field is empty, it is considered a match.
      *  - The new setup Y must match with the tosetup of the rule.<br>
      *    If the tosetup field is empty, it is considered a match.
      * The wildcard characters * and ? can be used in the fromsetup and
      * tosetup fields.<br>
      * As soon as a matching rule is found, it is applied and subsequent
      * rules are not evaluated.<br>
      * If no matching rule is found, the changeover is not allowed: a NULL
      * pointer is returned.
      */
    DECLARE_EXPORT Rule* calculateSetup(const string, const string) const;

  private:
    /** Head of the list of rules. */
    Rule *firstRule;
};


/** @brief This class is the default implementation of the abstract
  * SetupMatrix class.
  */
class SetupMatrixDefault : public SetupMatrix
{
  public:
    explicit SetupMatrixDefault(const string& str) : SetupMatrix(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(SetupMatrixDefault) + SetupMatrix::extrasize();}
    static int initialize();
};


/** @brief This class models skills that can be assigned to resources. */
class Skill : public HasName<Skill>, public HasSource
{
  friend class ResourceSkill;

  public:
    /** Default constructor. */
    explicit DECLARE_EXPORT Skill(const string& n) : HasName<Skill>(n) {}

    /** Destructor. */
    DECLARE_EXPORT ~Skill();

    typedef Association<Resource,Skill,ResourceSkill>::ListB resourcelist;

    /** Returns an reference to the list of resources having this skill. */
    const resourcelist& getResources() const {return resources;}

    /** Python interface to add a new resource. */
    static DECLARE_EXPORT PyObject* addPythonResource(PyObject*, PyObject*);

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

    virtual size_t getSize() const
    {
      size_t i = sizeof(Skill) + getName().size() + resources.size() * 3 * sizeof(Resource*);
      return i;
    }

    size_t extrasize() const
    {
      return getName().size() + resources.size() * 3 * sizeof(Resource*);
    }

  private:
    /** This is a list of resources having this skill. */
    resourcelist resources;
};


/** @brief this class is the default implementation of the abstract
  * Skill class.
  */
class SkillDefault : public Skill
{
  public:
    explicit SkillDefault(const string& str) : Skill(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(SkillDefault) + Skill::extrasize();}
    static int initialize();
};


/** @brief This class represents a workcentre, a physical or logical
  * representation of capacity.
  */
class Resource : public HasHierarchy<Resource>,
  public HasLevel, public Plannable, public HasDescription
{
    friend class Load;
    friend class LoadPlan;
    friend class ResourceSkill;

  public:
    class PlanIterator;

    /** The default time window before the ask date where we look for
      * available capacity. */
    static const long defaultMaxEarly = 100*86400L;

    /** Constructor. */
    explicit Resource(const string& str) : HasHierarchy<Resource>(str),
      size_max_cal(NULL), size_max(0), loc(NULL), cost(0.0), hidden(false), maxearly(defaultMaxEarly),
      setupmatrix(NULL) { setMaximum(1); };

    /** Destructor. */
    virtual DECLARE_EXPORT ~Resource();

    /** Updates the size of a resource, when it is time-dependent. */
    virtual DECLARE_EXPORT void setMaximumCalendar(CalendarDouble*);

    /** Updates the size of a resource. */
    DECLARE_EXPORT void setMaximum(double);

    /** Return a pointer to the maximum capacity profile. */
    CalendarDouble* getMaximumCalendar() const {return size_max_cal;}

    /** Return a pointer to the maximum capacity. */
    double getMaximum() const {return size_max;}

    /** Returns the cost of using 1 unit of this resource for 1 hour.<br>
      * The default value is 0.0.
      */
    double getCost() const {return cost;}

    /** Update the cost of using 1 unit of this resource for 1 hour. */
    void setCost(const double c)
    {
      if (c >= 0) cost = c;
      else throw DataException("Resource cost must be positive");
    }

    typedef Association<Operation,Resource,Load>::ListB loadlist;
    typedef Association<Resource,Skill,ResourceSkill>::ListA skilllist;
    typedef TimeLine<LoadPlan> loadplanlist;

    /** Returns a reference to the list of loadplans. */
    const loadplanlist& getLoadPlans() const {return loadplans;}

    /** Returns a reference to the list of loadplans. */
    loadplanlist& getLoadPlans() {return loadplans;}

    /** Returns a constant reference to the list of loads. It defines
      * which operations are using the resource.
      */
    const loadlist& getLoads() const {return loads;}

    /** Returns a constant reference to the list of skills. */
    const skilllist& getSkills() const {return skills;}

    /** Return the load that is associates a given operation with this
      * resource. Returns NULL is no such load exists. */
    Load* findLoad(const Operation* o, Date d) const
    {return loads.find(o,d);}

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);

    /** Initialize the class. */
    static int initialize();

    size_t extrasize() const
    {
      return getName().size() + HasDescription::extrasize()
        + setup.size() + skills.size() * 3 * sizeof(Skill*);
    }

    /** Returns the location of this resource. */
    Location* getLocation() const {return loc;}

    /** Updates the location of this resource. */
    void setLocation(Location* i) {loc = i;}

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    /** Deletes all operationplans loading this resource. The boolean parameter
      * controls whether we delete also locked operationplans or not.
      */
    DECLARE_EXPORT void deleteOperationPlans(bool = false);

    /** Recompute the problems of this resource. */
    virtual DECLARE_EXPORT void updateProblems();

    /** Scan the setups of this resource. */
    virtual DECLARE_EXPORT void updateSetups(const LoadPlan* = NULL);

    void setHidden(bool b) {if (hidden!=b) setChanged(); hidden = b;}
    bool getHidden() const {return hidden;}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

    /** Returns the maximum inventory buildup allowed in case of capacity
      * shortages. */
    TimePeriod getMaxEarly() const {return maxearly;}

    /** Updates the maximum inventory buildup allowed in case of capacity
      * shortages. */
    void setMaxEarly(TimePeriod c)
    {
      if (c >= 0L) maxearly = c;
      else throw DataException("MaxEarly must be positive");
    }

    /** Return a pointer to the setup matrix. */
    SetupMatrix* getSetupMatrix() const {return setupmatrix;}

    /** Update the reference to the setup matrix. */
    void setSetupMatrix(SetupMatrix *s) {setupmatrix = s;}

    /** Return the current setup. */
    const string& getSetup() const {return setup;}

    /** Update the current setup. */
    void setSetup(const string s) {setup = s;}

  protected:
    /** This calendar is used to updates to the resource size. */
    CalendarDouble* size_max_cal;

    /** Stores the collection of all loadplans of this resource. */
    loadplanlist loadplans;

  private:
    /** The maximum resource size.<br>
      * If a calendar is specified, this field is ignored.
      */
    double size_max;

    /** This is a list of all load models that are linking this resource with
      * operations. */
    loadlist loads;

    /** This is a list of skills this resource has. */
    skilllist skills;

    /** A pointer to the location of the resource. */
    Location* loc;

    /** The cost of using 1 unit of this resource for 1 hour. */
    double cost;

    /** Specifies whether this resource is hidden for serialization. */
    bool hidden;

    /** Maximum inventory buildup allowed in case of capacity shortages. */
    TimePeriod maxearly;

    /** Reference to the setup matrix. */
    SetupMatrix *setupmatrix;

    /** Current setup. */
    string setup;

    /** Python method that returns an iterator over the resource plan. */
    static PyObject* plan(PyObject*, PyObject*);
};


/** @brief This class provides an efficient way to iterate over
  * the plan of a resource aggregated in time buckets.<br>
  * For resources of type default, a list of dates needs to be passed as
  * argument to define the time buckets.<br>
  * For resources of type buckets, the time buckets are defined on the
  * resource and the argument is ignored.
  */
class Resource::PlanIterator : public PythonExtension<Resource::PlanIterator>
{
  public:
    static int initialize();

    /** Constructor.
      * The first argument is the resource whose plan we're looking at.
      * The second argument is a Python iterator over a list of dates. These
      * dates define the buckets at which we aggregate the resource plan.
      */
    PlanIterator(Resource*, PyObject*);

    /** Destructor. */
    ~PlanIterator();

  private:
    /** Pointer to the resource we're investigating. */
    Resource* res;

    /** A Python object pointing to a list of start dates of buckets. */
    PyObject* bucketiterator;

    /** An iterator over all events in the resource timeline. */
    Resource::loadplanlist::iterator ldplaniter;

    /** Python function to iterate over the periods. */
    PyObject* iternext();

    double cur_setup;
    double cur_load;
    double cur_size;
    bool bucketized;
    Date cur_date;
    Date prev_date;
    bool prev_value;
    Calendar::EventIterator unavailableIterator;
    bool hasUnavailability;
    double bucket_available;
    double bucket_load;
    double bucket_setup;
    double bucket_unavailable;

    void update(Date till);

    /** Python object pointing to the start date of the plan bucket. */
    PyObject* start_date;

    /** Python object pointing to the start date of the plan bucket. */
    PyObject* end_date;
};


/** @brief This class is the default implementation of the abstract
  * Resource class.
  */
class ResourceDefault : public Resource
{
  public:
    explicit ResourceDefault(const string& str) : Resource(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(ResourceDefault) + Resource::extrasize();}
    static int initialize();
};


/** @brief This class represents a resource that'll never have any
  * capacity shortage. */
class ResourceInfinite : public Resource
{
  public:
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual const MetaClass& getType() const {return *metadata;}
    explicit ResourceInfinite(const string& c) : Resource(c)
    {setDetectProblems(false); initType(metadata);}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(ResourceInfinite) + Resource::extrasize();}
    static int initialize();
};


/** @brief This class represents a resource whose capacity is defined per
    time bucket. */
class ResourceBuckets : public Resource
{
  public:
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual const MetaClass& getType() const {return *metadata;}
    explicit ResourceBuckets(const string& c) : Resource(c)
    {initType(metadata);}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(ResourceBuckets) + Resource::extrasize();}
    static int initialize();

    virtual DECLARE_EXPORT void updateProblems();

    /** Updates the time buckets and the quantity per time bucket. */
    virtual DECLARE_EXPORT void setMaximumCalendar(CalendarDouble*);

};


/** @brief This class associates a resource with its skills. */
class ResourceSkill : public Object, public Association<Resource,Skill,ResourceSkill>::Node
{
  public:
    /** Default constructor. */
    explicit ResourceSkill() {initType(metadata);}

    /** Constructor. */
    explicit DECLARE_EXPORT ResourceSkill(Skill*, Resource*, int);

    /** Constructor. */
    explicit DECLARE_EXPORT ResourceSkill(Skill*, Resource*, int, DateRange);

    /** Initialize the class. */
    static int initialize();
    static void writer(const MetaCategory*, XMLOutput*);
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    DECLARE_EXPORT PyObject* getattro(const Attribute&);
    DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);

    /** Returns the resource. */
    Resource* getResource() const {return getPtrA();}

    /** Updates the resource. This method can only be called on an instance. */
    void setResource(Resource* r) {if (r) setPtrA(r,r->getSkills());}

    /** Returns the skill. */
    Skill* getSkill() const {return getPtrB();}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    virtual size_t getSize() const {return sizeof(ResourceSkill);}

    /** Updates the skill. This method can only be called on an instance. */
    void setSkill(Skill* s) {if (s) setPtrB(s,s->getResources());}

  private:
    /** Factory method. */
    static PyObject* create(PyTypeObject*, PyObject*, PyObject*);

    /** This method is called to check the validity of the object.<br>
      * An exception is thrown if the resourceskill is invalid.
      */
    DECLARE_EXPORT void validate(Action action);
};


/** @brief This class links a resource to a certain operation. */
class Load
  : public Object, public Association<Operation,Resource,Load>::Node,
  public Solvable, public HasSource
{
    friend class Resource;
    friend class Operation;

  public:
    /** Constructor. */
    explicit Load(Operation* o, Resource* r, double u)
      : hasAlts(false), altLoad(NULL), search(PRIORITY), skill(NULL)
    {
      setOperation(o);
      setResource(r);
      setQuantity(u);
      initType(metadata);
      try { validate(ADD); }
      catch (...)
      {
        if (getOperation()) getOperation()->loaddata.erase(this);
        if (getResource()) getResource()->loads.erase(this);
        resetReferenceCount();
        throw;
      }
    }

    /** Constructor. */
    explicit Load(Operation* o, Resource* r, double u, DateRange e)
      : hasAlts(false), altLoad(NULL), search(PRIORITY), skill(NULL)
    {
      setOperation(o);
      setResource(r);
      setQuantity(u);
      setEffective(e);
      initType(metadata);
      try { validate(ADD); }
      catch (...)
      {
        if (getOperation()) getOperation()->loaddata.erase(this);
        if (getResource()) getResource()->loads.erase(this);
        resetReferenceCount();
        throw;
      }
    }

    /** Destructor. */
    DECLARE_EXPORT ~Load();

    /** Returns the operation consuming the resource capacity. */
    Operation* getOperation() const {return getPtrA();}

    /** Updates the operation being loaded. This method can only be called
      * once for a load. */
    void setOperation(Operation* o) {if (o) setPtrA(o,o->getLoads());}

    /** Returns the capacity resource being consumed. */
    Resource* getResource() const {return getPtrB();}

    /** Updates the capacity being consumed. This method can only be called
      * once on a resource. */
    void setResource(Resource* r) {if (r) setPtrB(r,r->getLoads());}

    /** Returns how much capacity is consumed during the duration of the
      * operationplan. */
    double getQuantity() const {return qty;}

    /** Updates the quantity of the load.
      * @exception DataException When a negative number is passed.
      */
    void setQuantity(double f)
    {
      if (f < 0) throw DataException("Load quantity can't be negative");
      qty = f;
    }

    /** Returns true if there are alternates for this load. */
    bool hasAlternates() const {return hasAlts;}

    /** Returns the load of which this one is an alternate.<br>
      * NULL is return where there is none.
      */
    Load* getAlternate() const {return altLoad;}

    /** Define the load of which this one is an alternate. */
    DECLARE_EXPORT void setAlternate(Load *);

    /** Define the load of which this one is an alternate. */
    DECLARE_EXPORT void setAlternate(const string& n);

    /** Update the required resource setup. */
    DECLARE_EXPORT void setSetup(const string);

    /** Return the required resource setup. */
    const string& getSetup() const {return setup;}

    /** Update the required skill. */
    void setSkill(Skill* s) {skill = s;}

    /** Return the required skill. */
    Skill* getSkill() const {return skill;}

    /** This method holds the logic the compute the date of a loadplan. */
    virtual Date getLoadplanDate(const LoadPlan*) const;

    /** This method holds the logic the compute the quantity of a loadplan. */
    virtual double getLoadplanQuantity(const LoadPlan*) const;

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    DECLARE_EXPORT PyObject* getattro(const Attribute&);
    DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();
    static void writer(const MetaCategory*, XMLOutput*);

    bool getHidden() const
    {
      return (getResource() && getResource()->getHidden())
          || (getOperation() && getOperation()->getHidden());
    }
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    virtual size_t getSize() const
    {return sizeof(Load) + getName().size() + getSetup().size();}

    /** Default constructor. */
    Load() : qty(1.0), hasAlts(false), altLoad(NULL),
      search(PRIORITY), skill(NULL) {initType(metadata);}

    /** Return the search mode. */
    SearchMode getSearch() const {return search;}

    /** Update the search mode. */
    void setSearch(const string a) {search = decodeSearchMode(a);}

  private:
    /** This method is called to check the validity of the object.<br>
      * An exception is thrown if the load is invalid.
      */
    DECLARE_EXPORT void validate(Action action);

    /** Stores how much capacity is consumed during the duration of an
      * operationplan. */
    double qty;

    /** Flag that is set to true when a load has alternates. */
    bool hasAlts;

    /** A load representing the main load of a set of alternates. */
    Load* altLoad;

    /** Required setup. */
    string setup;

    /** Mode to select the preferred alternates. */
    SearchMode search;

    /** Required skill. */
    Skill* skill;

    /** Factory method. */
    static PyObject* create(PyTypeObject*, PyObject*, PyObject*);
};



/** @brief This is the (logical) top class of the complete model.
  *
  * This is a singleton class: only a single instance can be created.
  * The data model has other limitations that make it not obvious to support
  * building multiple models/plans in memory of the same application: e.g.
  * the operations, resources, problems, operationplans... etc are all
  * implemented in static, global lists. An entity can't be simply linked with
  * a particular plan if multiple ones would exist.
  */
class Plan : public Plannable, public Object
{
  private:
    /** Current Date of this plan. */
    Date cur_Date;

    /** A name for this plan. */
    string name;

    /** A getDescription of this plan. */
    string descr;

    /** Pointer to the singleton plan object. */
    static DECLARE_EXPORT Plan* thePlan;

    /** The only constructor of this class is made private. An object of this
      * class is created by the instance() member function.
      */
    Plan() : cur_Date(Date::now()) {initType(metadata);}

  public:
    /** Return a pointer to the singleton plan object.
      * The singleton object is created during the initialization of the
      * library.
      */
    static Plan& instance() {return *thePlan;}

    /** Destructor.
      * @warning In multi threaded applications, the destructor is never called
      * and the plan object leaks when we exit the application.
      * In single-threaded applications this function is called properly, when
      * the static plan variable is deleted.
      */
    DECLARE_EXPORT ~Plan();

    /** Returns the plan name. */
    const string& getName() const {return name;}

    /** Updates the plan name. */
    void setName(const string& s) {name = s;}

    /** Returns the current Date of the plan. */
    const Date & getCurrent() const {return cur_Date;}

    /** Updates the current date of the plan. This method can be relatively
      * heavy in a plan where operationplans already exist, since the
      * detection for BeforeCurrent problems needs to be rerun.
      */
    DECLARE_EXPORT void setCurrent(Date);

    /** Returns the description of the plan. */
    const string& getDescription() const {return descr;}

    /** Updates the description of the plan. */
    void setDescription(const string& str) {descr = str;}

    /** This method writes out the model information. Depending on a flag in
      * the XMLOutput object a complete model is written, or only the
      * dynamic plan information.
      */
    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    DECLARE_EXPORT PyObject* getattro(const Attribute&);
    DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);

    /** Initialize the class. */
    static int initialize();

    virtual void updateProblems() {};

    /** This method basically solves the whole planning problem. */
    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    virtual size_t getSize() const
    {return sizeof(Plan) + name.size() + descr.size();}
};


/** @brief Represents the (independent) demand in the system. It can represent a
  * customer order or a forecast.
  *
  * This is an abstract class.
  */
class Demand
  : public HasHierarchy<Demand>, public Plannable, public HasDescription
{
  public:
    typedef slist<OperationPlan*> OperationPlan_list;

    /** Constructor. */
    explicit DECLARE_EXPORT Demand(const string& str) :
      HasHierarchy<Demand>(str), it(NULL), oper(NULL), cust(NULL), qty(0.0),
      prio(0), maxLateness(TimePeriod::MAX), minShipment(1), hidden(false) {}

    /** Destructor.
      * Deleting the demand will also delete all delivery operation
      * plans (including locked ones) and reset the motive on any
      * operationplans. */
    virtual DECLARE_EXPORT ~Demand();

    /** Returns the quantity of the demand. */
    double getQuantity() const {return qty;}

    /** Updates the quantity of the demand. The quantity must be be greater
      * than or equal to 0. */
    virtual DECLARE_EXPORT void setQuantity(double);

    /** Returns the priority of the demand.<br>
      * Lower numbers indicate a higher priority level.
      */
    int getPriority() const {return prio;}

    /** Updates the due date of the demand.<br>
      * Lower numbers indicate a higher priority level.
      */
    virtual void setPriority(int i) {prio=i; setChanged();}

    /** Returns the item/product being requested. */
    Item* getItem() const {return it;}

    /** Updates the item/product being requested. */
    virtual void setItem(Item *i) {it=i; setChanged();}

    /** This fields points to an operation that is to be used to plan the
      * demand. By default, the field is left to NULL and the demand will then
      * be planned using the delivery operation of its item.
      * @see Item::getDelivery()
      */
    Operation* getOperation() const {return oper;}

    /** This function returns the operation that is to be used to satisfy this
      * demand. In sequence of priority this goes as follows:
      *   1) If the "operation" field on the demand is set, use it.
      *   2) Otherwise, use the "delivery" field of the requested item.
      *   3) Else, return NULL. This demand can't be satisfied!
      */
    DECLARE_EXPORT Operation* getDeliveryOperation() const;

    /** Returns the cluster which this demand belongs to. */
    int getCluster() const
    {
      Operation* o = getDeliveryOperation();
      return o ? o->getCluster() : 0;
    }

    /** Updates the operation being used to plan the demand. */
    virtual void setOperation(Operation* o) {oper=o; setChanged();}

    /** Returns the delivery operationplan list. */
    DECLARE_EXPORT const OperationPlan_list& getDelivery() const;

    /** Returns the latest delivery operationplan. */
    DECLARE_EXPORT OperationPlan* getLatestDelivery() const;

    /** Returns the earliest delivery operationplan. */
    DECLARE_EXPORT OperationPlan* getEarliestDelivery() const;

    /** Adds a delivery operationplan for this demand. */
    DECLARE_EXPORT void addDelivery(OperationPlan *o);

    /** Removes a delivery operationplan for this demand. */
    DECLARE_EXPORT void removeDelivery(OperationPlan *o);

    /** Deletes all delivery operationplans of this demand.<br>
      * The (optional) boolean parameter controls whether we delete also locked
      * operationplans or not.<br>
      * The second (optional) argument is a command list that can be used to
      * remove the operationplans in an undo-able way.<br>
      * The optional third argument is used to flag whether or not we also
      * want to delete the upstream pegged operationplans along the supply
      * path.
      *
      * \attention This method is implemented differently in the enterprise
      * edition. The community edition only deletes the delivery
      * operationplans.
      */
    DECLARE_EXPORT void deleteOperationPlans (
      bool deleteLockedOpplans = false,
      CommandManager* = NULL,
      bool deleteUpstream = true
      );

    /** Returns the due date of the demand. */
    const Date& getDue() const {return dueDate;}

    /** Updates the due date of the demand. */
    virtual void setDue(Date d) {dueDate = d; setChanged();}

    /** Returns the customer. */
    Customer* getCustomer() const {return cust;}

    /** Updates the customer. */
    virtual void setCustomer(Customer* c) {cust = c; setChanged();}

    /** Return a reference to the constraint list. */
    const Problem::List& getConstraints() const {return constraints;}

    /** Return a reference to the constraint list. */
    Problem::List& getConstraints() {return constraints;}

    /** Returns the total amount that has been planned. */
    DECLARE_EXPORT double getPlannedQuantity() const;

    virtual DECLARE_EXPORT void writeElement(XMLOutput*, const Keyword&, mode=DEFAULT) const;
    virtual DECLARE_EXPORT void endElement(XMLInput&, const Attribute&, const DataElement&);
    virtual DECLARE_EXPORT void beginElement(XMLInput&, const Attribute&);
    virtual DECLARE_EXPORT PyObject* getattro(const Attribute&);
    virtual DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);
    static int initialize();

    size_t extrasize() const
    {
      return getName().size() + HasDescription::extrasize()
          + sizeof(void*) * 2 * deli.size();
    }

    virtual void solve(Solver &s, void* v = NULL) const {s.solve(this,v);}

    /** Return the maximum delay allowed in satisfying this demand.<br>
      * The default value is infinite.
      */
    TimePeriod getMaxLateness() const {return maxLateness;}

    /** Updates the maximum allowed lateness for this demand.<br>
      * The default value is infinite.<br>
      * The argument must be a positive time period.
      */
    virtual void setMaxLateness(TimePeriod m)
    {
      if (m < 0L)
        throw DataException("The maximum demand lateness must be positive");
      maxLateness = m;
    }

    /** Return the minimum shipment quantity allowed in satisfying this
      * demand.<br>
      * The default value is 1.
      */
    double getMinShipment() const {return minShipment;}

    /** Updates the maximum allowed lateness for this demand.<br>
      * The default value is infinite.<br>
      * The argument must be a positive time period.
      */
    virtual void setMinShipment(double m)
    {
      if (m < 0.0)
        throw DataException("The minumum demand shipment quantity must be positive");
      minShipment = m;
    }

    /** Recompute the problems. */
    virtual DECLARE_EXPORT void updateProblems();

    /** Specifies whether of not this demand is to be hidden from
      * serialization. The default value is false. */
    void setHidden(bool b) {hidden = b;}

    /** Returns true if this demand is to be hidden from serialization. */
    bool getHidden() const {return hidden;}

    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;

  private:
    /** Requested item. */
    Item *it;

    /** Delivery Operation. Can be left NULL, in which case the delivery
      * operation can be specified on the requested item. */
    Operation *oper;

    /** Customer creating this demand. */
    Customer *cust;

    /** Requested quantity. Only positive numbers are allowed. */
    double qty;

    /** Priority. Lower numbers indicate a higher priority level.*/
    int prio;

    /** Due date. */
    Date dueDate;

    /** Maximum lateness allowed when planning this demand.<br>
      * The default value is TimePeriod::MAX.
      */
    TimePeriod maxLateness;

    /** Minimum size for a delivery operation plan satisfying this demand. */
    double minShipment;

    /** Hide this demand or not. */
    bool hidden;

    /** A list of operation plans to deliver this demand. */
    OperationPlan_list deli;

    /** A list of constraints preventing this demand from being planned in
      * full and on time. */
    Problem::List constraints;
};


/** @brief This class is the default implementation of the abstract
  * Demand class. */
class DemandDefault : public Demand
{
  public:
    explicit DemandDefault(const string& str) : Demand(str) {initType(metadata);}
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaClass* metadata;
    virtual size_t getSize() const
    {return sizeof(DemandDefault) + Demand::extrasize();}
    static int initialize();
};


/** @brief This class represents the resource capacity of an operationplan.
  *
  * For both the start and the end date of the operationplan, a loadplan
  * object is created. These are then inserted in the timeline structure
  * associated with a resource.
  */
class LoadPlan : public TimeLine<LoadPlan>::EventChangeOnhand, public PythonExtensionBase
{
    friend class OperationPlan::LoadPlanIterator;
  public:
    /** Public constructor.<br>
      * This constructor constructs the starting loadplan and will
      * also call a private constructor to creates the ending loadplan.
      * In other words, a single call to the constructor will create
      * two loadplan objects.
      */
    explicit DECLARE_EXPORT LoadPlan(OperationPlan*, const Load*);

    /** Return the operationplan owning this loadplan. */
    OperationPlan* getOperationPlan() const {return oper;}

    /** Return the load of which this is a plan instance. */
    const Load* getLoad() const {return ld;}

    /** Update the resource.<br>
      * The optional second argument specifies whether or not we need to verify
      * if the assigned resource is valid. A valid resource must a) be a
      * subresource of the resource specified on the load, and b) must also
      * have the skill specified on the resource.
      */
    DECLARE_EXPORT void setResource(Resource*, bool = false);

    /** Return the resource. */
    Resource* getResource() const {return res;}

    /** Update the load of an already existing flowplan.<br>
      * The new load must belong to the same operation.
      */
    DECLARE_EXPORT void setLoad(const Load*);

    /** Return true when this loadplan marks the start of an operationplan. */
    bool isStart() const {return start_or_end == START;}

    /** Destructor. */
    DECLARE_EXPORT virtual ~LoadPlan();

    /** This function needs to be called whenever the loadplan date or
      * quantity are changed.
      */
    DECLARE_EXPORT void update();

    /** Return a pointer to the timeline data structure owning this loadplan. */
    TimeLine<LoadPlan>* getTimeLine() const
    {return &(res->loadplans);}

    /** Returns the current setup of the resource.<br>
      * When the argument is true (= default) the current setup is returned.<br>
      * When the argument is false the setup just before the loadplan is returned.
      */
    DECLARE_EXPORT const string& getSetup(bool = true) const;

    /** Returns true when the loadplan is hidden.<br>
      * This is determined by looking at whether the load is hidden or not.
      */
    bool getHidden() const {return ld->getHidden();}

    /** Each operationplan has 2 loadplans per load: one at the start,
      * when the capacity consumption starts, and one at the end, when the
      * capacity consumption ends.<br>
      * This method returns the "companion" loadplan. It is not very
      * scalable: the performance is linear with the number of loadplans
      * on the resource.
      */
    DECLARE_EXPORT LoadPlan* getOtherLoadPlan() const;

    static int initialize();
    static DECLARE_EXPORT const MetaCategory* metadata;
    PyObject* getattro(const Attribute&);
    DECLARE_EXPORT int setattro(const Attribute&, const PythonObject&);

  private:
    /** Private constructor. It is called from the public constructor.<br>
      * The public constructor constructs the starting loadplan, while this
      * constructor creates the ending loadplan.
      */
    DECLARE_EXPORT LoadPlan(OperationPlan*, const Load*, LoadPlan*);

    /** This type is used to differentiate loadplans aligned with the START date
      * or the END date of operationplan. */
    enum type {START, END};

    /** Is this loadplan a starting one or an ending one. */
    type start_or_end;

    /** A pointer to the load model. */
    const Load *ld;

    /** A pointer to the selected resource.<br>
      * In case we use skills, the resource of the loadplan can be different
      * than the resource on the load.
      */
    Resource *res;

    /** A pointer to the operationplan owning this loadplan. */
    OperationPlan *oper;

    /** Points to the next loadplan owned by the same operationplan. */
    LoadPlan *nextLoadPlan;
};


inline Date Load::getLoadplanDate(const LoadPlan* lp) const
{
  const DateRange & dr = lp->getOperationPlan()->getDates();
  if (lp->isStart())
    return dr.getStart() > getEffective().getStart() ?
        dr.getStart() :
        getEffective().getStart();
  else
    return dr.getEnd() < getEffective().getEnd() ?
        dr.getEnd() :
        getEffective().getEnd();
}


inline double Load::getLoadplanQuantity(const LoadPlan* lp) const
{
  if (!lp->getOperationPlan()->getQuantity())
    // Operationplan has zero size, and so should the capacity it needs
    return 0.0;
  if (!lp->getOperationPlan()->getDates().overlap(getEffective())
      && (lp->getOperationPlan()->getDates().getDuration()
          || !getEffective().within(lp->getOperationPlan()->getDates().getStart())))
    // Load is not effective during this time.
    // The extra check is required to make sure that zero duration operationplans
    // operationplans don't get resized to 0
    return 0.0;
  if (getResource()->getType() == *ResourceBuckets::metadata)
    // Bucketized resource
    return - getQuantity() * lp->getOperationPlan()->getQuantity();
  else
    // Continuous resource
    return lp->isStart() ? getQuantity() : -getQuantity();
}



/** @brief A problem of this class is created when an operationplan is being
  * planned in the past, i.e. it starts before the "current" date of
  * the plan.
  */
class ProblemBeforeCurrent : public Problem
{
  public:
    string getDescription() const
    {
      ostringstream ch;
      ch << "Operation '"
          << (oper ? oper : static_cast<OperationPlan*>(getOwner())->getOperation())
          << "' planned in the past";
      return ch.str();
    }
    bool isFeasible() const {return false;}
    double getWeight() const
    {return oper ? state.quantity : static_cast<OperationPlan*>(getOwner())->getQuantity();}
    explicit ProblemBeforeCurrent(OperationPlan* o, bool add = true) : Problem(o), oper(NULL)
    {if (add) addProblem();}
    explicit ProblemBeforeCurrent(Operation* o, Date st, Date nd, double q)
      : oper(o), state(st, nd, q) {}
    ~ProblemBeforeCurrent() {removeProblem();}
    string getEntity() const {return "operation";}
    Object* getOwner() const
    {return oper ? static_cast<Object*>(oper) : static_cast<OperationPlan*>(owner);}
    const DateRange getDates() const
    {
      if (oper) return DateRange(state.start, state.end);
      OperationPlan *o = static_cast<OperationPlan*>(getOwner());
      if (o->getDates().getEnd() > Plan::instance().getCurrent())
        return DateRange(o->getDates().getStart(),
            Plan::instance().getCurrent());
      else
        return DateRange(o->getDates().getStart(),
            o->getDates().getEnd());
    }
    size_t getSize() const {return sizeof(ProblemBeforeCurrent);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    Operation* oper;
    OperationPlanState state;
};


/** @brief A problem of this class is created when an operationplan is being
  * planned before its fence date, i.e. it starts 1) before the "current"
  * date of the plan plus the release fence of the operation and 2) after the
  * current date of the plan.
  */
class ProblemBeforeFence : public Problem
{
  public:
    string getDescription() const
    {
      ostringstream ch;
      ch << "Operation '"
          << (oper ? oper : static_cast<OperationPlan*>(getOwner())->getOperation())
          << "' planned before fence";
      return ch.str();
    }
    bool isFeasible() const {return true;}
    double getWeight() const
    {return oper ? state.quantity : static_cast<OperationPlan*>(getOwner())->getQuantity();}
    explicit ProblemBeforeFence(OperationPlan* o, bool add = true)
      : Problem(o), oper(NULL)
    {if (add) addProblem();}
    explicit ProblemBeforeFence(Operation* o, Date st, Date nd, double q)
      : oper(o), state(st, nd, q) {}
    ~ProblemBeforeFence() {removeProblem();}
    string getEntity() const {return "operation";}
    Object* getOwner() const
    {return oper ? static_cast<Object*>(oper) : static_cast<OperationPlan*>(owner);}
    const DateRange getDates() const
    {
      if (oper) return DateRange(state.start, state.end);
      OperationPlan *o = static_cast<OperationPlan*>(owner);
      if (o->getDates().getEnd() > Plan::instance().getCurrent()
          + o->getOperation()->getFence())
        return DateRange(o->getDates().getStart(),
            Plan::instance().getCurrent() + o->getOperation()->getFence());
      else
        return DateRange(o->getDates().getStart(),
            o->getDates().getEnd());
    }
    size_t getSize() const {return sizeof(ProblemBeforeFence);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    Operation* oper;
    OperationPlanState state;
};


/** @brief A problem of this class is created when the sequence of two
  * operationplans in a routing isn't respected.
  */
class ProblemPrecedence : public Problem
{
  public:
    string getDescription() const
    {
      OperationPlan *o = static_cast<OperationPlan*>(getOwner());
      if (!o->nextsubopplan)
        return string("Bogus precedence problem on '")
            + o->getOperation()->getName() + "'";
      else
        return string("Operation '") + o->getOperation()->getName()
            + "' starts before operation '"
            + o->nextsubopplan->getOperation()->getName() +"' ends";
    }
    bool isFeasible() const {return false;}
    /** The weight of the problem is equal to the duration in days. */
    double getWeight() const
    {
      return static_cast<double>(getDates().getDuration()) / 86400;
    }
    explicit ProblemPrecedence(OperationPlan* o, bool add = true) : Problem(o)
    {if (add) addProblem();}
    ~ProblemPrecedence() {removeProblem();}
    string getEntity() const {return "operation";}
    Object* getOwner() const {return static_cast<OperationPlan*>(owner);}
    const DateRange getDates() const
    {
      OperationPlan *o = static_cast<OperationPlan*>(getOwner());
      return DateRange(o->nextsubopplan->getDates().getStart(),
          o->getDates().getEnd());
    }

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;
    size_t getSize() const {return sizeof(ProblemPrecedence);}
};


/** @brief A Problem of this class is created in the model when a new demand is
  * brought in the system, but it hasn't been planned yet.
  *
  * As a special case, a demand with a requested quantity of 0.0 doesn't create
  * this type of problem.
  */
class ProblemDemandNotPlanned : public Problem
{
  public:
    string getDescription() const
    {return string("Demand '") + getDemand()->getName() + "' is not planned";}
    bool isFeasible() const {return false;}
    double getWeight() const {return getDemand()->getQuantity();}
    explicit ProblemDemandNotPlanned(Demand* d, bool add = true) : Problem(d)
    {if (add) addProblem();}
    ~ProblemDemandNotPlanned() {removeProblem();}
    string getEntity() const {return "demand";}
    const DateRange getDates() const
    {return DateRange(getDemand()->getDue(),getDemand()->getDue());}
    Object* getOwner() const {return static_cast<Demand*>(owner);}
    Demand* getDemand() const {return static_cast<Demand*>(owner);}
    size_t getSize() const {return sizeof(ProblemDemandNotPlanned);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;
};


/** @brief A problem of this class is created when a demand is satisfied later
  * than the accepted tolerance after its due date.
  */
class ProblemLate : public Problem
{
  public:
    DECLARE_EXPORT string getDescription() const;
    bool isFeasible() const {return true;}

    /** The weight is equal to the delay, expressed in days.<br>
      * The quantity being delayed is not included.
      */
    double getWeight() const
    {
      assert(getDemand() && !getDemand()->getDelivery().empty());
      return static_cast<double>(DateRange(
          getDemand()->getDue(),
          getDemand()->getLatestDelivery()->getDates().getEnd()
          ).getDuration()) / 86400;
    }

    /** Constructor. */
    explicit ProblemLate(Demand* d, bool add = true) : Problem(d)
    {if (add) addProblem();}

    /** Destructor. */
    ~ProblemLate() {removeProblem();}

    const DateRange getDates() const
    {
      assert(getDemand() && !getDemand()->getDelivery().empty());
      return DateRange(getDemand()->getDue(),
          getDemand()->getLatestDelivery()->getDates().getEnd());
    }
    Demand* getDemand() const {return static_cast<Demand*>(getOwner());}
    size_t getSize() const {return sizeof(ProblemLate);}
    string getEntity() const {return "demand";}
    Object* getOwner() const {return static_cast<Demand*>(owner);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;
};


/** @brief A problem of this class is created when a demand is planned earlier
  * than the accepted tolerance before its due date.
  */
class ProblemEarly : public Problem
{
  public:
    DECLARE_EXPORT string getDescription() const;
    bool isFeasible() const {return true;}
    double getWeight() const
    {
      assert(getDemand() && !getDemand()->getDelivery().empty());
      return static_cast<double>(DateRange(
          getDemand()->getDue(),
          getDemand()->getEarliestDelivery()->getDates().getEnd()
          ).getDuration()) / 86400;
    }
    explicit ProblemEarly(Demand* d, bool add = true) : Problem(d)
    {if (add) addProblem();}
    ~ProblemEarly() {removeProblem();}
    string getEntity() const {return "demand";}
    Object* getOwner() const {return static_cast<Demand*>(owner);}
    const DateRange getDates() const
    {
      assert(getDemand() && !getDemand()->getDelivery().empty());
      return DateRange(getDemand()->getDue(),
          getDemand()->getEarliestDelivery()->getDates().getEnd());
    }
    Demand* getDemand() const {return static_cast<Demand*>(getOwner());}
    size_t getSize() const {return sizeof(ProblemEarly);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;
};


/** @brief A Problem of this class is created in the model when a data exception
  * prevents planning of certain objects
  */
class ProblemInvalidData : public Problem
{
  public:
    string getDescription() const {return description;}
    bool isFeasible() const {return false;}
    double getWeight() const {return qty;}
    explicit ProblemInvalidData(HasProblems* o, string d, string e,
        Date st, Date nd, double q, bool add = true)
      : Problem(o), description(d), entity(e), dates(st,nd), qty(q)
    {if (add) addProblem();}
    ~ProblemInvalidData() {removeProblem();}
    string getEntity() const {return entity;}
    const DateRange getDates() const {return dates;}
    Object* getOwner() const
    {
      if (entity == "demand") return static_cast<Demand*>(owner);
      if (entity == "buffer") return static_cast<Buffer*>(owner);
      if (entity == "resource") return static_cast<Resource*>(owner);
      if (entity == "operation") return static_cast<Operation*>(owner);
      throw LogicException("Unknown problem entity type");
    }
    size_t getSize() const
    {return sizeof(ProblemInvalidData) + description.size() + entity.size();}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    /** Description of the data issue. */
    string description;
    string entity;
    DateRange dates;
    double qty;
};


/** @brief A problem of this class is created when a demand is planned for less than
  * the requested quantity.
  */
class ProblemShort : public Problem
{
  public:
    string getDescription() const
    {
      ostringstream ch;
      ch << "Demand '" << getDemand()->getName() << "' planned "
          << (getDemand()->getQuantity() - getDemand()->getPlannedQuantity())
          << " units short";
      return ch.str();
    }
    bool isFeasible() const {return true;}
    double getWeight() const
    {return getDemand()->getQuantity() - getDemand()->getPlannedQuantity();}
    explicit ProblemShort(Demand* d, bool add = true) : Problem(d)
    {if (add) addProblem();}
    ~ProblemShort() {removeProblem();}
    string getEntity() const {return "demand";}
    const DateRange getDates() const
    {return DateRange(getDemand()->getDue(), getDemand()->getDue());}
    Object* getOwner() const {return static_cast<Demand*>(owner);}
    Demand* getDemand() const {return static_cast<Demand*>(owner);}
    size_t getSize() const {return sizeof(ProblemShort);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;
};


/** @brief A problem of this class is created when a demand is planned for more
  * than the requested quantity.
  */
class ProblemExcess : public Problem
{
  public:
    string getDescription() const
    {
      ostringstream ch;
      ch << "Demand '" << getDemand()->getName() << "' planned "
          << (getDemand()->getPlannedQuantity() - getDemand()->getQuantity())
          << " units excess";
      return ch.str();
    }
    bool isFeasible() const {return true;}
    double getWeight() const
    {return getDemand()->getPlannedQuantity() - getDemand()->getQuantity();}
    explicit ProblemExcess(Demand* d, bool add = true) : Problem(d)
    {if (add) addProblem();}
    string getEntity() const {return "demand";}
    Object* getOwner() const {return static_cast<Demand*>(owner);}
    ~ProblemExcess() {removeProblem();}
    const DateRange getDates() const
    {return DateRange(getDemand()->getDue(), getDemand()->getDue());}
    Demand* getDemand() const {return static_cast<Demand*>(getOwner());}
    size_t getSize() const {return sizeof(ProblemExcess);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;
};


/** @brief A problem of this class is created when a resource is being
  * overloaded during a certain period of time.
  */
class ProblemCapacityOverload : public Problem
{
  public:
    DECLARE_EXPORT string getDescription() const;
    bool isFeasible() const {return false;}
    double getWeight() const {return qty;}
    ProblemCapacityOverload(Resource* r, Date st, Date nd, double q, bool add = true)
      : Problem(r), qty(q), dr(st,nd) {if (add) addProblem();}
    ~ProblemCapacityOverload() {removeProblem();}
    string getEntity() const {return "capacity";}
    Object* getOwner() const {return static_cast<Resource*>(owner);}
    const DateRange getDates() const {return dr;}
    Resource* getResource() const {return static_cast<Resource*>(getOwner());}
    size_t getSize() const {return sizeof(ProblemCapacityOverload);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    /** Overload quantity. */
    double qty;

    /** The daterange of the problem. */
    DateRange dr;
};


/** @brief A problem of this class is created when a resource is loaded below
  * its minimum during a certain period of time.
  */
class ProblemCapacityUnderload : public Problem
{
  public:
    DECLARE_EXPORT string getDescription() const;
    bool isFeasible() const {return true;}
    double getWeight() const {return qty;}
    ProblemCapacityUnderload(Resource* r, DateRange d, double q, bool add = true)
      : Problem(r), qty(q), dr(d) {if (add) addProblem();}
    ~ProblemCapacityUnderload() {removeProblem();}
    string getEntity() const {return "capacity";}
    Object* getOwner() const {return static_cast<Resource*>(owner);}
    const DateRange getDates() const {return dr;}
    Resource* getResource() const {return static_cast<Resource*>(getOwner());}
    size_t getSize() const {return sizeof(ProblemCapacityUnderload);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    /** Underload quantity. */
    double qty;

    /** The daterange of the problem. */
    DateRange dr;
};


/** @brief A problem of this class is created when a buffer is having a
  * material shortage during a certain period of time.
  */
class ProblemMaterialShortage : public Problem
{
  public:
    DECLARE_EXPORT string getDescription() const;
    bool isFeasible() const {return false;}
    double getWeight() const {return qty;}
    ProblemMaterialShortage(Buffer* b, Date st, Date nd, double q, bool add = true)
      : Problem(b), qty(q), dr(st,nd) {if (add) addProblem();}
    string getEntity() const {return "material";}
    Object* getOwner() const {return static_cast<Buffer*>(owner);}
    ~ProblemMaterialShortage() {removeProblem();}
    const DateRange getDates() const {return dr;}
    Buffer* getBuffer() const {return static_cast<Buffer*>(getOwner());}
    size_t getSize() const {return sizeof(ProblemMaterialShortage);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    /** Shortage quantity. */
    double qty;

    /** The daterange of the problem. */
    DateRange dr;
};


/** @brief A problem of this class is created when a buffer is carrying too
  * much material during a certain period of time.
  */
class ProblemMaterialExcess : public Problem
{
  public:
    DECLARE_EXPORT string getDescription() const;
    bool isFeasible() const {return true;}
    double getWeight() const {return qty;}
    ProblemMaterialExcess(Buffer* b, Date st, Date nd, double q, bool add = true)
      : Problem(b), qty(q), dr(st,nd) {if (add) addProblem();}
    string getEntity() const {return "material";}
    ~ProblemMaterialExcess() {removeProblem();}
    const DateRange getDates() const {return dr;}
    Object* getOwner() const {return static_cast<Buffer*>(owner);}
    Buffer* getBuffer() const {return static_cast<Buffer*>(owner);}
    size_t getSize() const {return sizeof(ProblemMaterialExcess);}

    /** Return a reference to the metadata structure. */
    const MetaClass& getType() const {return *metadata;}

    /** Storing metadata on this class. */
    static DECLARE_EXPORT const MetaClass* metadata;

  private:
    /** Excess quantity. */
    double qty;

    /** The daterange of the problem. */
    DateRange dr;
};


/** @brief This command is used to create an operationplan.
  *
  * The operationplan will have its loadplans and flowplans created when the
  * command is created. It is assigned an id and added to the list of all
  * operationplans when the command is committed.
  */
class CommandCreateOperationPlan : public Command
{
  public:
    /** Constructor. */
    CommandCreateOperationPlan
    (const Operation* o, double q, Date d1, Date d2, Demand* l,
     OperationPlan* ow=NULL, bool makeflowsloads=true)
    {
      opplan = o ?
          o->createOperationPlan(q, d1, d2, l, ow, 0, makeflowsloads)
          : NULL;
    }
    void commit()
    {
      if (opplan)
      {
        opplan->activate();
        opplan = NULL; // Avoid executing / initializing more than once
      }
    }
    virtual void rollback() {delete opplan; opplan = NULL;}
    virtual void undo() {if (opplan) opplan->deleteFlowLoads();}
    virtual void redo() {if (opplan) opplan->createFlowLoads();}
    virtual ~CommandCreateOperationPlan() {if (opplan) delete opplan;}
    OperationPlan *getOperationPlan() const {return opplan;}

  private:
    /** Pointer to the newly created operationplan. */
    OperationPlan *opplan;
};


/** @brief This command is used to delete an operationplan. */
class CommandDeleteOperationPlan : public Command
{
  public:
    /** Constructor. */
    DECLARE_EXPORT CommandDeleteOperationPlan(OperationPlan* o);
    virtual void commit()
    {
      if (opplan) delete opplan;
      opplan = NULL;
    }
    virtual void undo()
    {
      if (!opplan) return;
      opplan->createFlowLoads();
      opplan->insertInOperationplanList();
      if (opplan->getDemand())
        opplan->getDemand()->addDelivery(opplan);
      for (OperationPlan::iterator x(opplan); x != OperationPlan::end(); x++)
      {
        x->createFlowLoads();
        x->insertInOperationplanList();
      }
    }
    virtual void redo()
    {
      if (!opplan) return;
      opplan->deleteFlowLoads();
      opplan->removeFromOperationplanList();
      if (opplan->getDemand())
        opplan->getDemand()->removeDelivery(opplan);
      for (OperationPlan::iterator x(opplan); x != OperationPlan::end(); x++)
      {
        x->deleteFlowLoads();
        x->removeFromOperationplanList();
      }
    }
    virtual void rollback()
    {
      undo();
      opplan = NULL;
    }
    virtual ~CommandDeleteOperationPlan() {undo();}

  private:
    /** Pointer to the operationplan being deleted.<br>
      * Until the command is committed we don't deallocate the memory for the
      * operationplan, but only remove all pointers to it from various places.
      */
    OperationPlan *opplan;
};


/** @brief This class represents the command of moving an operationplan to a
  * new date and/or resizing it.
  * @todo Moving in a routing operation can't be undone with the current
  * implementation! The command will need to store all original dates of
  * the suboperationplans...
  */
class CommandMoveOperationPlan : public Command
{
  public:
    /** Constructor.<br>
      * Unlike most other commands the constructor already executes the change.
      * @param opplanptr Pointer to the operationplan being moved.
      * @param newStart New start date of the operationplan.
      * @param newEnd New end date of the operationplan.
      * @param newQty New quantity of the operationplan.The default is -1,
      * which indicates to leave the quantity unchanged.
      */
    DECLARE_EXPORT CommandMoveOperationPlan(OperationPlan* opplanptr,
        Date newStart, Date newEnd, double newQty = -1.0);

    /** Default constructor. */
    DECLARE_EXPORT CommandMoveOperationPlan(OperationPlan*);

    /** Commit the changes. */
    virtual void commit() {opplan=NULL;}

    /** Undo the changes. */
    virtual void rollback() {restore(true); opplan = NULL;}

    virtual void undo() {restore(false);}
    virtual DECLARE_EXPORT void redo();

    /** Undo the changes.<br>
      * When the argument is true, subcommands for suboperationplans are deleted. */
    DECLARE_EXPORT void restore(bool = false);

    /** Destructor. */
    virtual ~CommandMoveOperationPlan() {if (opplan) rollback();}

    /** Returns the operationplan being manipulated. */
    OperationPlan* getOperationPlan() const {return opplan;}

    /** Set another start date for the operationplan. */
    void setStart(Date d) {if (opplan) opplan->setStart(d);}

    /** Set another start date, end date and quantity for the operationplan. */
    void setParameters(Date s, Date e, double q, bool b)
    {
      assert(opplan->getOperation());
      if (opplan)
        opplan->getOperation()->setOperationPlanParameters(opplan, q, s, e, b);
    }

    /** Set another start date for the operationplan. */
    void setEnd(Date d) {if (opplan) opplan->setEnd(d);}

    /** Set another quantity for the operationplan. */
    void setQuantity(double q) {if (opplan) opplan->setQuantity(q);}

    /** Return the quantity of the original operationplan. */
    double getQuantity() const {return originalqty; }

    /** Return the dates of the original operationplan. */
    DateRange getDates() const {return originaldates;}

  private:
    /** This is a pointer to the operationplan being moved. */
    OperationPlan *opplan;

    /** These are the original dates of the operationplan before its move. */
    DateRange originaldates;

    /** This is the quantity of the operationplan before the command. */
    double originalqty;

    /** A pointer to a list of suboperationplan commands. */
    Command* firstCommand;
};


/** @brief This class models a iterator that walks over all available
  * HasProblem entities.
  *
  * This list is containing hard-coding the classes that are implementing
  * this class. It's not ideal, but we don't have an explicit container
  * of the objects (and we don't want one either) and this allows us also
  * to re-use the sorting used for the container classes.
  */
class HasProblems::EntityIterator
{
  private:
    /** This union contains iterators through the different entity types.
      * Only one of the different iterators will be active at a time, and
      * can thus save memory by collapsing the iterators into a single
      * union. */
    union
    {
      Buffer::iterator *bufIter;
      Resource::iterator *resIter;
      OperationPlan::iterator *operIter;
      Demand::iterator *demIter;
    };

    /** This type indicates which type of entity we are currently recursing
      * through.
      *  - 0: buffers
      *  - 1: resources
      *  - 2: operationplans
      *  - 3: demands
      */
    unsigned short type;

  public:
    /** Default constructor, which creates an iterator to the first
      * HasProblems object. */
    explicit DECLARE_EXPORT EntityIterator();

    /** Used to create an iterator pointing beyond the last HasProblems
      * object. */
    explicit EntityIterator(unsigned short i) : type(i) {}

    /** Copy constructor. */
    DECLARE_EXPORT EntityIterator(const EntityIterator& o);

    /** Assignment operator. */
    DECLARE_EXPORT EntityIterator& operator=(const EntityIterator& o);

    /** Destructor. */
    DECLARE_EXPORT ~EntityIterator();

    /** Pre-increment operator. */
    DECLARE_EXPORT EntityIterator& operator++();

    /** Inequality operator.<br>
      * Two iterators are different when they point to different objects.
      */
    DECLARE_EXPORT bool operator != (const EntityIterator& t) const;

    /** Equality operator.<br>
      * Two iterators are equal when they point to the same object.
      */
    bool operator == (const EntityIterator& t) const {return !(*this != t);}

    /** Dereference operator. */
    DECLARE_EXPORT HasProblems& operator*() const;

    /** Dereference operator. */
    DECLARE_EXPORT HasProblems* operator->() const;
};


/** @brief This class models an STL-like iterator that allows us to iterate
  * over the named entities in a simple and safe way.
  *
  * Objects of this class are returned by the begin() and end() functions.
  * @see Problem::begin()
  * @see Problem::begin(HasProblem*)
  * @see Problem::end()
  */
class Problem::const_iterator
{
    friend class Problem;
  private:
    /** A pointer to the current problem. If this pointer is NULL, we are
      * at the end of the list. */
    Problem* iter;
    HasProblems* owner;
    HasProblems::EntityIterator eiter;

  public:
    /** Creates an iterator that will loop through the problems of a
      * single entity only. <BR>
      * This constructor is also used to create a end-iterator, when passed
      * a NULL pointer as argument.
      */
    explicit const_iterator(HasProblems* o) : iter(o ? o->firstProblem : NULL),
      owner(o), eiter(4) {}

    /** Creates an iterator that will loop through the constraints of
      * a demand.
      */
    explicit const_iterator(Problem* o) : iter(o),
      owner(NULL), eiter(4) {}

    /** Creates an iterator that will loop through the problems of all
      * entities. */
    explicit const_iterator() : owner(NULL)
    {
      // Loop till we find an entity with a problem
      while (eiter!=HasProblems::endEntity() && !(eiter->firstProblem))
        ++eiter;
      // Found a first problem, or no problem at all
      iter = (eiter!=HasProblems::endEntity()) ? eiter->firstProblem : NULL;
    }

    /** Pre-increment operator. */
    DECLARE_EXPORT const_iterator& operator++();

    /** Inequality operator. */
    bool operator != (const const_iterator& t) const {return iter!=t.iter;}

    /** Equality operator. */
    bool operator == (const const_iterator& t) const {return iter==t.iter;}

    Problem& operator*() const {return *iter;}
    Problem* operator->() const {return iter;}
};


/** Retrieve an iterator for the list. */
inline Problem::const_iterator Problem::List::begin() const
{return Problem::const_iterator(first);}


/** Stop iterator. */
inline Problem::const_iterator Problem::List::end() const
{return Problem::const_iterator(static_cast<Problem*>(NULL));}


/** @brief This class allows upstream and downstream navigation through
  * the plan.
  *
  * Downstream navigation follows the material flow from raw materials
  * towards the produced end item.<br>
  * Upstream navigation traces back the material flow from the end item up to
  * the consumed raw materials.<br>
  * The class is implemented as an STL-like iterator.
  *
  * @todo operationplans without flowplans are skipped by the iterator - not correct!
  */
class PeggingIterator : public Object
{
  public:
    /** Constructor. */
    DECLARE_EXPORT PeggingIterator(const Demand*);

    /** Constructor. */
    DECLARE_EXPORT PeggingIterator(const OperationPlan*, bool = true);

    /** Constructor. */
    PeggingIterator(const FlowPlan* e, bool b = true)
      : first(true), downstream(b), firstIteration(true)
    {
      if (!e) return;
      if (downstream)
        states.push(state(0,abs(e->getQuantity()),1.0,e,NULL,NULL));
      else
        states.push(state(0,abs(e->getQuantity()),1.0,NULL,e,NULL));
      initType(metadata);
    }

    /** Return the operationplan consuming the material. */
    OperationPlan* getConsumingOperationplan() const
    {
      const FlowPlan* x = states.top().cons_flowplan;
      return x ? x->getOperationPlan() : NULL;
    }

    /** Return the material buffer through which we are pegging. */
    Buffer *getBuffer() const
    {
      const FlowPlan* x = states.top().prod_flowplan;
      if (!x) x = states.top().cons_flowplan;
      return x ? x->getFlow()->getBuffer() : NULL;
    }

    /** Return the operationplan producing the material. */
    OperationPlan* getProducingOperationplan() const
    {
      const FlowPlan* x = states.top().prod_flowplan;
      if (x) return x->getOperationPlan();
      return const_cast<OperationPlan*>(states.top().opplan);
    }

    /** Return the date when the material is consumed. */
    Date getConsumingDate() const
    {
      const FlowPlan* x = states.top().cons_flowplan;
      return x ? x->getDate() : Date::infinitePast;
    }

    /** Return the date when the material is produced. */
    Date getProducingDate() const
    {
      const FlowPlan* x = states.top().prod_flowplan;
      if (x) return x->getDate();
      const OperationPlan* y = states.top().opplan;
      return y ? y->getDates().getEnd() : Date::infinitePast;
    }

    /** Returns the recursion depth of the iterator.<br>
      * The original flowplan is at level 0, and each level (either upstream
      * or downstream) increments the value by 1.
      */
    short getLevel() const {return states.top().level;}

    /** Returns the quantity of the demand that is linked to this pegging
      * record.
      */
    double getQuantityDemand() const {return states.top().qty;}

    /** Returns the quantity of the buffer flowplans that is linked to this
      * pegging record.
      */
    double getQuantityBuffer() const
    {
      const state& t = states.top();
      return t.prod_flowplan
          ? t.factor * t.prod_flowplan->getOperationPlan()->getQuantity()
          : 0;
    }

    /** Returns which portion of the current flowplan is fed/supplied by the
      * original flowplan. */
    double getFactor() const {return states.top().factor;}

    /** Returns false if the flowplan remained unpegged, i.e. it wasn't
      * -either completely or paritally- unconsumed at the next level.
      */
    bool getPegged() const {return states.top().pegged;}

    /** Move the iterator foward to the next downstream flowplan. */
    DECLARE_EXPORT PeggingIterator& operator++();

    /** Move the iterator foward to the next downstream flowplan.<br>
      * This post-increment operator is less efficient than the pre-increment
      * operator.
      */
    PeggingIterator operator++(int)
    {PeggingIterator tmp = *this; ++*this; return tmp;}

    /** Move the iterator foward to the next upstream flowplan. */
    DECLARE_EXPORT PeggingIterator& operator--();

    /** Move the iterator foward to the next upstream flowplan.<br>
      * This post-increment operator is less efficient than the pre-decrement
      * operator.
      */
    PeggingIterator operator--(int)
    {PeggingIterator tmp = *this; --*this; return tmp;}

    /** Comparison operator. */
    bool operator==(const PeggingIterator& x) const {return states == x.states;}

    /** Inequality operator. */
    bool operator!=(const PeggingIterator& x) const {return states != x.states;}

    /** Conversion operator to a boolean value.
      * The return value is true when the iterator still has next elements to
      * explore. Returns false when the iteration is finished.
      */
    operator bool () const {return !states.empty();}

    /** Update the stack. */
    DECLARE_EXPORT void updateStack(short, double, double, const FlowPlan*, const FlowPlan*, bool = true);
    DECLARE_EXPORT void updateStack(short, double, double, const OperationPlan*, bool = true);

    /** Returns true if this is a downstream iterator. */
    bool isDownstream() {return downstream;}

    /** Initialize the class. */
    static int initialize();

    virtual void endElement(XMLInput& i, const Attribute& a, const DataElement& d)
    {
      throw LogicException("Pegging can't be read");
    }
    virtual const MetaClass& getType() const {return *metadata;}
    static DECLARE_EXPORT const MetaCategory* metadata;
    size_t getSize() const {return sizeof(PeggingIterator);}

  private:
    /** This structure is used to keep track of the iterator states during the
      * iteration. */
    struct state
    {
      /** Stores the quantity of this flowplan that is involved. */
      double qty;

      /** Stores what portion of the flowplan is involved with the root flowplan
        * where the recursion started.
        */
      double factor;

      /** Keeps track of the number of levels we're removed from the root
        * flowplan where the recursion started.
        */
      short level;

      /** The current flowplan. */
      const FlowPlan* cons_flowplan;

      /** The current flowplan. */
      const FlowPlan* prod_flowplan;

      /** Set to false when unpegged quantities are involved. */
      bool pegged;

      /** A pointer to an operationplan.
        * Used only when no flowplan at all is found to represent the
        * pegging. */
      const OperationPlan* opplan;

      /** Constructor. */
      state(unsigned int l, double d, double f,
          const FlowPlan* fc, const FlowPlan* fp,
          const OperationPlan* op, bool p = true)
        : qty(d), factor(f), level(l),
          cons_flowplan(fc), prod_flowplan(fp), pegged(p),
          opplan(op) {};

      /** Inequality operator. */
      bool operator != (const state& s) const
      {
        return cons_flowplan != s.cons_flowplan
            || prod_flowplan != s.prod_flowplan
            || level != s.level
            || opplan != s.opplan;
      }

      /** Equality operator. */
      bool operator == (const state& s) const
      {
        return cons_flowplan == s.cons_flowplan
            && prod_flowplan == s.prod_flowplan
            && level == s.level
            && opplan == s.opplan;
      }
    };

    /** A type to hold the iterator state. */
    typedef stack < state > statestack;

    /** A stack is used to store the iterator state. */
    statestack states;

    /** Iterate over the pegging in Python. */
    DECLARE_EXPORT PyObject *iternext();

    DECLARE_EXPORT PyObject* getattro(const Attribute&);

    /* Auxilary function to make recursive code possible. */
    DECLARE_EXPORT void followPegging(const OperationPlan*, short, double, double);

    /** Convenience variable during stack updates.
      * Depending on the value of this field, either the top element in the
      * stack is updated or a new state is pushed on the stack.
      */
    bool first;

    /** Downstream or upstream iterator. */
    bool downstream;

    /** A flag used by the Python iterators.
      * @see iternext()
      */
    bool firstIteration;
};


/** @brief An iterator class to go through all flowplans of an operationplan.
  * @see OperationPlan::beginFlowPlans
  * @see OperationPlan::endFlowPlans
  */
class OperationPlan::FlowPlanIterator
{
    friend class OperationPlan;
  private:
    FlowPlan* curflowplan;
    FlowPlan* prevflowplan;
    FlowPlanIterator(FlowPlan* b) : curflowplan(b), prevflowplan(NULL) {}
  public:
    FlowPlanIterator(const FlowPlanIterator& b)
    {
      curflowplan = b.curflowplan;
      prevflowplan = b.prevflowplan;
    }
    bool operator != (const FlowPlanIterator &b) const
    {return b.curflowplan != curflowplan;}
    bool operator == (const FlowPlanIterator &b) const
    {return b.curflowplan == curflowplan;}
    FlowPlanIterator& operator++()
    {
      prevflowplan = curflowplan;
      if (curflowplan) curflowplan = curflowplan->nextFlowPlan;
      return *this;
    }
    FlowPlanIterator operator++(int)
    {FlowPlanIterator tmp = *this; ++*this; return tmp;}
    FlowPlan* operator ->() const {return curflowplan;}
    FlowPlan& operator *() const {return *curflowplan;}
    void deleteFlowPlan()
    {
      if (!curflowplan) return;
      if (prevflowplan) prevflowplan->nextFlowPlan = curflowplan->nextFlowPlan;
      else curflowplan->oper->firstflowplan = curflowplan->nextFlowPlan;
      FlowPlan* tmp = curflowplan;
      // Move the iterator to the next element
      curflowplan = curflowplan->nextFlowPlan;
      delete tmp;
    }
};

inline OperationPlan::FlowPlanIterator OperationPlan::beginFlowPlans() const
{return OperationPlan::FlowPlanIterator(firstflowplan);}

inline OperationPlan::FlowPlanIterator OperationPlan::endFlowPlans() const
{return OperationPlan::FlowPlanIterator(NULL);}

inline int OperationPlan::sizeFlowPlans() const
{
  int c = 0;
  for (FlowPlanIterator i = beginFlowPlans(); i != endFlowPlans(); ++i) ++c;
  return c;
}


/** @brief An iterator class to go through all loadplans of an operationplan.
  * @see OperationPlan::beginLoadPlans
  * @see OperationPlan::endLoadPlans
  */
class OperationPlan::LoadPlanIterator
{
    friend class OperationPlan;
  private:
    LoadPlan* curloadplan;
    LoadPlan* prevloadplan;
    LoadPlanIterator(LoadPlan* b) : curloadplan(b), prevloadplan(NULL) {}
  public:
    LoadPlanIterator(const LoadPlanIterator& b)
    {
      curloadplan = b.curloadplan;
      prevloadplan = b.prevloadplan;
    }
    bool operator != (const LoadPlanIterator &b) const
    {return b.curloadplan != curloadplan;}
    bool operator == (const LoadPlanIterator &b) const
    {return b.curloadplan == curloadplan;}
    LoadPlanIterator& operator++()
    {
      prevloadplan = curloadplan;
      if (curloadplan) curloadplan = curloadplan->nextLoadPlan;
      return *this;
    }
    LoadPlanIterator operator++(int)
    {LoadPlanIterator tmp = *this; ++*this; return tmp;}
    LoadPlan* operator ->() const {return curloadplan;}
    LoadPlan& operator *() const {return *curloadplan;}
    void deleteLoadPlan()
    {
      if (!curloadplan) return;
      if (prevloadplan) prevloadplan->nextLoadPlan = curloadplan->nextLoadPlan;
      else curloadplan->oper->firstloadplan = curloadplan->nextLoadPlan;
      LoadPlan* tmp = curloadplan;
      // Move the iterator to the next element
      curloadplan = curloadplan->nextLoadPlan;
      delete tmp;
    }
};


inline OperationPlan::LoadPlanIterator OperationPlan::beginLoadPlans() const
{return OperationPlan::LoadPlanIterator(firstloadplan);}


inline OperationPlan::LoadPlanIterator OperationPlan::endLoadPlans() const
{return OperationPlan::LoadPlanIterator(NULL);}


inline int OperationPlan::sizeLoadPlans() const
{
  int c = 0;
  for (LoadPlanIterator i = beginLoadPlans(); i != endLoadPlans(); ++i) ++c;
  return c;
}


class ProblemIterator
  : public FreppleIterator<ProblemIterator,Problem::const_iterator,Problem>
{
  public:
    /** Constructor starting the iteration from a certain problem. */
    ProblemIterator(Problem *x) :
      FreppleIterator<ProblemIterator,Problem::const_iterator,Problem>(x) {}

    /** Constructor starting the iteration from a certain problem. */
    ProblemIterator(Problem &x) :
      FreppleIterator<ProblemIterator,Problem::const_iterator,Problem>(&x) {}

    /** Default constructor. */
    ProblemIterator() :
      FreppleIterator<ProblemIterator,Problem::const_iterator,Problem>() {}
};


class BufferIterator
  : public FreppleIterator<BufferIterator,Buffer::memberIterator,Buffer>
{
  public:
    BufferIterator(Buffer* b) : FreppleIterator<BufferIterator,Buffer::memberIterator,Buffer>(b) {}
    BufferIterator() {}
};


class LocationIterator
  : public FreppleIterator<LocationIterator,Location::memberIterator,Location>
{
  public:
    LocationIterator(Location* b) : FreppleIterator<LocationIterator,Location::memberIterator,Location>(b) {}
    LocationIterator() {}
};


class CustomerIterator
  : public FreppleIterator<CustomerIterator,Customer::memberIterator,Customer>
{
  public:
    CustomerIterator(Customer* b) : FreppleIterator<CustomerIterator,Customer::memberIterator,Customer>(b) {}
    CustomerIterator() {}
};


class ItemIterator
  : public FreppleIterator<ItemIterator,Item::memberIterator,Item>
{
  public:
    ItemIterator(Item* b) : FreppleIterator<ItemIterator,Item::memberIterator,Item>(b) {}
    ItemIterator() {}
};


class DemandIterator
  : public FreppleIterator<DemandIterator,Demand::memberIterator,Demand>
{
  public:
    DemandIterator(Demand* b) : FreppleIterator<DemandIterator,Demand::memberIterator,Demand>(b) {}
    DemandIterator() {}
};


class ResourceIterator
  : public FreppleIterator<ResourceIterator,Resource::memberIterator,Resource>
{
  public:
    ResourceIterator(Resource* b) : FreppleIterator<ResourceIterator,Resource::memberIterator,Resource>(b) {}
    ResourceIterator() {}
};


class SolverIterator
  : public FreppleIterator<SolverIterator,Solver::iterator,Solver>
{
};


class OperationIterator
  : public FreppleIterator<OperationIterator,Operation::iterator,Operation>
{
};


class CalendarIterator
  : public FreppleIterator<CalendarIterator,Calendar::iterator,Calendar>
{
};


class SetupMatrixIterator
  : public FreppleIterator<SetupMatrixIterator,SetupMatrix::iterator,SetupMatrix>
{
};


class SkillIterator
  : public FreppleIterator<SkillIterator,Skill::iterator,Skill>
{
};


//
// SETUP MATRIX RULES
//


class SetupMatrixRuleIterator : public PythonExtension<SetupMatrixRuleIterator>
{
  public:
    static int initialize();

    SetupMatrixRuleIterator(SetupMatrix* c) : matrix(c)
    {
      if (!c)
        throw LogicException("Creating rule iterator for NULL matrix");
      currule = c->beginRules();
    }

  private:
    SetupMatrix* matrix;
    SetupMatrix::RuleIterator currule;
    PyObject *iternext();
};


//
// RESOURCE SKILLS
//

class ResourceSkillIterator : public PythonExtension<ResourceSkillIterator>
{
  public:
    static int initialize();

    ResourceSkillIterator(Resource* r)
      : res(r), ir(r ? r->getSkills().begin() : NULL), skill(NULL), is(NULL)
    {
      if (!r)
        throw LogicException("Creating resourceskill iterator for NULL resource");
    }

    ResourceSkillIterator(Skill* s)
      : res(NULL), ir(NULL), skill(s), is(s ? s->getResources().begin() : NULL)
    {
      if (!s)
        throw LogicException("Creating resourceskill iterator for NULL skill");
    }

  private:
    Resource* res;
    Resource::skilllist::const_iterator ir;
    Skill* skill;
    Skill::resourcelist::const_iterator is;
    PyObject *iternext();
};


//
// CALENDARS
//


class CalendarBucketIterator : public PythonExtension<CalendarBucketIterator>
{
  public:
    static int initialize();

    CalendarBucketIterator(Calendar* c) : cal(c)
    {
      if (!c)
        throw LogicException("Creating bucket iterator for NULL calendar");
      i = c->beginBuckets();
    }

  private:
    Calendar* cal;
    Calendar::BucketIterator i;
    PyObject *iternext();
};


class CalendarEventIterator
  : public PythonExtension<CalendarEventIterator>
{
  public:
    static int initialize();

    CalendarEventIterator(Calendar* c, Date d=Date::infinitePast, bool f=true)
      : cal(c), eventiter(c,d,f), forward(f) {}

  private:
    Calendar* cal;
    Calendar::EventIterator eventiter;
    bool forward;
    PyObject *iternext();
};


//
// OPERATIONPLANS
//


class OperationPlanIterator
  : public FreppleIterator<OperationPlanIterator,OperationPlan::iterator,OperationPlan>
{
  public:
    /** Constructor to iterate over all operationplans. */
    OperationPlanIterator() {}

    /** Constructor to iterate over the operationplans of a single operation. */
    OperationPlanIterator(Operation* o)
      : FreppleIterator<OperationPlanIterator,OperationPlan::iterator,OperationPlan>(o)
    {}

    /** Constructor to iterate over the suboperationplans of an operationplans. */
    OperationPlanIterator(OperationPlan* opplan)
      : FreppleIterator<OperationPlanIterator,OperationPlan::iterator,OperationPlan>(opplan)
    {}
};


//
// FLOWPLANS
//


class FlowPlanIterator : public PythonExtension<FlowPlanIterator>
{
  public:
    /** Registration of the Python class and its metadata. */
    static int initialize();

    /** Constructor to iterate over the flowplans of a buffer. */
    FlowPlanIterator(Buffer* b) : buf(b), buffer_or_opplan(true)
    {
      if (!b)
        throw LogicException("Creating flowplan iterator for NULL buffer");
      bufiter = new Buffer::flowplanlist::const_iterator(b->getFlowPlans().begin());
    }

    /** Constructor to iterate over the flowplans of an operationplan. */
    FlowPlanIterator(OperationPlan* o) : opplan(o), buffer_or_opplan(false)
    {
      if (!o)
        throw LogicException("Creating flowplan iterator for NULL operationplan");
      opplaniter = new OperationPlan::FlowPlanIterator(o->beginFlowPlans());
    }

    ~FlowPlanIterator()
    {
      if (buffer_or_opplan) delete bufiter;
      else delete opplaniter;
    }

  private:
    union
    {
      Buffer* buf;
      OperationPlan* opplan;
    };

    union
    {
      Buffer::flowplanlist::const_iterator *bufiter;
      OperationPlan::FlowPlanIterator *opplaniter;
    };

    /** Flags whether we are browsing over the flowplans in a buffer or in an
      * operationplan. */
    bool buffer_or_opplan;

    PyObject *iternext();
};


//
// LOADPLANS
//


class LoadPlanIterator : public PythonExtension<LoadPlanIterator>
{
  public:
    static int initialize();

    LoadPlanIterator(Resource* r) : res(r), resource_or_opplan(true)
    {
      if (!r)
        throw LogicException("Creating loadplan iterator for NULL resource");
      resiter = new Resource::loadplanlist::const_iterator(r->getLoadPlans().begin());
    }

    LoadPlanIterator(OperationPlan* o) : opplan(o), resource_or_opplan(false)
    {
      if (!opplan)
        throw LogicException("Creating loadplan iterator for NULL operationplan");
      opplaniter = new OperationPlan::LoadPlanIterator(o->beginLoadPlans());
    }

    ~LoadPlanIterator()
    {
      if (resource_or_opplan) delete resiter;
      else delete opplaniter;
    }

  private:
    union
    {
      Resource* res;
      OperationPlan* opplan;
    };

    union
    {
      Resource::loadplanlist::const_iterator *resiter;
      OperationPlan::LoadPlanIterator *opplaniter;
    };

    /** Flags whether we are browsing over the flowplans in a buffer or in an
      * operationplan. */
    bool resource_or_opplan;

    PyObject *iternext();
};


//
// DEMAND DELIVERY OPERATIONPLANS
//


class DemandPlanIterator : public PythonExtension<DemandPlanIterator>
{
  public:
    static int initialize();

    DemandPlanIterator(Demand* r) : dem(r)
    {
      if (!r)
        throw LogicException("Creating demandplan iterator for NULL demand");
      i = r->getDelivery().begin();
    }

  private:
    Demand* dem;
    Demand::OperationPlan_list::const_iterator i;
    PyObject *iternext();
};


//
// LOADS
//


class LoadIterator : public PythonExtension<LoadIterator>
{
  public:
    static int initialize();

    LoadIterator(Resource* r)
      : res(r), ir(r ? r->getLoads().begin() : NULL), oper(NULL), io(NULL)
    {
      if (!r)
        throw LogicException("Creating loadplan iterator for NULL resource");
    }

    LoadIterator(Operation* o)
      : res(NULL), ir(NULL), oper(o), io(o ? o->getLoads().begin() : NULL)
    {
      if (!o)
        throw LogicException("Creating loadplan iterator for NULL operation");
    }

  private:
    Resource* res;
    Resource::loadlist::const_iterator ir;
    Operation* oper;
    Operation::loadlist::const_iterator io;
    PyObject *iternext();
};


//
// FLOW
//


class FlowIterator : public PythonExtension<FlowIterator>
{
  public:
    static int initialize();

    FlowIterator(Buffer* b)
      : buf(b), ib(b ? b->getFlows().begin() : NULL), oper(NULL), io(NULL)
    {
      if (!b)
        throw LogicException("Creating flowplan iterator for NULL buffer");
    }

    FlowIterator(Operation* o)
      : buf(NULL), ib(NULL), oper(o), io(o ? o->getFlows().begin() : NULL)
    {
      if (!o)
        throw LogicException("Creating flowplan iterator for NULL operation");
    }

  private:
    Buffer* buf;
    Buffer::flowlist::const_iterator ib;
    Operation* oper;
    Operation::flowlist::const_iterator io;
    PyObject *iternext();
};


/** @brief This Python function is used for reading XML input.
  *
  * The function takes up to three arguments:
  *   - XML data file to be processed.
  *     If this argument is omitted or None, the standard input is read.
  *   - Optional validate flag, defining whether or not the input data needs to be
  *     validated against the XML schema definition.
  *     The validation is switched ON by default.
  *     Switching it ON is recommended in situations where there is no 100% guarantee
  *     on the validity of the input data.
  *   - Optional validate_only flag, which allows us to validate the data but
  *     skip any processing.
  */
DECLARE_EXPORT PyObject* readXMLfile(PyObject*, PyObject*);


/** @brief This Python function is used for processing XML input data from a string.
  *
  * The function takes up to three arguments:
  *   - XML data string to be processed
  *   - Optional validate flag, defining whether or not the input data needs to be
  *     validated against the XML schema definition.
  *     The validation is switched ON by default.
  *     Switching it ON is recommended in situations where there is no 100% guarantee
  *     on the validity of the input data.
  *   - Optional validate_only flag, which allows us to validate the data but
  *     skip any processing.
  */
DECLARE_EXPORT PyObject* readXMLdata(PyObject *, PyObject *);


/** @brief This Python function writes the dynamic part of the plan to an text file.
  *
  * This saved information covers the buffer flowplans, operationplans,
  * resource loading, demand, problems, etc...<br>
  * The main use of this function is in the test suite: a simple text file
  * comparison allows us to identify changes quickly. The output format is
  * only to be seen in this context of testing, and is not intended to be used
  * as an official method for publishing plans to other systems.
  */
DECLARE_EXPORT PyObject* savePlan(PyObject*, PyObject*);


/** @brief This Python function prints a summary of the dynamically allocated
  * memory to the standard output. This is useful for understanding better the
  * size of your model.
  *
  * The numbers reported by this function won't match the memory size as
  * reported by the operating system, since the dynamically allocated memory
  * is only a part of the total memory used by a program.
  */
DECLARE_EXPORT PyObject* printModelSize(PyObject* self, PyObject* args);


/** @brief This python function writes the complete model to an XML-file.
  *
  * Both the static model (i.e. items, locations, buffers, resources,
  * calendars, etc...) and the dynamic data (i.e. the actual plan including
  * the operationplans, demand, problems, etc...).<br>
  * The format is such that the output file can be re-read to restore the
  * very same model.<br>
  * The function takes the following arguments:
  *   - Name of the output file
  *   - Type of output desired: STANDARD, PLAN or PLANDETAIL.
  *     The default value is STANDARD.
  */
DECLARE_EXPORT PyObject* saveXMLfile(PyObject*, PyObject*);


/** @brief This Python function erases the model or the plan from memory.
  *
  * The function allows the following modes to control what to delete:
  *  - plan:<br>
  *    Deletes the dynamic modelling constructs, such as operationplans,
  *    loadplans and flowplans only. Locked operationplans are not
  *    deleted.<br>
  *    The static model is left intact.<br>
  *    This is the default mode.
  *  - model:<br>
  *    The dynamic as well as the static objects are removed. You'll end
  *    up with a completely empty model.
  *    Due to the logic required in the object destructors this mode doesn't
  *    scale linear with the model size.
  */
DECLARE_EXPORT PyObject* eraseModel(PyObject* self, PyObject* args);


}   // End namespace

#endif
