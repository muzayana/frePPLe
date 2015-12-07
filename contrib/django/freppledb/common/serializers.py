#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from rest_framework_bulk.serializers import BulkListSerializer, BulkSerializerMixin
from rest_framework_bulk import ListBulkCreateUpdateDestroyAPIView

from rest_framework.serializers import ModelSerializer
from freppledb.common.api.views import frePPleListCreateAPIView, frePPleRetrieveUpdateDestroyAPIView

import freppledb.common.models
from rest_framework import filters

class BucketSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.common.models.Bucket
      fields = ('name', 'description', 'level', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class BucketAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()
    serializer_class = BucketSerializer


    filter_fields = ('name','description','level','source')

class BucketdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()
    serializer_class = BucketSerializer

class BucketDetailSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.common.models.BucketDetail
      fields = ('bucket', 'name', 'startdate', 'enddate', 'source', 'lastmodified')

class BucketDetailAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()
    serializer_class = BucketDetailSerializer
class BucketDetaildetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()
    serializer_class = BucketDetailSerializer


class CommentSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.common.models.Comment
      fields = ('id', 'object_pk', 'comment', 'lastmodified', 'content_type','user')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class CommentAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Comment.objects.all()
    serializer_class = CommentSerializer


    filter_fields = ('id', 'object_pk', 'comment', 'lastmodified', 'content_type','user')

class CommentdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Comment.objects.all()
    serializer_class = CommentSerializer


class ParameterSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.common.models.Parameter
      fields = ('name','source', 'lastmodified', 'value', 'description')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class ParameterAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()
    serializer_class = ParameterSerializer


    filter_fields = ('name', 'source', 'lastmodified', 'value', 'description')

class ParameterdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()
    serializer_class = ParameterSerializer

