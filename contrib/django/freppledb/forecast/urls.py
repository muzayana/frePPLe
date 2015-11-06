#
# Copyright (C) 2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns

import freppledb.forecast.views
import freppledb.forecast.serializers

# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  '',  # Prefix
  (r'^data/forecast/forecastdemand/$', freppledb.forecast.views.ForecastDemandList.as_view()),
  (r'^data/forecast/forecast/$', freppledb.forecast.views.ForecastList.as_view()),
  (r'^forecast/demand/$', freppledb.forecast.views.OrderReport.as_view()),
  (r'^forecast/(.+)/$', freppledb.forecast.views.OverviewReport.as_view()),
  (r'^forecast/$', freppledb.forecast.views.OverviewReport.as_view()),
  (r'^constraintforecast/(.+)/$', freppledb.forecast.views.ConstraintReport.as_view()),
  (r'^supplypath/forecast/(.+)/$', freppledb.forecast.views.UpstreamForecastPath.as_view()),

  #REST framework
  (r'^api/forecast/forecastdemand/$', freppledb.forecast.serializers.ForecastDemandREST.as_view()),
  (r'^api/forecast/forecast/$', freppledb.forecast.serializers.ForecastREST.as_view()),
  (r'^api/forecast/forecastplan/$', freppledb.forecast.serializers.ForecastPlanREST.as_view()),

  (r'^api/forecast/forecastdemand/(?P<pk>(.+))/$', freppledb.forecast.serializers.ForecastDemanddetailREST.as_view()),
  (r'^api/forecast/forecast/(?P<pk>(.+))/$', freppledb.forecast.serializers.ForecastdetailREST.as_view()),
  (r'^api/forecast/forecastplan/(?P<pk>(.+))/$', freppledb.forecast.serializers.ForecastPlandetailREST.as_view()),

  )
