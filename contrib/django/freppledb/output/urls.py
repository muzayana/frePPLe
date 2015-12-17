#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns, url

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

urlpatterns = patterns(
  '',  # Prefix
  url(r'^buffer/(.+)/$', freppledb.output.views.buffer.OverviewReport.as_view(), name="output_buffer_plandetail"),
  url(r'^buffer/$', freppledb.output.views.buffer.OverviewReport.as_view(), name="output_buffer_plan"),
  url(r'^demand/(.+)/$', freppledb.output.views.demand.OverviewReport.as_view(), name="output_demand_plandetail"),
  url(r'^demand/$', freppledb.output.views.demand.OverviewReport.as_view(), name="output_demand_plan"),
  url(r'^resource/(.+)/$', freppledb.output.views.resource.OverviewReport.as_view(), name="output_resource_plandetail"),
  url(r'^resource/$', freppledb.output.views.resource.OverviewReport.as_view(), name="output_resource_plan"),
  url(r'^operation/(.+)/$', freppledb.output.views.operation.OverviewReport.as_view(), name="output_operation_plandetail"),
  url(r'^operation/$', freppledb.output.views.operation.OverviewReport.as_view(), name="output_operation_plan"),
  url(r'^resourcegantt/$', freppledb.output.views.resource.GanttReport.as_view(), name="output_resource_gantt"),
  url(r'^resourcegantt/(.+)/$', freppledb.output.views.resource.GanttReport.as_view(), name="output_resource_ganttdetail"),
  url(r'^demandpegging/(.+)/$', freppledb.output.views.pegging.ReportByDemand.as_view(), name="output_demand_pegging"),
  url(r'^flowplan/(.+)/$', freppledb.output.views.buffer.DetailReport.as_view(), name="output_flowplan_plandetail"),
  url(r'^flowplan/$', freppledb.output.views.buffer.DetailReport.as_view(), name="output_flowplan_plan"),
  url(r'^problem/$', freppledb.output.views.problem.Report.as_view(), name="output_problem"),
  url(r'^constraint/$', freppledb.output.views.constraint.BaseReport.as_view(), name="output_constraint"),
  url(r'^constraintoperation/(.+)/$', freppledb.output.views.constraint.ReportByOperation.as_view(), name="output_constraint_operation"),
  url(r'^constraintdemand/(.+)/$', freppledb.output.views.constraint.ReportByDemand.as_view(), name="output_constraint_demand"),
  url(r'^constraintbuffer/(.+)/$', freppledb.output.views.constraint.ReportByBuffer.as_view(), name="output_constraint_buffer"),
  url(r'^constraintresource/(.+)/$', freppledb.output.views.constraint.ReportByResource.as_view(), name="output_constraint_resource"),
  url(r'^operationplan/(.+)/$', freppledb.output.views.operation.DetailReport.as_view(), name="output_operationplan_plandetail"),
  url(r'^operationplan/$', freppledb.output.views.operation.DetailReport.as_view(), name="output_operationplan_plan"),
  url(r'^loadplan/(.+)/$', freppledb.output.views.resource.DetailReport.as_view(), name="output_loadplan_plandetail"),
  url(r'^loadplan/$', freppledb.output.views.resource.DetailReport.as_view(), name="output_loadplan_plan"),
  url(r'^demandplan/(.+)/$', freppledb.output.views.demand.DetailReport.as_view(), name="output_demandplan_plandetail"),
  url(r'^demandplan/$', freppledb.output.views.demand.DetailReport.as_view(), name="output_buffer_plan"),
  url(r'^kpi/$', freppledb.output.views.kpi.Report.as_view(), name="output_kpi"),
  )
