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
    (r'^data/forecast/forecastdemand/$', freppledb.forecast.views.ForecastDemandList.as_view()),
    (r'^data/forecast/forecast/$', freppledb.forecast.views.ForecastList.as_view()),
    (r'^forecast/(.+)/$',  freppledb.forecast.views.OverviewReport.as_view()),
    (r'^forecast/$', freppledb.forecast.views.OverviewReport.as_view()),
    )
