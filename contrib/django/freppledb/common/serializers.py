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


#===============================================================================
# class AuditModelSerializer(serializers.ModelSerializer):
#     class Meta:
#       model = freppledb.common.models.AuditModel
#       fields = ('source', 'lastmodified', 'objects')
#
# class AuditModelREST(viewsets.ReadOnlyModelViewSet):
#     queryset = freppledb.common.models.AuditModel.objects.all()#.using(request.database)
#     serializer_class = AuditModelSerializer
#===============================================================================


class BucketSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Bucket
      fields = ('name', 'description', 'level', 'source', 'lastmodified')
class BucketREST(generics.ListCreateAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()#.using(request.database)
    serializer_class = BucketSerializer
class BucketdetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()#.using(request.database)
    serializer_class = BucketSerializer



class BucketDetailSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.BucketDetail
      fields = ('bucket', 'name', 'startdate', 'enddate', 'source', 'lastmodified')
class BucketDetailREST(generics.ListCreateAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()#.using(request.database)
    serializer_class = BucketDetailSerializer
class BucketDetaildetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()#.using(request.database)
    serializer_class = BucketDetailSerializer



class CommentSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Comment
      fields = ('id', 'content_type', 'object_pk', 'content_object', 'comment')
class CommentREST(generics.ListCreateAPIView):
    queryset = freppledb.common.models.Comment.objects.all()#.using(request.database)
    serializer_class = CommentSerializer
class CommentdetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Comment.objects.all()#.using(request.database)
    serializer_class = CommentSerializer


class ParameterSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Parameter
      fields = ('name', 'value', 'description', 'source', 'lastmodified')
class ParameterREST(generics.ListCreateAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()#.using(request.database)
    serializer_class = ParameterSerializer
class ParameterdetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()#.using(request.database)
    serializer_class = ParameterSerializer


class ScenarioSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Scenario
      fields = ('name', 'description', 'status', 'lastrefresh')
class ScenarioREST(generics.ListCreateAPIView):
    queryset = freppledb.common.models.Scenario.objects.all()#.using(request.database)
    serializer_class = ScenarioSerializer
class ScenariodetailREST(generics.RetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Scenario.objects.all()#.using(request.database)
    serializer_class = ScenarioSerializer



# Create your views here.
#===============================================================================
#
# #@detail_route(renderer_classes=(renderers.StaticHTMLRenderer))
# class ParameterREST(viewsets.ReadOnlyModelViewSet):
#     queryset = freppledb.common.models.Parameter.objects.all()#.using(request.database)
#     serializer_class = freppledb.api.serializers.ParameterSerializer
#
#
#
# class LocationREST(generics.ListCreateAPIView):
#     queryset = freppledb.input.models.Location.objects.all()#.using(request.database)
#     serializer = freppledb.api.serializers.LocationSerializer
#
#
# class api:
#   model = None
#   serializer = None
#
#   @classmethod
#   @csrf_exempt
#   def rest_api(cls, request):
#     '''
#     All configurable parameters serialized.
#     '''
#     if request.method == 'GET':
#       basequeryset = cls.model.objects.all().using(request.database)
#       serializer = cls.serializer(basequeryset, many=True)
#       print(serializer, request.database)
#       return Response(serializer.data)
#
#     elif request.method == 'POST':
#       return Response(serializer.errors)
#===============================================================================

#===============================================================================
#
# @api_view(['GET', 'POST'])
# class ParameterList_REST(viewsets.ModelViewSet):
#   '''
#   All configurable parameters serialized.
#   '''
#   basequeryset = Parameter.objects.all().using(request.database)
#   serializer_class = serializers.frepple_serializer
#   permission_classes = (permissions.IsAuthenticatedOrReadOnly, IsOwnerOrReadOnly,)
#
#   @detail_route(renderer_classes=[renderers.StaticHTMLRenderer])
#   def highlight(self, request, *args, **kwargs):
#       snippet = self.get_object()
#       return Response(snippet.highlighted)
#
#   def perform_create(self, serializer):
#       serializer.save(owner=self.request.user)
#===============================================================================



#===============================================================================
# from rest_framework import serializers
# import freppledb.common.models
#
#
# class ParameterSerializer(serializers.ModelSerializer): # serializers.ModelSerializer):
#   class Meta:
#       model = freppledb.common.models.Parameter
#       fields = ('name', 'value', 'description')
#
#
#===============================================================================
#===============================================================================
# class frepple_serializer(serializers.ModelSerializer):
#   @classmethod
#   @csrf_exempt
#   def rest(cls, request, args=None):
#     '''
#     All configurable parameters serialized.
#     '''
#     if request.method == 'GET':
#       if args:
#         try:
#           obj = cls.Meta.model.objects.all().using(request.database).get(pk=args)
#         except cls.Meta.model.DoesNotExist:
#           return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)
#         serializer = cls(obj)
#       else:
#         basequeryset = cls.Meta.model.objects.all().using(request.database)
#         serializer = cls(basequeryset, many=True)
#         print(serializer, request.database)
#       return Response(serializer.data)
#
#     elif request.method == 'POST':
#       return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)
#===============================================================================
