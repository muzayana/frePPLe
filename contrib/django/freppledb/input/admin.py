#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _

from freppledb.input.models import Resource, Operation, Location, SetupMatrix, SetupRule
from freppledb.input.models import Buffer, Customer, Demand, Item, Load, Flow, Skill, ResourceSkill
from freppledb.input.models import Calendar, CalendarBucket, OperationPlan, SubOperation, Supplier
from freppledb.input.models import ItemSupplier, ItemDistribution, DistributionOrder, PurchaseOrder
from freppledb.common.adminforms import MultiDBModelAdmin, MultiDBTabularInline

from freppledb.admin import data_site

class CalendarBucket_inline(MultiDBTabularInline):
  model = CalendarBucket
  extra = 0
  exclude = ('source',)


class CalendarBucket_admin(MultiDBModelAdmin):
  model = CalendarBucket
  raw_id_fields = ('calendar',)
  save_on_top = True
  fieldsets = (
    (None, {'fields': ('calendar', ('startdate', 'enddate'), 'value', 'priority')}),
    (_('Repeating pattern'), {
      'fields': (('starttime', 'endtime'), ('monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday', 'sunday')),
      }),
    )
  tabs = [
    {"name": 'edit', "label": _("edit"), "view":  "admin:input_calendarbucket_change", "permissions": "input.change_calendarbucket"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_calendarbucket_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_calendarbucket_history"},
    ]
data_site.register(CalendarBucket, CalendarBucket_admin)


class Calendar_admin(MultiDBModelAdmin):
  model = Calendar
  save_on_top = True
  inlines = [ CalendarBucket_inline, ]
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_calendar_change", "permissions": "input.change_calendar"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_calendar_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_calendar_history"},
    ]
data_site.register(Calendar, Calendar_admin)


class Location_admin(MultiDBModelAdmin):
  model = Location
  raw_id_fields = ('available', 'owner',)
  save_on_top = True
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_location_change", "permissions": "input.change_location"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_location_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_location_history"},
    ]
data_site.register(Location, Location_admin)


class Customer_admin(MultiDBModelAdmin):
  model = Customer
  raw_id_fields = ('owner',)
  save_on_top = True
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_customer_change", "permissions": "input.change_customer"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_customer_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_customer_history"},
    ]
data_site.register(Customer, Customer_admin)


class ItemSupplier_inline(MultiDBTabularInline):
  model = ItemSupplier
  fk_name = 'item'
  raw_id_fields = ('supplier','location')
  extra = 0
  exclude = ('source',)



class Supplier_admin(MultiDBModelAdmin):
  model = Supplier
  raw_id_fields = ('owner',)
  save_on_top = True
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_supplier_change", "permissions": "input.change_supplier"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_supplier_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_supplier_history"},
    ]
data_site.register(Supplier, Supplier_admin)


class Item_admin(MultiDBModelAdmin):
  model = Item
  save_on_top = True
  raw_id_fields = ('operation', 'owner',)
  inlines = [ ItemSupplier_inline, ]
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_item_change", "permissions": "input.change_item"},
    {"name": 'supplypath', "label": _("supply path"), "view": "supplypath_item"},
    {"name": 'whereused', "label": _("where used"),"view": "whereused_item"},
    {"name": 'plan', "label": _("plan"), "view": "output_demand_plandetail"},
    {"name": 'plandetail', "label": _("plan detail"), "view": "output_demandplan_plandetail"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_item_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_item_history"},
    ]
data_site.register(Item, Item_admin)


class ItemSupplier_admin(MultiDBModelAdmin):
  model = ItemSupplier
  save_on_top = True
  raw_id_fields = ('item', 'supplier')
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_itemsupplier_change", "permissions": "input.change_itemsupplier"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_itemsupplier_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_itemsupplier_history"},
    ]
data_site.register(ItemSupplier, ItemSupplier_admin)


class ItemDistribution_admin(MultiDBModelAdmin):
  model = ItemDistribution
  save_on_top = True
  raw_id_fields = ('item',)
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_itemdistribution_change", "permissions": "input.change_itemdistribution"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_itemdistribution_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_itemdistribution_history"},
  ]
data_site.register(ItemDistribution, ItemDistribution_admin)


class SubOperation_inline(MultiDBTabularInline):
  model = SubOperation
  fk_name = 'operation'
  extra = 1
  raw_id_fields = ('suboperation',)
  exclude = ('source',)


