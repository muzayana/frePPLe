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
from rest_framework.response import Response
from rest_framework import status, viewsets, generics, permissions, renderers
from rest_framework.decorators import api_view, detail_route

from django.views.decorators.csrf import csrf_protect, csrf_exempt

import freppledb.common.models


class frePPleListCreateAPIView(generics.ListCreateAPIView):
    def get_queryset(self):
      return super(frePPleListCreateAPIView, self).get_queryset().using(self.request.database)

class frePPleRetrieveUpdateDestroyAPIView(generics.RetrieveUpdateDestroyAPIView):
    def get_queryset(self):
      return super(frePPleRetrieveUpdateDestroyAPIView, self).get_queryset().using(self.request.database)


class BucketSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Bucket
      fields = ('name', 'description', 'level', 'source', 'lastmodified')
class BucketREST(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()#.using(request.database)
    serializer_class = BucketSerializer
class BucketdetailREST(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()#.using(request.database)
    serializer_class = BucketSerializer



class BucketDetailSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.BucketDetail
      fields = ('bucket', 'name', 'startdate', 'enddate', 'source', 'lastmodified')
class BucketDetailREST(frePPleListCreateAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()#.using(request.database)
    serializer_class = BucketDetailSerializer
class BucketDetaildetailREST(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()#.using(request.database)
    serializer_class = BucketDetailSerializer



class CommentSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Comment
      fields = ('id', 'content_type', 'object_pk', 'content_object', 'comment')
class CommentREST(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Comment.objects.all()#.using(request.database)
    serializer_class = CommentSerializer
class CommentdetailREST(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Comment.objects.all()#.using(request.database)
    serializer_class = CommentSerializer


class ParameterSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Parameter
      fields = ('name', 'value', 'description', 'source', 'lastmodified')
class ParameterREST(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()#.using(request.database)
    serializer_class = ParameterSerializer
class ParameterdetailREST(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()#.using(request.database)
    serializer_class = ParameterSerializer


class ScenarioSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Scenario
      fields = ('name', 'description', 'status', 'lastrefresh')
class ScenarioREST(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Scenario.objects.all()#.using(request.database)
    serializer_class = ScenarioSerializer
class ScenariodetailREST(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Scenario.objects.all()#.using(request.database)
    serializer_class = ScenarioSerializer

