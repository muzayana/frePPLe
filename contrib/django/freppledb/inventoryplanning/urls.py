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

import freppledb.inventoryplanning.views

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  '',  # Prefix
  # Model list reports, which override standard admin screens
  (r'^data/inventoryplanning/inventoryplanning/$', freppledb.inventoryplanning.views.InventoryPlanningList.as_view()),
  )
