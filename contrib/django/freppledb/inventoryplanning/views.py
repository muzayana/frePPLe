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

from freppledb.common.report import GridFieldText, GridFieldInteger, GridReport
from freppledb.common.report import GridFieldLastModified, GridFieldChoice
from freppledb.common.report import GridFieldNumber, GridFieldBool

from freppledb.inventoryplanning.models import InventoryPlanning


class InventoryPlanningList(GridReport):
  '''
  A list report to show inventory planning parameters.
  '''
  template = 'inventoryplanning/inventoryplanninglist.html'
  title = _("Inventory Planning Parameter List")
  basequeryset = InventoryPlanning.objects.all()
  model = InventoryPlanning
  frozenColumns = 1

  rows = (
    GridFieldText('buffer', title=_('buffer'), key=True, formatter='buffer'),
    GridFieldBool('nostock', title=_("Do not stock")),
    GridFieldNumber('roq_min_qty', title=_('ROQ minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_max_qty', title=_('ROQ maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_multiple_qty', title=_('ROQ multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_min_poc', title=_('ROQ minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_max_poc', title=_('ROQ maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('leadtime_deviation', title=_('lead time deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('demand_deviation', title=_('demand deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldChoice('demand_distribution', title=_('demand distribution'), 
      choices=InventoryPlanning.distributions, extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('service_level', title=_('service level'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_qty', title=_('safety stock minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_max_qty', title=_('safety stock maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_multiple_qty', title=_('safety stock multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_poc', title=_('safety stock minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_max_poc', title=_('safety stock maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldText('source', title=_('source')),
    GridFieldLastModified('lastmodified'),
    )
