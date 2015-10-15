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
    ('negative binomial', _('Negative Binomial')),
  )

  # TODO combined method is currently disabled
  calculationtype = (
    #('combined', _("combined")),
    ('calculated', _("calculated")),
    ('quantity', _("quantity")),
    ('periodofcover', _("period of cover")),
  )

  # Database fields
  buffer = models.OneToOneField(Buffer, primary_key=True)
  roq_type = models.CharField(
    _('ROQ type'), null=True, blank=True, max_length=20, choices=calculationtype
    )
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
  ss_type = models.CharField(
    _('Safety stock type'), null=True, blank=True, max_length=20, choices=calculationtype
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
  nostock = models.BooleanField(
    _("Do not stock"), blank=True, default=False,
    help_text=_("Enforces a safety stock of 0 and a reorder quantity of 1")
    )

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

  Two processes are updating the contents in this table:
    - A strategic planning process which computes the safety stock
      and reorder quantities.
      This is typically run on a monthly or weekly basis, as soon as a new
      planning period is started.
    - A tactical planning process evaluates the current stock
      compared to the ideal computed in the previous step.
      This is typically run daily, and also when purchase orders and
      distribution orders are changing status.
  '''
  # Database fields - key
  buffer = models.OneToOneField(Buffer, primary_key=True)

  # Database fields - computed by the inventory planning run
  leadtime = models.DurationField(_('lead time'), db_index=True, null=True)
  calculatedreorderquantity = models.DecimalField(_('calculated reorder quantity'), max_digits=15, decimal_places=4, null=True)
  calculatedsafetystock = models.DecimalField(_('calculated safety stock'), max_digits=15, decimal_places=4, null=True)

  # Database fields - computed by the inventory planning run
  # The following fields are exported by the inventory planning run as a calendar.
  # The stock position run copies the result in this table.
  safetystock = models.DecimalField(_('safety stock'), max_digits=15, decimal_places=4, null=True)
  reorderquantity = models.DecimalField(_('reorder quantity'), max_digits=15, decimal_places=4, null=True)
  safetystockvalue = models.DecimalField(_('safety stock value'), max_digits=15, decimal_places=4, null=True)
  reorderquantityvalue = models.DecimalField(_('reorder quantity value'), max_digits=15, decimal_places=4, null=True)

  # Database fields - computed by the stock position run
  onhand = models.DecimalField(_('onhand'), max_digits=15, decimal_places=4, null=True)
  overduesalesorders = models.DecimalField(_('overdue sales orders'), max_digits=15, decimal_places=4, null=True)
  opensalesorders = models.DecimalField(_('open sales orders'), max_digits=15, decimal_places=4, null=True)
  proposedpurchases = models.DecimalField(_('proposed purchases'), max_digits=15, decimal_places=4, null=True)
  proposedtransfers = models.DecimalField(_('proposed transfers'), max_digits=15, decimal_places=4, null=True)
  openpurchases = models.DecimalField(_('open purchases'), max_digits=15, decimal_places=4, null=True)
  opentransfers = models.DecimalField(_('open transfers'), max_digits=15, decimal_places=4, null=True)
  onhandvalue = models.DecimalField(_('onhand value'), max_digits=15, decimal_places=4, null=True)
  overduesalesordersvalue = models.DecimalField(_('overdue sales orders value'), max_digits=15, decimal_places=4, null=True)
  opensalesordersvalue = models.DecimalField(_('open sales orders value'), max_digits=15, decimal_places=4, null=True)
  proposedpurchasesvalue = models.DecimalField(_('proposed purchases value'), max_digits=15, decimal_places=4, null=True)
  proposedtransfersvalue = models.DecimalField(_('proposed transfers value'), max_digits=15, decimal_places=4, null=True)
  openpurchasesvalue = models.DecimalField(_('open purchases value'), max_digits=15, decimal_places=4, null=True)
  opentransfersvalue = models.DecimalField(_('open transfers value'), max_digits=15, decimal_places=4, null=True)
  localforecast = models.DecimalField(_('local forecast'), max_digits=15, decimal_places=4, null=True)
  dependentdemand = models.DecimalField(_('dependent demand'), max_digits=15, decimal_places=4, null=True)
  totaldemand = models.DecimalField(_('total demand'), max_digits=15, decimal_places=4, null=True)
  localforecastvalue = models.DecimalField(_('local forecast value'), max_digits=15, decimal_places=4, null=True)
  dependentdemandvalue = models.DecimalField(_('dependent demand value'), max_digits=15, decimal_places=4, null=True)
  totaldemandvalue = models.DecimalField(_('total demand value'), max_digits=15, decimal_places=4, null=True)
  # TODO stockoutrisk = models.DecimalField(_('stockout risk'), max_digits=15, decimal_places=4, null=True)
  
  def __str__(self):
    return self.buffer.name

  class Meta:
    db_table = 'out_inventoryplanning'
    ordering = ['buffer']
