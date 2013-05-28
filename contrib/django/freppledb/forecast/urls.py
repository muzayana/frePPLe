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

import freppledb.forecast.views

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns('',
    (r'^admin/input/forecast/$', freppledb_extra.views.forecast.ForecastList.as_view()),
    (r'^forecast/([^/]+)/$',  freppledb_extra.views.forecast.OverviewReport.as_view()),
    (r'^forecastgraph/([^/]+)/$', freppledb_extra.views.forecast.GraphData),
    (r'^forecast/$', freppledb_extra.views.forecast.OverviewReport.as_view()),
    )
