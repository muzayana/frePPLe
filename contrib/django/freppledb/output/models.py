#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _
from django.db import models
from django.conf import settings


class OperationPlan(models.Model):
  # Database fields
  id = models.IntegerField(_('identifier'), primary_key=True)
  operation = models.CharField(_('operation'), max_length=settings.NAMESIZE, db_index=True, null=True)
  quantity = models.DecimalField(_('quantity'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='1.00')
  unavailable = models.DecimalField(_('unavailable'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  startdate = models.DateTimeField(_('startdate'), db_index=True)
  enddate = models.DateTimeField(_('enddate'), db_index=True)
  locked = models.BooleanField(_('locked'), default=True)
  owner = models.IntegerField(_('owner'), null=True, blank=True, db_index=True)

  def __unicode__(self):
    return "Operationplan %s" % self.id

  class Meta:
    db_table = 'out_operationplan'
    permissions = (("view_operationplan", "Can view operation plans"),)
    verbose_name = _('operationplan')
    verbose_name_plural = _('operationplans')


class Problem(models.Model):
  # Database fields
  entity = models.CharField(_('entity'), max_length=15, db_index=True)
  owner = models.CharField(_('owner'), max_length=settings.NAMESIZE, db_index=True)
  name = models.CharField(_('name'), max_length=20, db_index=True)
  description = models.CharField(_('description'), max_length=settings.NAMESIZE+20)
  startdate = models.DateTimeField(_('start date'), db_index=True)
  enddate = models.DateTimeField(_('end date'), db_index=True)
  weight = models.DecimalField(_('weight'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES)

  def __unicode__(self): return str(self.description)

  class Meta:
    db_table = 'out_problem'
    permissions = (("view_problem", "Can view problems"),)
    ordering = ['startdate']
    verbose_name = _('problem')
    verbose_name_plural = _('problems')


class Constraint(models.Model):
  # Database fields
  demand = models.CharField(_('demand'), max_length=settings.NAMESIZE, db_index=True)
  entity = models.CharField(_('entity'), max_length=15, db_index=True)
  owner = models.CharField(_('owner'), max_length=settings.NAMESIZE, db_index=True)
  name = models.CharField(_('name'), max_length=20, db_index=True)
  description = models.CharField(_('description'), max_length=settings.NAMESIZE+20)
  startdate = models.DateTimeField(_('start date'), db_index=True)
  enddate = models.DateTimeField(_('end date'), db_index=True)
  weight = models.DecimalField(_('weight'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES)

  def __unicode__(self): return str(self.demand) + ' ' + str(self.description)

  class Meta:
    db_table = 'out_constraint'
    permissions = (("view_constraint", "Can view constraints"),)
    ordering = ['demand','startdate']
    verbose_name = _('constraint')
    verbose_name_plural = _('constraints')


class ResourceSummary(models.Model):
  theresource = models.CharField(_('resource'), max_length=settings.NAMESIZE)
  startdate = models.DateTimeField(_('startdate'))
  available = models.DecimalField(_('available'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True)
  unavailable = models.DecimalField(_('unavailable'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True)
  setup = models.DecimalField(_('setup'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True)
  load = models.DecimalField(_('load'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True)
  free = models.DecimalField(_('free'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True)

  class Meta:
    db_table = 'out_resourceplan'
    permissions = (("view_loadplans", "Can view load plans"),)
    ordering = ['theresource','startdate']
    unique_together = (('theresource', 'startdate'),)
    verbose_name = 'resource summary'  # No need to translate these since only used internally
    verbose_name_plural = 'resource summaries'


class LoadPlan(models.Model):
  # Database fields
  theresource = models.CharField(_('resource'), max_length=settings.NAMESIZE, db_index=True)
  quantity = models.DecimalField(_('quantity'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES)
  startdate = models.DateTimeField(_('startdate'), db_index=True)
  enddate = models.DateTimeField(_('enddate'), db_index=True)
  operationplan = models.ForeignKey(OperationPlan, verbose_name=_('operationplan'), db_index=True, related_name='loadplans')
  setup = models.CharField(_('setup'), max_length=settings.NAMESIZE, null=True)

  def __unicode__(self):
      return self.theresource + ' ' + str(self.startdate) + ' ' + str(self.enddate)

  class Meta:
    db_table = 'out_loadplan'
    permissions = (("view_loadplans", "Can view load plans"),)
    ordering = ['theresource','startdate']
    verbose_name = _('loadplan')
    verbose_name_plural = _('loadplans')


class FlowPlan(models.Model):
  # Database fields
  thebuffer = models.CharField(_('buffer'), max_length=settings.NAMESIZE, db_index=True)
  operationplan = models.ForeignKey(OperationPlan, verbose_name=_('operationplan'), db_index=True, related_name='flowplans')
  quantity = models.DecimalField(_('quantity'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES)
  flowdate = models.DateTimeField(_('date'), db_index=True)
  onhand = models.DecimalField(_('onhand'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES)

  def __unicode__(self):
    return self.thebuffer.name + str(self.flowdate)

  class Meta:
    db_table = 'out_flowplan'
    permissions = (("view_flowplans", "Can view flow plans"),)
    ordering = ['thebuffer','flowdate']
    verbose_name = _('flowplan')
    verbose_name_plural = _('flowplans')


class Demand(models.Model):
  # Database fields
  demand = models.CharField(_('demand'), max_length=settings.NAMESIZE, db_index=True, null=True)
  item = models.CharField(_('item'), max_length=settings.NAMESIZE, db_index=True, null=True)
  customer = models.CharField(_('customer'), max_length=settings.NAMESIZE, db_index=True, null=True)
  due = models.DateTimeField(_('due'), db_index=True)
  quantity = models.DecimalField(_('demand quantity'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  planquantity = models.DecimalField(_('planned quantity'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', null=True)
  plandate = models.DateTimeField(_('planned date'), null=True, db_index=True)
  operationplan = models.IntegerField(_('operationplan'), null=True, db_index=True)

  def __unicode__(self):
    return self.demand.name

  class Meta:
    db_table = 'out_demand'
    ordering = ['id']
    verbose_name = _('demand')
    verbose_name_plural = _('demands')


class DemandPegging(models.Model):
  # Database fields
  demand = models.CharField(_('demand'), max_length=settings.NAMESIZE, db_index=True)
  depth = models.IntegerField(_('depth'))
  cons_operationplan = models.IntegerField(_('consuming operationplan'), db_index=True, null=True)
  cons_date = models.DateTimeField(_('consuming date'))
  prod_operationplan = models.IntegerField(_('producing operationplan'), db_index=True, null=True)
  prod_date = models.DateTimeField(_('producing date'))
  buffer = models.CharField(_('buffer'), max_length=settings.NAMESIZE, db_index=True, null=True)
  item = models.CharField(_('item'), max_length=settings.NAMESIZE, null=True)
  quantity_demand = models.DecimalField(_('quantity demand'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  quantity_buffer = models.DecimalField(_('quantity buffer'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')

  def __unicode__(self):
    return self.demand \
      + ' - ' + str(self.depth) + ' - ' + str(self.cons_operationplan or 'None') \
      + ' - ' + self.buffer

  class Meta:
    db_table = 'out_demandpegging'
    ordering = ['id']
    verbose_name = _('demand pegging')
    verbose_name_plural = _('demand peggings')
