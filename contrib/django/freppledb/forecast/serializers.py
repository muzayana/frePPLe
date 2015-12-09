#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from rest_framework import serializers

from freppledb.common.api.views import frePPleListCreateAPIView, frePPleRetrieveUpdateDestroyAPIView
import freppledb.forecast.models


class ForecastSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.forecast.models.Forecast
      fields = ('name', 'description', 'category', 'subcategory', 'customer',
                'item', 'location', 'method', 'operation', 'priority', 'minshipment',
                'maxlateness', 'discrete', 'planned', 'out_smape', 'out_method', 'source', 'lastmodified')
class ForecastAPI(frePPleListCreateAPIView):
    queryset = freppledb.forecast.models.Forecast.objects.all()#.using(request.database)
    serializer_class = ForecastSerializer
class ForecastdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.forecast.models.Forecast.objects.all()#.using(request.database)
    serializer_class = ForecastSerializer
