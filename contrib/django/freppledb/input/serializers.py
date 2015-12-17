#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from freppledb.common.api.views import frePPleListCreateAPIView, frePPleRetrieveUpdateDestroyAPIView
import freppledb.input.models

from rest_framework_bulk.drf3.serializers import BulkListSerializer, BulkSerializerMixin
from rest_framework_bulk import ListBulkCreateUpdateDestroyAPIView

from rest_framework.serializers import ModelSerializer
from freppledb.common.api.views import frePPleListCreateAPIView, frePPleRetrieveUpdateDestroyAPIView

from rest_framework import filters

class CalendarSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Calendar
      fields = ('name', 'description', 'category', 'subcategory', 'defaultvalue', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class CalendarAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Calendar.objects.all()
    serializer_class = CalendarSerializer


    filter_fields = ('name', 'description', 'category', 'subcategory', 'defaultvalue', 'source', 'lastmodified')

class CalendardetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Calendar.objects.all()
    serializer_class = CalendarSerializer


class CalendarBucketSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.CalendarBucket
      fields = ('id', 'calendar', 'startdate', 'enddate', 'value', 'priority', 'monday', 'tuesday', 'wednesday',
                'thursday', 'friday', 'saturday', 'sunday', 'starttime', 'endtime', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class CalendarBucketAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.CalendarBucket.objects.all()
    serializer_class = CalendarBucketSerializer
class CalendarBucketdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.CalendarBucket.objects.all()
    serializer_class = CalendarBucketSerializer


class LocationSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Location
      fields = ('name', 'description', 'category', 'subcategory', 'available', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class LocationAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Location.objects.all()
    serializer_class = LocationSerializer


    filter_fields = ('name', 'description', 'category', 'subcategory', 'available', 'source', 'lastmodified')

class LocationdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Location.objects.all()
    serializer_class = LocationSerializer


class CustomerSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Customer
      fields = ( 'name', 'description', 'category', 'subcategory', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class CustomerAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Customer.objects.all()
    serializer_class = CustomerSerializer


    filter_fields = ( 'name', 'description', 'category', 'subcategory', 'source', 'lastmodified')

class CustomerdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Customer.objects.all()
    serializer_class = CustomerSerializer


class ItemSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Item
      fields = ('name', 'owner', 'description', 'category', 'subcategory', 'operation', 'price', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class ItemAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Item.objects.all()


    filter_fields = ('name', 'owner', 'description', 'category', 'subcategory', 'operation', 'price', 'source', 'lastmodified')

    serializer_class = ItemSerializer
class ItemdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Item.objects.all()
    serializer_class = ItemSerializer


class SupplierSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Supplier
      fields = ('name', 'owner', 'description', 'category', 'subcategory', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class SupplierAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Supplier.objects.all()
    serializer_class = SupplierSerializer


    filter_fields = ('name', 'owner', 'description', 'category', 'subcategory', 'source', 'lastmodified')

class SupplierdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Supplier.objects.all()
    serializer_class = SupplierSerializer


class ItemSupplierSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.ItemSupplier
      fields = ('id', 'item', 'location', 'supplier', 'leadtime', 'sizeminimum', 'sizemultiple',
                 'cost', 'priority', 'effective_start', 'effective_end', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class ItemSupplierAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.ItemSupplier.objects.all()
    serializer_class = ItemSupplierSerializer


    filter_fields = ('id', 'item', 'location', 'supplier', 'leadtime', 'sizeminimum', 'sizemultiple',
                 'cost', 'priority', 'effective_start', 'effective_end', 'source', 'lastmodified')

class ItemSupplierdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.ItemSupplier.objects.all()
    serializer_class = ItemSupplierSerializer


class ItemDistributionSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.ItemDistribution
      fields = ('id', 'item', 'location', 'origin', 'leadtime', 'sizeminimum', 'sizemultiple',
                 'cost', 'priority', 'effective_start', 'effective_end', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class ItemDistributionAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.ItemDistribution.objects.all()
    serializer_class = ItemDistributionSerializer


    filter_fields = ('id', 'item', 'location', 'origin', 'leadtime', 'sizeminimum', 'sizemultiple',
                 'cost', 'priority', 'effective_start', 'effective_end', 'source', 'lastmodified')

class ItemDistributiondetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.ItemDistribution.objects.all()
    serializer_class = ItemDistributionSerializer


class OperationSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Operation
      fields = ('name', 'type', 'description', 'category', 'subcategory', 'location', 'fence',
                'posttime', 'sizeminimum', 'sizemultiple', 'sizemaximum',
                 'cost', 'duration', 'duration_per', 'search', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class OperationAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Operation.objects.all()
    serializer_class = OperationSerializer


    filter_fields = ('name', 'type', 'description', 'category', 'subcategory', 'location', 'fence',
                'posttime', 'sizeminimum', 'sizemultiple', 'sizemaximum',
                 'cost', 'duration', 'duration_per', 'search', 'source', 'lastmodified')

class OperationdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Operation.objects.all()
    serializer_class = OperationSerializer


class SubOperationSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.SubOperation
      fields = ('id','operation', 'priority', 'suboperation', 'effective_start', 'effective_end', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class SubOperationAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.SubOperation.objects.all()
    serializer_class = SubOperationSerializer


    filter_fields = ('id','operation', 'priority', 'suboperation', 'effective_start', 'effective_end', 'source', 'lastmodified')

class SubOperationdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.SubOperation.objects.all()
    serializer_class = SubOperationSerializer


class BufferSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Buffer
      fields = ('name', 'description', 'category', 'subcategory', 'type', 'location', 'item',
                'onhand', 'minimum', 'minimum_calendar', 'producing', 'leadtime', 'fence',
                'min_inventory', 'max_inventory', 'min_interval', 'max_interval',
                'size_minimum', 'size_multiple', 'size_maximum', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class BufferAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Buffer.objects.all()
    serializer_class = BufferSerializer


    filter_fields = ('name', 'description', 'category', 'subcategory', 'type', 'location', 'item',
                'onhand', 'minimum', 'minimum_calendar', 'producing', 'leadtime', 'fence',
                'min_inventory', 'max_inventory', 'min_interval', 'max_interval',
                'size_minimum', 'size_multiple', 'size_maximum', 'source', 'lastmodified')

class BufferdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Buffer.objects.all()
    serializer_class = BufferSerializer


class SetupMatrixSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.SetupMatrix
      fields = ('name', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class SetupMatrixAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.SetupMatrix.objects.all()
    serializer_class = SetupMatrixSerializer


    filter_fields = ('name', 'source', 'lastmodified')

class SetupMatrixdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.SetupMatrix.objects.all()
    serializer_class = SetupMatrixSerializer


class SetupRuleSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.SetupRule
      fields = ('setupmatrix', 'fromsetup', 'tosetup', 'duration', 'cost')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'setupmatrix'
      partial=True

class SetupRuleAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.SetupRule.objects.all()
    serializer_class = SetupRuleSerializer


    filter_fields = ('setupmatrix', 'fromsetup', 'tosetup', 'duration', 'cost')

class SetupRuledetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.SetupRule.objects.all()
    serializer_class = SetupRuleSerializer


class ResourceSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Resource
      fields = ('name', 'description', 'category', 'subcategory', 'type', 'maximum', 'maximum_calendar',
                'location',  'cost', 'maxearly', 'setupmatrix', 'setup', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class ResourceAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Resource.objects.all()
    serializer_class = ResourceSerializer


    filter_fields = ('name', 'description', 'category', 'subcategory', 'type', 'maximum', 'maximum_calendar',
                'location',  'cost', 'maxearly', 'setupmatrix', 'setup', 'source', 'lastmodified')

class ResourcedetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Resource.objects.all()
    serializer_class = ResourceSerializer


class SkillSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Skill
      fields = ('name', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class SkillAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Skill.objects.all()
    serializer_class = SkillSerializer


    filter_fields = ('name', 'source', 'lastmodified')

class SkilldetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Skill.objects.all()
    serializer_class = SkillSerializer


class ResourceSkillSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.ResourceSkill
      fields = ('id', 'skill', 'effective_start', 'effective_end', 'priority', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class ResourceSkillAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.ResourceSkill.objects.all()
    serializer_class = ResourceSkillSerializer


    filter_fields = ('id', 'skill', 'effective_start', 'effective_end', 'priority', 'source', 'lastmodified')

class ResourceSkilldetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.ResourceSkill.objects.all()
    serializer_class = ResourceSkillSerializer


class FlowSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Flow
      fields = ('id', 'operation', 'thebuffer', 'quantity', 'type', 'effective_start', 'effective_end',
                'name', 'alternate', 'priority', 'search', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class FlowAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Flow.objects.all()
    serializer_class = FlowSerializer


    filter_fields = ('id', 'operation', 'thebuffer', 'quantity', 'type', 'effective_start', 'effective_end',
                'name', 'alternate', 'priority', 'search', 'source', 'lastmodified')

class FlowdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Flow.objects.all()
    serializer_class = FlowSerializer


class LoadSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Load
      fields = ('id', 'operation', 'resource', 'skill', 'quantity', 'effective_start', 'effective_end',
                'name', 'alternate', 'priority', 'setup', 'search', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class LoadAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Load.objects.all()
    serializer_class = LoadSerializer


    filter_fields = ('id', 'operation', 'resource', 'skill', 'quantity', 'effective_start', 'effective_end',
                'name', 'alternate', 'priority', 'setup', 'search', 'source', 'lastmodified')

class LoaddetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Load.objects.all()
    serializer_class = LoadSerializer


class OperationPlanSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.OperationPlan
      fields = ('id', 'status', 'reference', 'operation', 'quantity', 'startdate', 'enddate',
                'criticality', 'owner', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class OperationPlanAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.OperationPlan.objects.all()
    serializer_class = OperationPlanSerializer


    filter_fields = ('id', 'status', 'reference', 'operation', 'quantity', 'startdate', 'enddate',
                'criticality', 'owner', 'source', 'lastmodified')

class OperationPlandetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.OperationPlan.objects.all()
    serializer_class = OperationPlanSerializer


class DistributionOrderSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.DistributionOrder
      fields = ('id', 'reference', 'status', 'item', 'origin', 'destination', 'quantity',
                'startdate', 'enddate', 'consume_material', 'criticality', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class DistributionOrderAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.DistributionOrder.objects.all()
    serializer_class = DistributionOrderSerializer


    filter_fields = ('id', 'reference', 'status', 'item', 'origin', 'destination', 'quantity',
                'startdate', 'enddate', 'consume_material', 'criticality', 'source', 'lastmodified')

class DistributionOrderdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.DistributionOrder.objects.all()
    serializer_class = DistributionOrderSerializer


class PurchaseOrderSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.PurchaseOrder
      fields = ('id', 'reference', 'status', 'item', 'supplier', 'location', 'quantity',
                'startdate', 'enddate', 'criticality', 'source', 'lastmodified')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'id'
      partial=True

class PurchaseOrderAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.PurchaseOrder.objects.all()
    serializer_class = PurchaseOrderSerializer


    filter_fields = ('id', 'reference', 'status', 'item', 'supplier', 'location', 'quantity',
                'startdate', 'enddate', 'criticality', 'source', 'lastmodified')

class PurchaseOrderdetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.PurchaseOrder.objects.all()
    serializer_class = PurchaseOrderSerializer


class DemandSerializer(BulkSerializerMixin, ModelSerializer):
    class Meta:
      model = freppledb.input.models.Demand
      fields = ('name', 'description', 'category', 'subcategory', 'item', 'location', 'due',
                'status', 'operation', 'quantity', 'priority', 'minshipment', 'maxlateness')
      list_serializer_class = BulkListSerializer
      update_lookup_field = 'name'
      partial=True

class DemandAPI(frePPleListCreateAPIView):
    queryset = freppledb.input.models.Demand.objects.all()
    serializer_class = DemandSerializer


    filter_fields = ('name', 'description', 'category', 'subcategory', 'item', 'location', 'due',
                'status', 'operation', 'quantity', 'priority', 'minshipment', 'maxlateness')

class DemanddetailAPI(frePPleRetrieveUpdateDestroyAPIView):
    queryset = freppledb.input.models.Demand.objects.all()
    serializer_class = DemandSerializer
