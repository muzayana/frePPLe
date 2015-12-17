#
# Copyright (C) 2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns, url

import freppledb.forecast.views
import freppledb.forecast.serializers

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  '',  # Prefix
  url(r'^data/forecast/forecastdemand/$', freppledb.forecast.views.ForecastDemandList.as_view()),
  url(r'^data/forecast/forecast/$', freppledb.forecast.views.ForecastList.as_view()),
  url(r'^forecast/demand/$', freppledb.forecast.views.OrderReport.as_view()),
  url(r'^forecast/(.+)/$', freppledb.forecast.views.OverviewReport.as_view(), name="forecast_plan"),
  url(r'^forecast/$', freppledb.forecast.views.OverviewReport.as_view()),
  url(r'^constraintforecast/(.+)/$', freppledb.forecast.views.ConstraintReport.as_view(), name="forecast_constraint"),
  url(r'^supplypath/forecast/(.+)/$', freppledb.forecast.views.UpstreamForecastPath.as_view(), name="supplypath_forecast"),

  # REST API framework
  url(r'^api/forecast/forecast/$', freppledb.forecast.serializers.ForecastAPI.as_view()),

  url(r'^api/forecast/forecast/(?P<pk>(.+))/$', freppledb.forecast.serializers.ForecastdetailAPI.as_view()),
  
  )
