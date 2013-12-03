#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
'''
This test shows how we can use Python to create a frePPLe model: we can
create objects, access existing objects and change objects.

All other tests are running the Python interpreter embedded in the frePPLe
executable.
This test however runs frePPLe as Python extension module.
'''
from __future__ import print_function, unicode_literals

# Add the frePPLe directory to the Python module search path
import os
import site
if 'FREPPLE_HOME' in os.environ:
   site.addsitedir(os.environ['FREPPLE_HOME'])

import frepple
import datetime
import inspect
import types


def printModel(filename):
  '''
  A function that prints out all models to a file.
  '''

  # Open the output file
  output = open(filename,"wt")

  # Global settings
  print("Echoing global settings", file=output)
  print("Plan name:", frepple.settings.name, file=output)
  print("Plan description:", frepple.settings.description.encode('utf-8'), file=output)
  print("Plan current:", frepple.settings.current, file=output)

  # Solvers
  print("\nEchoing solvers:", file=output)
  for b in frepple.solvers():
    print("  Solver:", b.name, b.loglevel, getattr(b,'constraints',None), file=output)

  # Calendars
  print("\nEchoing calendars:", file=output)
  for b in frepple.calendars():
    print("  Calendar:", b.name, getattr(b,'default',None), file=output)
    for j in b.buckets:
      print("    Bucket:", getattr(j,'value',None), j.start, j.end, j.priority, file=output)

  # Customers
  print("\nEchoing customers:", file=output)
  for b in frepple.customers():
    print("  Customer:", b.name, b.description, b.category, b.subcategory, b.owner, file=output)

  # Locations
  print("\nEchoing locations:", file=output)
  for b in frepple.locations():
    print("  Location:", b.name, b.description, b.category, b.subcategory, b.owner, file=output)

  # Items
  print("\nEchoing items:", file=output)
  for b in frepple.items():
    print("  Item:", b.name, b.description, b.category, b.subcategory, b.owner, b.operation, file=output)

  # Resources
  print("\nEchoing resources:", file=output)
  for b in frepple.resources():
    print("  Resource:", b.name, b.description, b.category, b.subcategory, b.owner, file=output)
    for l in b.loads:
      print("    Load:", l.operation.name, l.quantity, l.effective_start, l.effective_end, file=output)
    for l in b.loadplans:
      print("    Loadplan:", l.operationplan.id, l.operationplan.operation.name, l.quantity, l.startdate, l.enddate, file=output)

  # Buffers
  print("\nEchoing buffers:", file=output)
  for b in frepple.buffers():
    print("  Buffer:", b.name, b.description, b.category, b.subcategory, b.owner, file=output)
    for l in b.flows:
      print("    Flow:", l.operation.name, l.quantity, l.effective_start, l.effective_end, file=output)
    for l in b.flowplans:
      print("    Flowplan:", l.operationplan.id, l.operationplan.operation.name, l.quantity, l.date, file=output)

  # Operations
  print("\nEchoing operations:", file=output)
  for b in frepple.operations():
    print("  Operation:", b.name, b.description, b.category, b.subcategory, file=output)
    for l in b.loads:
      print("    Load:", l.resource.name, l.quantity, l.effective_start, l.effective_end, file=output)
    for l in b.flows:
      print("    Flow:", l.buffer.name, l.quantity, l.effective_start, l.effective_end, file=output)
    if isinstance(b, frepple.operation_alternate):
      for l in b.alternates:
        print("    Alternate:", l.name, file=output)
    if isinstance(b, frepple.operation_routing):
      for l in b.steps:
        print("    Step:", l.name, file=output)

  # Demands
  print("\nEchoing demands:", file=output)
  for b in frepple.demands():
    print("  Demand:", b.name, b.due, b.item.name, b.quantity, file=output)
    for i in b.operationplans:
      print("    Operationplan:", i.id, i.operation.name, i.quantity, i.end, file=output)

  # Operationplans
  print("\nEchoing operationplans:", file=output)
  for b in frepple.operationplans():
    print("  Operationplan:", b.operation.name, b.quantity, b.start, b.end, file=output)
    for s in b.operationplans:
      print("       ", s.operation.name, s.quantity, s.start, s.end, file=output)

  # Problems
  print("\nPrinting problems", file=output)
  for i in frepple.problems():
    print("  Problem:", i.entity, i.name, i.description, i.start, i.end, i.weight, file=output)


