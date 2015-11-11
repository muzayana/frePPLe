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
import freppledb.common.models


class BucketSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Bucket
      fields = ('name', 'description', 'level', 'source', 'lastmodified')
class BucketAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()
    serializer_class = BucketSerializer
class BucketdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Bucket.objects.all()
    serializer_class = BucketSerializer


class BucketDetailSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.BucketDetail
      fields = ('bucket', 'name', 'startdate', 'enddate', 'source', 'lastmodified')
class BucketDetailAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()
    serializer_class = BucketDetailSerializer
class BucketDetaildetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.BucketDetail.objects.all()
    serializer_class = BucketDetailSerializer


class CommentSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Comment
      fields = ('id', 'content_type', 'object_pk', 'content_object', 'comment')
class CommentAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Comment.objects.all()
    serializer_class = CommentSerializer
class CommentdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Comment.objects.all()
    serializer_class = CommentSerializer


class ParameterSerializer(serializers.ModelSerializer):
    class Meta:
      model = freppledb.common.models.Parameter
      fields = ('name', 'value', 'description', 'source', 'lastmodified')
class ParameterAPI(frePPleListCreateAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()
    serializer_class = ParameterSerializer
class ParameterdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.common.models.Parameter.objects.all()
    serializer_class = ParameterSerializer
