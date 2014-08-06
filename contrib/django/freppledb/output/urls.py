#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns

import freppledb.output.views.buffer
import freppledb.output.views.demand
import freppledb.output.views.problem
import freppledb.output.views.constraint
import freppledb.output.views.resource
import freppledb.output.views.operation
import freppledb.output.views.pegging
import freppledb.output.views.kpi

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns('',
    (r'^buffer/(.+)/$', freppledb.output.views.buffer.OverviewReport.as_view()),
    (r'^buffer/$', freppledb.output.views.buffer.OverviewReport.as_view()),
    (r'^demand/(.+)/$', freppledb.output.views.demand.OverviewReport.as_view()),
    (r'^demand/$', freppledb.output.views.demand.OverviewReport.as_view()),
    (r'^resource/(.+)/$', freppledb.output.views.resource.OverviewReport.as_view()),
    (r'^resource/$', freppledb.output.views.resource.OverviewReport.as_view()),
    (r'^resourcegantt/$', freppledb.output.views.resource.GanttReport.as_view()),
    (r'^resourcegantt/(.+)/$', freppledb.output.views.resource.GanttReport.as_view()),
    (r'^operation/(.+)/$', freppledb.output.views.operation.OverviewReport.as_view()),
    (r'^operation/$',  freppledb.output.views.operation.OverviewReport.as_view()),
    (r'^demandpegging/(.+)/$', freppledb.output.views.pegging.ReportByDemand.as_view()),
    (r'^bufferpegging/$', freppledb.output.views.pegging.ReportByBuffer.as_view()),
    (r'^resourcepegging/$', freppledb.output.views.pegging.ReportByResource.as_view()),
    (r'^operationpegging/$', freppledb.output.views.pegging.ReportByOperation.as_view()),
    (r'^flowplan/(.+)/$', freppledb.output.views.buffer.DetailReport.as_view()),
    (r'^flowplan/$', freppledb.output.views.buffer.DetailReport.as_view()),
    (r'^problem/$', freppledb.output.views.problem.Report.as_view()),
    (r'^constraint/$', freppledb.output.views.constraint.BaseReport.as_view()),
    (r'^constraintdemand/(.+)/$', freppledb.output.views.constraint.ReportByDemand.as_view()),
    (r'^constraintbuffer/(.+)/$', freppledb.output.views.constraint.ReportByBuffer.as_view()),
    (r'^constraintresource/(.+)/$', freppledb.output.views.constraint.ReportByResource.as_view()),
    (r'^operationplan/(.+)/$', freppledb.output.views.operation.DetailReport.as_view()),
    (r'^operationplan/$', freppledb.output.views.operation.DetailReport.as_view()),
    (r'^loadplan/(.+)/$', freppledb.output.views.resource.DetailReport.as_view()),
    (r'^loadplan/$', freppledb.output.views.resource.DetailReport.as_view()),
    (r'^demandplan/(.+)/$', freppledb.output.views.demand.DetailReport.as_view()),
    (r'^demandplan/$', freppledb.output.views.demand.DetailReport.as_view()),
    (r'^kpi/$', freppledb.output.views.kpi.Report.as_view()),
)
