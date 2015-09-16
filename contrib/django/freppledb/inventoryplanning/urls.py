#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns

from freppledb.inventoryplanning.views import InventoryPlanningList, DRP, DRPitemlocation, DRPitem, DRPlocation

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  '',  # Prefix
  # Model list reports, which override standard admin screens
  (r'^data/inventoryplanning/inventoryplanning/$', InventoryPlanningList.as_view()),
  (r'^inventoryplanning/drp/$', DRP.as_view()),
  (r'^inventoryplanning/drpitemlocation/(.+)/$', DRPitemlocation.as_view()),
  (r'^inventoryplanning/drpitem/(.+)/$', DRPitem.as_view()),
  (r'^inventoryplanning/drplocation/(.+)/$', DRPlocation.as_view()),
  )