class Flow_inline(MultiDBTabularInline):
  model = Flow
  fields = ('thebuffer', 'operation', 'quantity', 'type', 'effective_start', 'effective_end')
  raw_id_fields = ('operation', 'thebuffer',)
  extra = 0
  exclude = ('source',)


class Load_inline(MultiDBTabularInline):
  model = Load
  raw_id_fields = ('operation', 'resource', 'skill')
  fields = ('resource', 'operation', 'quantity', 'effective_start', 'effective_end', 'skill', 'setup')
  sfieldsets = (
    (None, {'fields': ['resource', 'operation', 'quantity', 'effective_start', 'effective_end', 'skill', 'setup']}),
    (_('Alternates'), {'fields': ('name', 'priority', 'search')}),
    )
  extra = 0
  exclude = ('source',)


class ResourceSkill_inline(MultiDBTabularInline):
  model = ResourceSkill
  fk_name = 'resource'
  raw_id_fields = ('skill',)
  extra = 1
  exclude = ('source',)


class Operation_admin(MultiDBModelAdmin):
  model = Operation
  raw_id_fields = ('location',)
  save_on_top = True
  inlines = [ SubOperation_inline, Flow_inline, Load_inline, ]
  fieldsets = (
    (None, {'fields': ('name', 'type', 'location', 'description', ('category', 'subcategory'))}),
    (_('Planning parameters'), {
      'fields': ('fence', 'posttime', 'sizeminimum', 'sizemultiple', 'sizemaximum', 'cost', 'duration', 'duration_per', 'search'),
       }),
    )
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_operation_change", "permissions": "input.change_operation"},
    {"name": 'supplypath', "label": _("supply path"), "view": "supplypath_operation"},
    {"name": 'whereused', "label": _("where used"),"view": "whereused_operation"},
    {"name": 'plan', "label": _("plan"), "view": "output_operation_plandetail"},
    {"name": 'plandetail', "label": _("plan detail"), "view": "output_operationplan_plandetail"},
    {"name": 'constraint', "label": _("constrained demand"), "view": "output_constraint_operation"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_operation_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_operation_history"},
  ]
data_site.register(Operation, Operation_admin)


class SubOperation_admin(MultiDBModelAdmin):
  model = SubOperation
  raw_id_fields = ('operation', 'suboperation',)
  save_on_top = True
  exclude = ('source',)
data_site.register(SubOperation, SubOperation_admin)


class Buffer_admin(MultiDBModelAdmin):
  raw_id_fields = ('location', 'item', 'minimum_calendar', 'producing', 'owner', )
  fieldsets = (
    (None, {
      'fields': ('name', 'item', 'location', 'description', 'owner', 'category', 'subcategory')}),
    (_('Inventory'), {
      'fields': ('onhand',)
      }),
    (_('Planning parameters'), {
      'fields': ('type', 'minimum', 'minimum_calendar', 'producing'),
      }),
    (_('Planning parameters for procurement buffers'), {
      'fields': ('leadtime', 'fence', 'min_inventory', 'max_inventory', 'min_interval', 'max_interval', 'size_minimum', 'size_multiple', 'size_maximum'),
      }),
    )
  save_on_top = True
  inlines = [ Flow_inline, ]
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_buffer_change", "permissions": "input.change_buffer"},
    {"name": 'supplypath', "label": _("supply path"), "view": "supplypath_buffer"},
    {"name": 'whereused', "label": _("where used"),"view": "whereused_buffer"},
    {"name": 'plan', "label": _("plan"), "view": "output_buffer_plandetail"},
    {"name": 'plandetail', "label": _("plan detail"), "view": "output_flowplan_plandetail"},
    {"name": 'constraint', "label": _("constrained demand"), "view": "output_constraint_buffer"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_buffer_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_buffer_history"},
    ]
data_site.register(Buffer, Buffer_admin)


class SetupRule_inline(MultiDBTabularInline):
  model = SetupRule
  extra = 3
  exclude = ('source',)


class SetupMatrix_admin(MultiDBModelAdmin):
  model = SetupMatrix
  save_on_top = True
  inlines = [ SetupRule_inline, ]
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_setupmatrix_change", "permissions": "input.change_setupmatrix"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_setupmatrix_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_setupmatrix_history"},
    ]
data_site.register(SetupMatrix, SetupMatrix_admin)


class Skill_admin(MultiDBModelAdmin):
  model = Skill
  save_on_top = True
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_skill_change", "permissions": "input.change_skill"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_skill_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_skill_history"},
    ]
data_site.register(Skill, Skill_admin)


