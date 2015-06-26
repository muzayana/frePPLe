#
# Copyright (C) 2007-2012 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from freppledb.menu import menu
import freppledb.output.views.buffer
import freppledb.output.views.demand
import freppledb.output.views.problem
import freppledb.output.views.constraint
import freppledb.output.views.resource
import freppledb.output.views.operation
import freppledb.output.views.kpi

# Adding reports. We use an index value to keep the same order of the entries in all languages.
menu.addItem("reports", "inventory report", url="/buffer/", report=freppledb.output.views.buffer.OverviewReport, index=100)
menu.addItem("reports", "resource report", url="/resource/", report=freppledb.output.views.resource.OverviewReport, index=200)
menu.addItem("reports", "resource gantt report", url="/resourcegantt/", report=freppledb.output.views.resource.GanttReport, index=200)
menu.addItem("reports", "demand report", url="/demand/", report=freppledb.output.views.demand.OverviewReport, index=300)
menu.addItem("reports", "operation report", url="/operation/", report=freppledb.output.views.operation.OverviewReport, index=400)
menu.addItem("reports", "operation detail report", url="/operationplan/", report=freppledb.output.views.operation.DetailReport, index=500)
menu.addItem("reports", "resource detail report", url="/loadplan/", report=freppledb.output.views.resource.DetailReport, index=600)
menu.addItem("reports", "inventory detail report", url="/flowplan/", report=freppledb.output.views.buffer.DetailReport, index=700)
menu.addItem("reports", "demand detail report", url="/demandplan/", report=freppledb.output.views.demand.DetailReport, index=800)
menu.addItem("reports", "problem report", url="/problem/", report=freppledb.output.views.problem.Report, index=900)
menu.addItem("reports", "constraint report", url="/constraint/", report=freppledb.output.views.constraint.BaseReport, index=1000)
menu.addItem("reports", "kpi report", url="/kpi/", report=freppledb.output.views.kpi.Report, index=1100)
