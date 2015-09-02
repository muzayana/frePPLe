#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _

from freppledb.inventoryplanning.models import InventoryPlanning
from freppledb.admin import data_site
from freppledb.common.adminforms import MultiDBModelAdmin


class InventoryPlanning_admin(MultiDBModelAdmin):
  model = InventoryPlanning
  raw_id_fields = ('buffer',)
  save_on_top = True
  fieldsets = (
    (None, {'fields': ('buffer', 'nostock')}),
    (_('Reorder quantity'), {
      'fields': (
        ('roq_min_qty', 'roq_max_qty', 'roq_multiple_qty'), 
        ('roq_min_poc', 'roq_max_poc')
        ),
      }),
    (_('Safety stock quantity'), {
      'fields': (
        'service_level', 
        ('ss_min_qty', 'ss_max_qty', 'ss_multiple_qty'), 
        ('ss_min_poc', 'ss_max_poc')
        ),
      }),
    )
data_site.register(InventoryPlanning, InventoryPlanning_admin)