class ResourceSkill_admin(MultiDBModelAdmin):
  model = ResourceSkill
  raw_id_fields = ('resource', 'skill')
  save_on_top = True
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_resourceskill_change", "permissions": "input.change_resoureskill"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_resourceskill_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_resourceskill_history"},
    ]
data_site.register(ResourceSkill, ResourceSkill_admin)


class Resource_admin(MultiDBModelAdmin):
  model = Resource
  raw_id_fields = ('maximum_calendar', 'location', 'setupmatrix', 'owner')
  save_on_top = True
  inlines = [ Load_inline, ResourceSkill_inline, ]
  exclude = ('source',)
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_resource_change", "permissions": "input.change_resource"},
    {"name": 'supplypath', "label": _("supply path"), "view": "supplypath_resource"},
    {"name": 'whereused', "label": _("where used"),"view": "whereused_resource"},
    {"name": 'plan', "label": _("plan"), "view": "output_resource_plandetail"},
    {"name": 'gantt', "label": _("gantt chart"), "view": "output_resource_ganttdetail"},
    {"name": 'plandetail', "label": _("plan detail"), "view": "output_loadplan_plandetail"},
    {"name": 'constraint', "label": _("constrained demand"), "view": "output_constraint_resource"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_resource_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_resource_history"},
    ]
data_site.register(Resource, Resource_admin)


class Flow_admin(MultiDBModelAdmin):
  model = Flow
  raw_id_fields = ('operation', 'thebuffer',)
  save_on_top = True
  fieldsets = (
    (None, {'fields': ('thebuffer', 'operation', 'type', 'quantity', ('effective_start', 'effective_end'))}),
    (_('Alternates'), {'fields': ('name', 'priority', 'search'),}),
    )
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_flow_change", "permissions": "input.change_flow"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_flow_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_flow_history"},
    ]
data_site.register(Flow, Flow_admin)


class Load_admin(MultiDBModelAdmin):
  model = Load
  raw_id_fields = ('operation', 'resource', 'skill')
  save_on_top = True
  fieldsets = (
    (None, {'fields': ('resource', 'operation', 'quantity', 'skill', 'setup', ('effective_start', 'effective_end'))}),
    (_('Alternates'), {'fields': ('name', 'priority', 'search'),}),
    )
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_load_change", "permissions": "input.change_load"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_load_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_load_history"},
    ]
data_site.register(Load, Load_admin)


class OperationPlan_admin(MultiDBModelAdmin):
  model = OperationPlan
  raw_id_fields = ('operation', 'owner',)
  save_on_top = True
  exclude = ('id', 'source', 'criticality')
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_operationplan_change", "permissions": "input.change_operationplan"},
    ]
data_site.register(OperationPlan, OperationPlan_admin)


class DistributionOrder_admin(MultiDBModelAdmin):
  model = DistributionOrder
  raw_id_fields = ('item',)
  save_on_top = True
  exclude = ('id', 'source', 'criticality')
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_distributionorder_change", "permissions": "input.change_distributionorder"},
    ]
data_site.register(DistributionOrder, DistributionOrder_admin)


class PurchaseOrder_admin(MultiDBModelAdmin):
  model = PurchaseOrder
  raw_id_fields = ('item', 'supplier',)
  save_on_top = True
  exclude = ('id', 'source', 'criticality')
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_purchaseorder_change", "permissions": "input.change_purchaseorder"},
    ]
data_site.register(PurchaseOrder, PurchaseOrder_admin)


class Demand_admin(MultiDBModelAdmin):
  model = Demand
  raw_id_fields = ('customer', 'item', 'operation', 'owner',)
  fieldsets = (
    (None, {'fields': (
      'name', 'item', 'location', 'customer', 'description', 'category',
      'subcategory', 'due', 'quantity', 'priority', 'status', 'owner'
      )}),
    (_('Planning parameters'), {'fields': (
      'operation', 'minshipment', 'maxlateness'
      )}),
    )
  save_on_top = True
  tabs = [
    {"name": 'edit', "label": _("edit"), "view": "admin:input_demand_change", "permissions": "input.change_demand"},
    {"name": 'supplypath', "label": _("supply path"), "view": "supplypath_demand"},
    {"name": 'constraint', "label": _("why short or late?"),"view": "output_constraint_demand"},
    {"name": 'plan', "label": _("plan"), "view": "output_demand_pegging"},
    {"name": 'comments', "label": _("comments"), "view": "admin:input_demand_comment"},
    #. Translators: Translation included with Django
    {"name": 'history', "label": _("History"), "view": "admin:input_demand_history"},
    ]
data_site.register(Demand, Demand_admin)
