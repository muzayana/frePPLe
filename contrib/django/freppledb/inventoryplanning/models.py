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
    ('automatic', _('Automatic')),
    # Translators: Normal statistical distribution, aka Gaussian distribution
    ('normal', _('Normal')),
    # Translators: Poisson statistical distribution
    ('poisson', _('Poisson')),
    # Translators: Negative binomial statistical distribution
    ('negative binomial', _('Negative binomial')),
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