def printExtensions():
  '''
  Echoes all entities in our extension module.
  Useful to create documentation.
  '''
  print("  Types:")
  for name, o in inspect.getmembers(frepple):
    if not inspect.isclass(o) or issubclass(o,Exception) or hasattr(o,"__iter__"): continue
    print("    %s: %s" % (o.__name__, inspect.getdoc(o)))
  print("  Methods:")
  for name, o in inspect.getmembers(frepple):
    if not inspect.isroutine(o): continue
    print("    %s: %s" % (o.__name__, inspect.getdoc(o)))
  print("  Exceptions:")
  for name, o in inspect.getmembers(frepple):
    if not inspect.isclass(o) or not issubclass(o,Exception): continue
    print("    %s" % (o.__name__))
  print("  Iterators:")
  for name, o in inspect.getmembers(frepple):
    if not inspect.isclass(o) or not hasattr(o,"__iter__"): continue
    print("    %s: %s" % (o.__name__, inspect.getdoc(o)))
  print("  Other:")
  for name, o in inspect.getmembers(frepple):
    # Negating the exact same filters as in the previous blocks
    if not(not inspect.isclass(o) or issubclass(o,Exception) or hasattr(o,"__iter__")): continue
    if inspect.isroutine(o): continue
    if not(not inspect.isclass(o) or not issubclass(o,Exception)): continue
    if not(not inspect.isclass(o) or not hasattr(o,"__iter__")): continue
    print("    %s: %s" % (name, o))


###
print("\nUpdating global settings")
frepple.settings.name = "demo model"
frepple.settings.description = "unicode А Б В Г Д Е Ё Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь"
frepple.settings.current = datetime.datetime(2009,1,1)

###
print("\nCreating operations")
shipoper = frepple.operation_fixed_time(name="delivery end item", duration=86400)
choice = frepple.operation_alternate(name="make or buy item")
makeoper = frepple.operation_routing(name="make item")
makeoper.addStep(frepple.operation_fixed_time(name="make item - step 1", duration=4*86400))
makeoper.addStep(frepple.operation_fixed_time(name="make item - step 2", duration=3*86400))
buyoper = frepple.operation_fixed_time(name="buy item", duration=86400)
choice.addAlternate(operation=makeoper, priority=1)
choice.addAlternate(operation=buyoper, priority=2)

###
print("\nCreating calendars")
c = frepple.calendar(name="Cal1", default=4.56)
c.setValue(datetime.datetime(2009,1,1), datetime.datetime(2009,3,1), 1)
c.setValue(datetime.datetime(2009,2,1), datetime.datetime(2009,5,1), 2)
c.setValue(datetime.datetime(2009,2,1), datetime.datetime(2009,3,1), 3)
frepple.calendar(name="Cal2", default=1.23)
frepple.calendar(name="Cal3", default=1.23)

###
print("\nTesting the calendar iterator")
print("calendar events:")
for date, value in c.events():
  print("  ", date, value)

###
print("\nDeleting a calendar")
frepple.calendar(name="Cal3", action="R")

# Load some data - These things can't be done yet from Python
frepple.readXMLdata('''<?xml version="1.0" encoding="UTF-8" ?>
<plan xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <resources>
    <resource name="Resource">
      <maximum_calendar name="Capacity">
        <buckets>
          <bucket start="2009-01-01T00:00:00">
            <value>1</value>
          </bucket>
        </buckets>
      </maximum_calendar>
      <loads>
        <load>
          <operation name="make item - step 1" />
        </load>
        <load>
          <operation name="make item - step 2" />
        </load>
      </loads>
    </resource>
  </resources>
  <flows>
    <flow xsi:type="flow_start">
      <operation name="delivery end item" />
      <buffer name="end item" />
      <quantity>-1</quantity>
    </flow>
    <flow xsi:type="flow_end">
      <operation name="make or buy item" />
      <buffer name="end item" />
      <quantity>1</quantity>
    </flow>
  </flows>
</plan>
''')

