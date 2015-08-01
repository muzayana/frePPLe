#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns

import freppledb.input.views

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  '',  # Prefix

  # Model list reports, which override standard admin screens
  (r'^data/input/buffer/$', freppledb.input.views.BufferList.as_view()),
  (r'^data/input/resource/$', freppledb.input.views.ResourceList.as_view()),
  (r'^data/input/location/$', freppledb.input.views.LocationList.as_view()),
  (r'^data/input/customer/$', freppledb.input.views.CustomerList.as_view()),
  (r'^data/input/demand/$', freppledb.input.views.DemandList.as_view()),
  (r'^data/input/item/$', freppledb.input.views.ItemList.as_view()),
  (r'^data/input/load/$', freppledb.input.views.LoadList.as_view()),
  (r'^data/input/flow/$', freppledb.input.views.FlowList.as_view()),
  (r'^data/input/calendar/$', freppledb.input.views.CalendarList.as_view()),
  (r'^data/input/calendarbucket/$', freppledb.input.views.CalendarBucketList.as_view()),
  (r'^data/input/operation/$', freppledb.input.views.OperationList.as_view()),
  (r'^data/input/setupmatrix/$', freppledb.input.views.SetupMatrixList.as_view()),
  (r'^data/input/suboperation/$', freppledb.input.views.SubOperationList.as_view()),
  (r'^data/input/operationplan/$', freppledb.input.views.OperationPlanList.as_view()),
  (r'^data/input/skill/$', freppledb.input.views.SkillList.as_view()),
  (r'^data/input/resourceskill/$', freppledb.input.views.ResourceSkillList.as_view()),
  (r'^data/input/supplier/$', freppledb.input.views.SupplierList.as_view()),
  (r'^data/input/supplieritem/$', freppledb.input.views.SupplierItemList.as_view()),

  # Special reports
  (r'^data/input/calendar/location/(.+)/$', freppledb.input.views.location_calendar),
  (r'^supplypath/item/(.+)/$', freppledb.input.views.UpstreamItemPath.as_view()),
  (r'^whereused/item/(.+)/$', freppledb.input.views.DownstreamItemPath.as_view()),
  (r'^supplypath/buffer/(.+)/$', freppledb.input.views.UpstreamBufferPath.as_view()),
  (r'^whereused/buffer/(.+)/$', freppledb.input.views.DownstreamBufferPath.as_view()),
  (r'^supplypath/resource/(.+)/$', freppledb.input.views.UpstreamResourcePath.as_view()),
  (r'^supplypath/demand/(.+)/$', freppledb.input.views.UpstreamDemandPath.as_view()),
  (r'^whereused/resource/(.+)/$', freppledb.input.views.DownstreamResourcePath.as_view()),
  (r'^supplypath/operation/(.+)/$', freppledb.input.views.UpstreamOperationPath.as_view()),
  (r'^whereused/operation/(.+)/$', freppledb.input.views.DownstreamOperationPath.as_view()),
  (r'^search/$', freppledb.input.views.search),
  )
