#!/usr/bin/python3
#
# Copyright (C) 2007 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
import os, sys, random

runtimes = {}

for counter in [500,1000,1500,2000]:
  print("\ncounter", counter)
  out = open("input.xml","wt")

  # Print a header
  print('<plan xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">\n' +
    '<description>Single buffer plan with $counter demands</description>\n' +
    '<current>2009-01-01T00:00:00</current>\n' +
    '<items>\n' +
      '\t<item name="ITEM">' +
      '<operation name="Delivery ITEM" xsi:type="operation_fixed_time"/>' +
      '</item>\n' +
    '</items>\n' +
    '<operations>\n' +
      '\t<operation name="Make ITEM" xsi:type="operation_fixed_time"/>\n' +
    '</operations>\n' +
    '<buffers>\n' +
      '\t<buffer name="BUFFER"><onhand>10</onhand>' +
      '<producing name="Make ITEM"/>' +
      '</buffer>\n' +
    '</buffers>\n' +
    '<flows>\n' +
      '\t<flow xsi:type="flow_start"><operation name="Delivery ITEM"/>' +
      '<buffer name="BUFFER"/>' +
      '<quantity>-1</quantity></flow>\n' +
      '\t<flow xsi:type="flow_end"><operation name="Make ITEM"/>' +
      '<buffer name="BUFFER"/>' +
      '<quantity>1</quantity></flow>\n' +
    '</flows>\n' +
    '<demands>', file=out)

  # A loop to print all demand
  for cnt in range(counter):
    month = "%02d" % (int(random.uniform(0,12))+1)
    day = "%02d" % (int(random.uniform(0,28))+1)
    print(("<demand name=\"DEMAND %d\" quantity=\"10\" " +
      "due=\"2009-%s-%sT00:00:00\" " +
      "priority=\"1\">" +
      "<item name=\"ITEM\"/>" +
      "</demand>") % (cnt,month,day), file=out)

  # Finalize the input
  print('</demands>\n' +
    '<?python\n' +
    'import frepple\n' +
    'frepple.solver_mrp(name="MRP",constraints=0).solve()\n' +
    'frepple.saveXMLfile("output.xml")\n' +
    '?>\n' +
    '</plan>', file=out)
  out.close()

  # Run the executable
  starttime = os.times()
  out = os.popen(os.environ['EXECUTABLE'] + "  ./input.xml")
  while True:
    i = out.readline()
    if not i: break
    print(i.strip())
  if out.close() != None:
    print("Planner exited abnormally")
    sys.exit(1)

  # Measure the time
  endtime = os.times()
  runtimes[counter] = endtime[4]-starttime[4]
  print("time: %.3f" % runtimes[counter])

# Define failure criterium
if runtimes[2000] > runtimes[500]*4*1.2:
  print("\nTest failed. Run time is not linear with model size.")
  sys.exit(1)

print("\nTest passed")
