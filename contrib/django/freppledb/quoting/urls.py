#
# Copyright (C) 2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns

import freppledb.quoting.views

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns('',
    (r'^quote/$', freppledb.quoting.views.QuoteReport.as_view()),
    (r'^quote/info/$', freppledb.quoting.views.InfoView, {'action': 'info'}),
    (r'^quote/check/$', freppledb.quoting.views.InfoView, {'action': 'check'}),
    (r'^quote/cancel/$', freppledb.quoting.views.InfoView, {'action': 'cancel'}),
    (r'^quote/save/$', freppledb.quoting.views.InfoView, {'action': 'save'}),
    )