###
print("\nCreating operationplans")
opplan = frepple.operationplan(operation="make item", quantity=9, end=datetime.datetime(2011,1,1))
opplan.locked = True

###
print("\nCreating items")
item = frepple.item(name="end item", operation=shipoper)
itemlist = [ frepple.item(name="item %d" % i) for i in range(10) ]

###
print("\nTesting the comparison operator")
print("makeoper < shipoper", makeoper < shipoper)
print("shipoper < makeoper", shipoper < makeoper)
print("shipoper != makeoper", shipoper != makeoper)
print("shipoper == makeoper", shipoper == makeoper)
print("shipoper == shipoper", shipoper == shipoper)
try:
  print("makeoper == item", makeoper == item)
except Exception as e:
  print("Catching exception %s: %s" % (e.__class__.__name__, e))

###
print("\nCreating a resource")
frepple.resource(name="machine", maximum_calendar=frepple.calendar(name="Cal2"))

###
print("\nCreating customers")
mycustomer = frepple.customer(name="client")

###
print("\nCreating locations")
locA = frepple.location(name="locA")
locB = frepple.location(name="locB")

###
print("\nCreating some buffers")

buf = frepple.buffer(name="end item", producing=choice, item=item)

buf1 = frepple.buffer_procure(name="buffer1",
  description="My description",
  category="My category",
  location=locA,
  item=itemlist[1])
print(buf1, buf1.__class__, buf1.location, isinstance(buf1, frepple.buffer), \
  isinstance(buf1, frepple.buffer_default), \
  isinstance(buf1, frepple.buffer_procure), \
  isinstance(buf1, frepple.buffer_infinite))

buf2 = frepple.buffer(name="buffer2", owner=buf1)
print(buf2, buf2.__class__, buf2.location, isinstance(buf2, frepple.buffer), \
  isinstance(buf2, frepple.buffer_default), \
  isinstance(buf2, frepple.buffer_procure), \
  isinstance(buf2, frepple.buffer_infinite))

###
print("\nCatching some exceptions")
try:
  print(buf1.crazyfield)
except Exception as e:
  print("Catching exception %s: %s" % (e.__class__.__name__, e))

try:
  buf1.crazyfield = "doesn't exist"
except Exception as e:
  print("Catching exception %s: %s" % (e.__class__.__name__, e))

try:
  buf1.owner = buf2
except Exception as e:
  print("Catching exception %s: %s" % (e.__class__.__name__, e))

###
print("\nCreating demands")
order1 = frepple.demand(name="order 1", item=item, quantity=10, priority=1, \
  due=datetime.datetime(2009,3,2,9), customer=mycustomer, maxlateness=0)
order2 = frepple.demand(name="order 2", item=item, quantity=10, priority=2, \
  due=datetime.datetime(2009,3,2,8,30,0), customer=mycustomer, maxlateness=0)
order3 = frepple.demand(name="order 3", item=item, quantity=10, priority=3, \
  due=datetime.datetime(2009,3,2,20,0,0), customer=mycustomer, maxlateness=0)

###
print("\nCreating a solver and running it")
frepple.solver_mrp(name="MRP", constraints=7, loglevel=0).solve()

###
print("\nEchoing the model to a file")
printModel("output.1.xml")

###
print("\nSaving the model to an XML-file")
frepple.saveXMLfile("output.2.xml")

###
print("\nPrinting some models in XML format")
print(mycustomer.toXML())
print(locA.toXML())
print(opplan.toXML())
print(item.toXML())
print(order1.toXML())
print(buf1.toXML())
print(makeoper.toXML())
for i in frepple.problems():
  print(i.toXML())

###
print("\nPrinting some models in XML format to a file")
with open("output.3.xml","wt") as output:
  mycustomer.toXML('P',output)
  locA.toXML('P',output)
  opplan.toXML('P',output)
  item.toXML('P',output)
  order1.toXML('P',output)
  buf1.toXML('P',output)
  makeoper.toXML('P',output)
  for i in frepple.problems():
    i.toXML('P',output)

###
print("\nDocumenting all available Python entities defined by frePPLe:")
printExtensions()

###
print("\nPrinting memory consumption estimate:")
frepple.printsize()
