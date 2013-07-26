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
using namespace frepple;


int main (int argc, char *argv[])
{

  Date d1;
  d1.parse("2009-02-01T01:02:03", "%Y-%m-%dT%H:%M:%S");

  Date d2;
  d2.parse("2009-02-03T01:02:03", "%Y-%m-%dT%H:%M:%S");

  Date d3;
  // The date d3 is chosen such that daylight saving time
  // is in effect at that date.
  d3.parse("2009-06-01T00:00:00", "%Y-%m-%dT%H:%M:%S");

  TimePeriod t1 = 10;

  logger << "d1 \"2009-02-01T01:02:03\" => " << d1 << " " << d1.getSecondsDay()
     << " " << d1.getSecondsWeek() << " " << d1.getSecondsMonth()
     << " " << d1.getSecondsYear() << endl;
  logger << "d2 \"2009-02-03T01:02:03\" => " << d2 << " " << d2.getSecondsDay()
     << " " << d2.getSecondsWeek() << " " << d2.getSecondsMonth()
     << " " << d2.getSecondsYear() << endl;
  logger << "d3 \"2009-06-01T00:00:00\" => " << d3 << " " << d3.getSecondsDay()
     << " " << d3.getSecondsWeek() << " " << d3.getSecondsMonth()
     << " " << d3.getSecondsYear() << endl;
  logger << "t1: " << t1 << endl;

  TimePeriod t2 = d1 - d2;
  logger << "d1-d2: " << t2 << endl;

  t2 = d2 - d1;
  logger << "d2-d1: " << t2 << endl;

  d1 -= t1;
  logger << "d1-t1: " << d1 << endl;

  TimePeriod t3;
  t3.parse("P1D");
  logger << "time \"P1D\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;
  t3.parse("PT9M");
  logger << "time \"PT9M\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;
  try
  {
    t3.parse("Pwrong");
  }
  catch (const DataException& e)
  { logger << "Data exception caught: " << e.what() << endl; }
  logger << "time \"Pwrong\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;
  t3.parse("PT79M");
  logger << "time \"PT79M\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;
  t3.parse("P1W1DT1H");
  logger << "time \"P1W1DT1H\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;
  t3.parse("PT0S");
  logger << "time \"PT0S\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;
  t3.parse("-PT1M1S");
  logger << "time \"-PT1M1S\" => " << t3 << "    "
      << static_cast<long>(t3) << endl;

  logger << "infinite past: " << Date::infinitePast << endl;
  logger << "infinite future: " << Date::infiniteFuture << endl;

  return EXIT_SUCCESS;

}
