from rest_framework import serializers
from rest_framework.response import Response
from rest_framework import status, viewsets, generics, permissions, renderers
from rest_framework.decorators import api_view, detail_route

from django.views.decorators.csrf import csrf_protect, csrf_exempt

import freppledb.forecast.models

class ForecastSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.forecast.models.Forecast
      fields = ('name', 'description', 'category', 'subcategory', 'customer',
                'item', 'location', 'method', 'operation', 'priority', 'minshipment',
                'maxlateness', 'discrete', 'planned', 'out_smape', 'out_method', 'source', 'lastmodified')
class ForecastREST(generics.ListCreateAPIView):
    queryset = freppledb.forecast.models.Forecast.objects.all()#.using(request.database)
    serializer_class = ForecastSerializer
class ForecastdetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.forecast.models.Forecast.objects.all()#.using(request.database)
    serializer_class = ForecastSerializer



class ForecastDemandSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.forecast.models.ForecastDemand
      fields = ('id', 'forecast', 'startdate', 'enddate', 'quantity', 'source', 'lastmodified')
class ForecastDemandREST(generics.ListCreateAPIView):
    queryset = freppledb.forecast.models.ForecastDemand.objects.all()#.using(request.database)
    serializer_class = ForecastDemandSerializer
class ForecastDemanddetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.forecast.models.ForecastDemand.objects.all()#.using(request.database)
    serializer_class = ForecastDemandSerializer



class ForecastPlanSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.forecast.models.ForecastPlan
      fields = ('id', 'forecast', 'startdate', 'enddate', 'orderstotal', 'ordersadjustment',
                'ordersopen', 'ordersplanned', 'forecastbaseline', 'forecastadjustment',
                'forecasttotal', 'forecastconsumed', 'forecastplanned', 'orderstotalvalue',
                'ordersplannedvalue', 'forecastbaselinevalue', 'forecastadjustmentvalue',
                'ordersopenvalue', 'ordersplannedvalue', 'forecastbaselinevalue', 'forecastadjustmentvalue',
                'forecasttotalvalue', 'forecastnetvalue', 'forecastconsumedvalue', 'forecastplannedvalue')
class ForecastPlanREST(generics.ListCreateAPIView):
    queryset = freppledb.forecast.models.ForecastPlan.objects.all()#.using(request.database)
    serializer_class = ForecastPlanSerializer
class ForecastPlandetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.forecast.models.ForecastPlan.objects.all()#.using(request.database)
    serializer_class = ForecastPlanSerializer



