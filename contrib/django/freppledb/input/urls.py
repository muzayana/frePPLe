#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
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

urlpatterns = patterns('',

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

  # Special reports
  (r'^data/input/calendar/location/([^/]+)/$', freppledb.input.views.location_calendar),
  (r'^supplypath/([^/]+)/([^/]+)/$', freppledb.input.views.pathreport.viewupstream),
  (r'^whereused/([^/]+)/([^/]+)/$', freppledb.input.views.pathreport.viewdownstream),
  (r'^search/$', freppledb.input.views.search),  
)
