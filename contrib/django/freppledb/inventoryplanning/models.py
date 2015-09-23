#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db import models
from django.utils.translation import ugettext_lazy as _
from django.conf import settings

from freppledb.common.models import AuditModel
from freppledb.input.models import Buffer


class InventoryPlanning(AuditModel):
  distributions = (
    ('Automatic', _('Automatic')),
    # Translators: Normal statistical distribution, aka Gaussian distribution
    ('Normal', _('Normal')),
    # Translators: Poisson statistical distribution
    ('Poisson', _('Poisson')),
    # Translators: Negative binomial statistical distribution
    ('Negative Binomial', _('Negative Binomial')),
  )

  # Database fields
  buffer = models.OneToOneField(Buffer, primary_key=True)
  roq_min_qty = models.DecimalField(
    _('ROQ minimum quantity'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  roq_max_qty = models.DecimalField(
    _('ROQ maximum quantity'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  roq_multiple_qty = models.DecimalField(
    _('ROQ multiple quantity'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  roq_min_poc = models.DecimalField(
    _('ROQ minimum period of cover'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  roq_max_poc = models.DecimalField(
    _('ROQ maximum period of cover'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  leadtime_deviation = models.DecimalField(
    _('lead time deviation'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  demand_deviation = models.DecimalField(
    _('demand deviation'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  demand_distribution = models.CharField(
    _('demand distribution'), null=True, blank=True, max_length=20, choices=distributions
    )
  service_level = models.DecimalField(
    _('service level'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  ss_min_qty = models.DecimalField(
    _('safety stock minimum quantity'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  ss_max_qty = models.DecimalField(
    _('safety stock maximum quantity'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  ss_multiple_qty = models.DecimalField(
    _('safety stock multiple quantity'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  ss_min_poc = models.DecimalField(
    _('safety stock minimum period of cover'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  ss_max_poc = models.DecimalField(
    _('safety stock maximum period of cover'), max_digits=15,
    decimal_places=4, null=True, blank=True
    )
  nostock = models.BooleanField(_("Do not stock"), blank=True, default=False)

  class Meta(AuditModel.Meta):
    db_table = 'inventoryplanning'
    verbose_name = _('inventory planning parameter')
    verbose_name_plural = _('inventory planning parameters')
    ordering = ['buffer']

  def __str__(self):
    return self.buffer.name


class InventoryPlanningOutput(models.Model):
  '''
  This model stores the output result of the inventory planning and is the
  key driver for the distribution planning screen and workflows.

  The fields are populated from different source:
     - Populated by a run of the inventory planning module.
         - lead time
         - planned service level in the first bucket
         - local (forecast/)demand per period, averaged over the lead time
         - local dependent demand per period, averaged over the lead time
         - economic order quantity in the first bucket, unconstrained intermediate result
         - statistical distribution applied, intermediate result
         - safety stock in the first bucket, unconstrained intermediate result
         - safety stock in the first bucket, final value
         - reorder quantity in the first bucket
     - Populated by a daily update procedure (or computed on the fly!)
         - demand deviation computed?
         - local open orders
         - local open backorders
         - proposed purchases
         - proposed transfers in
         - proposed transfers out
         - period of cover
  '''
  # Database fields
  buffer = models.OneToOneField(Buffer, primary_key=True)
  leadtime = models.DurationField(_('lead time'), db_index=True, null=True)
  servicelevel = models.DecimalField(_('service level'), max_digits=15, decimal_places=4, null=True)
  localforecast = models.DecimalField(_('local forecast'), max_digits=15, decimal_places=4, null=True)
  localorders = models.DecimalField(_('local orders'), max_digits=15, decimal_places=4, null=True)
  localbackorders = models.DecimalField(_('local backorders'), max_digits=15, decimal_places=4, null=True)
  dependentforecast = models.DecimalField(_('dependent forecast'), max_digits=15, decimal_places=4, null=True)
  totaldemand = models.DecimalField(_('total demand'), max_digits=15, decimal_places=4, null=True)
  safetystock = models.DecimalField(_('safety stock'), max_digits=15, decimal_places=4, null=True)
  reorderquantity = models.DecimalField(_('reorder quantity'), max_digits=15, decimal_places=4, null=True)
  proposedpurchases = models.DecimalField(_('proposed purchases'), max_digits=15, decimal_places=4, null=True)
  proposedtransfers = models.DecimalField(_('proposed transfers'), max_digits=15, decimal_places=4, null=True)
  localforecastvalue = models.DecimalField(_('local forecast value'), max_digits=15, decimal_places=4, null=True)
  localordersvalue = models.DecimalField(_('local orders value'), max_digits=15, decimal_places=4, null=True)
  localbackordersvalue = models.DecimalField(_('local backorders value'), max_digits=15, decimal_places=4, null=True)
  dependentforecastvalue = models.DecimalField(_('dependent forecast value'), max_digits=15, decimal_places=4, null=True)
  totaldemandvalue = models.DecimalField(_('total demand value'), max_digits=15, decimal_places=4, null=True)
  safetystockvalue = models.DecimalField(_('safety stock value'), max_digits=15, decimal_places=4, null=True)
  reorderquantityvalue = models.DecimalField(_('reorder quantity value'), max_digits=15, decimal_places=4, null=True)
  proposedpurchasesvalue = models.DecimalField(_('proposed purchases value'), max_digits=15, decimal_places=4, null=True)
  proposedtransfersvalue = models.DecimalField(_('proposed transfers value'), max_digits=15, decimal_places=4, null=True)
  #  TODO other useful metrics:  OH, OO, POC, excess/shortage, EOQ,

  def __str__(self):
    return self.buffer.name

  class Meta:
    db_table = 'out_inventoryplanning'
    ordering = ['buffer']
