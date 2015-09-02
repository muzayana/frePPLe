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
    _('ROQ minimum quantity'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  roq_max_qty = models.DecimalField(
    _('ROQ maximum quantity'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  roq_multiple_qty = models.DecimalField(
    _('ROQ multiple quantity'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  roq_min_poc = models.DecimalField(
    _('ROQ minimum period of cover'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  roq_max_poc = models.DecimalField(
    _('ROQ maximum period of cover'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  leadtime_deviation = models.DecimalField(
    _('lead time deviation'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  demand_deviation = models.DecimalField(
    _('demand deviation'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  demand_distribution = models.CharField(
    _('demand distribution'), null=True, blank=True, max_length=20, choices=distributions
    )
  service_level = models.DecimalField(
    _('service level'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  ss_min_qty = models.DecimalField(
    _('safety stock minimum quantity'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  ss_max_qty = models.DecimalField(
    _('safety stock maximum quantity'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  ss_multiple_qty = models.DecimalField(
    _('safety stock multiple quantity'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  ss_min_poc = models.DecimalField(
    _('safety stock minimum period of cover'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  ss_max_poc = models.DecimalField(
    _('safety stock maximum period of cover'), max_digits=settings.MAX_DIGITS,
    decimal_places=settings.DECIMAL_PLACES, null=True, blank=True
    )
  nostock = models.BooleanField(_("Do not stock"), blank=True, default=False)

  class Meta(AuditModel.Meta):
    db_table = 'inventory_planning'
    verbose_name = _('inventory planning parameter')
    verbose_name_plural = _('inventory planning parameters')
    ordering = ['buffer']

  def __unicode__(self):
    return self.buffer
