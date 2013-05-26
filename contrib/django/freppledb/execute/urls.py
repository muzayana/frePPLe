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
    (r'^execute/scenarios/$', 'freppledb.execute.views.scenarios'),
    (r'^execute/logfrepple/$', 'freppledb.execute.views.logfile'),
    (r'^execute/log/$', freppledb.execute.views.LogReport.as_view()),
    (r'^execute/runfrepple/$', 'freppledb.execute.views.runfrepple'),
    (r'^execute/cancelfrepple/$', 'freppledb.execute.views.cancelfrepple'),
    (r'^execute/progressfrepple/$', 'freppledb.execute.views.progressfrepple'),
    (r'^execute/erase/$', 'freppledb.execute.views.erase'),
    (r'^execute/create/$', 'freppledb.execute.views.create'),
    (r'^execute/fixture/$', 'freppledb.execute.views.fixture'),
    (r'^execute/', 'freppledb.execute.views.main'),
)
