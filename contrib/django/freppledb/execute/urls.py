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

import freppledb.execute.views

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  '',   # Prefix
  url(r'^execute/$', freppledb.execute.views.TaskReport.as_view(), name="execute"),
  url(r'^execute/logfrepple/$', freppledb.execute.views.logfile, name="execute_log"),
  url(r'^execute/launch/(.+)/$', freppledb.execute.views.LaunchTask, name="execute_launch"),
  url(r'^execute/cancel/(.+)/$', freppledb.execute.views.CancelTask, name="execute_cancel"),
)
