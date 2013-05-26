#
# Copyright (C) 2007 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

r'''
Exports frePPLe information to a flat file usable as a data file for
a LP formulation.

This file is only an example. A different LP formulation will obviously
require a different data file. Given the easy structure of this
Python code this will remain a clean and simple piece of code.
'''

import frepple
import csv


def exportData(filename):

  output = open(filename,"wt")

  print >>output, "param timerate := 0.97;\n"

  print >>output, "param : demands : reqqty  prio  due :="
  for b in frepple.demands():
    if b.quantity > 0:
      # @todo Export of due date works for monthly buckets and a maximum horizon of 1 year only
      print >>output, b.name.replace(' ','').replace(':',''), b.quantity, b.priority, b.due.month
  print >>output, ";\n"

  print >>output, "param numbuckets := 12;"
  print >>output, "set buckets := 1 2 3 4 5 6 7 8 9 10 11 12;"

  print >>output, "set resources := "
  for b in frepple.resources():
    print >>output, b.name.replace(' ','').replace(':','')
  print >>output, ";\n"

  print >>output, "param availablecapacity"
  print >>output, ": 1 2 3 4 5 6 7 8 9 10 11 12 := "
  res = []
  for b in frepple.resources():
    # @todo need a more correct way to extract the capacity per bucket.
    res.append(b.name.replace(' ','').replace(':',''))
    print >>output, b.name.replace(' ','').replace(':',''), "120 120 120 120 120 120 120 120 120 120 120 120"
  print >>output, ";\n"

  print >>output, "param : loads : loadfactor :="
  for b in frepple.demands():
    if b.quantity > 0:
      oper = b.operation or b.item.operation
      if oper:
        for fl in oper.flows:
          if fl.quantity < 0: findResources(output, b, fl)
  print >>output, ";\n"
  print >>output, "end;\n"


def findResources(output, dem, flow):
  try:
    for load in flow.buffer.producing.loads:
      # @todo The load factor doesn't accumulate correctly across different steps: quantity_per of the flows...
      print >>output, dem.name.replace(' ','').replace(':',''), load.resource.name.replace(' ','').replace(':',''), load.quantity
    for newflow in flow.buffer.producing.flows:
      if newflow.quantity < 0:
        findResources(output, dem, newflow)
  except: pass


def importSolution(filename):

  # Open the solution file
  reader = csv.reader(
    open(filename, "rt"),
    delimiter=' ',
    skipinitialspace=True,
    quoting=csv.QUOTE_NONE)

  # Scan the solution file for lines like this:
  #    45 plannedqty[order1]
  #         NU            5             0            10         < eps
  # This indicates that "order1" has 5 units planned
  #    @todo need to also recognize lated demand
  print "Planned quantity per demand:"
  demandname = None
  for line in reader:
    if demandname:
      print "    ", demandname, line[1]
      demandname = None
    elif len(line)==2 and line[1].startswith("plannedqty["):
      demandname = line[1][11:-1]
