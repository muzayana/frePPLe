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

import freppledb.execute.views

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns('',
    (r'^execute/$', freppledb.execute.views.TaskReport.as_view()),
    (r'^execute/logfrepple/$', freppledb.execute.views.logfile),
    (r'^execute/launch/(.+)/$', freppledb.execute.views.LaunchTask),
)
