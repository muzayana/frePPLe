#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.  
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code 
# or in the form of compiled binaries.
#

# file : $URL: file:///C:/Users/Johan/Dropbox/SVNrepository/frepple/addon/contrib/django/freppledb_extra/models.py $
# revision : $LastChangedRevision: 449 $  $LastChangedBy: Johan $
# date : $LastChangedDate: 2012-12-28 18:59:56 +0100 (Fri, 28 Dec 2012) $

from decimal import Decimal

from django.db import models
from django.utils.translation import ugettext_lazy as _
from django.conf import settings

from freppledb.common.models import AuditModel
from freppledb.input.models import Customer, Operation, Item, Calendar, Demand


class Forecast(AuditModel):
  # Forecasting methods
  methods = (
    ('automatic',_('Automatic')),
    ('constant',_('Constant')),
    ('trend',_('Trend')),
    ('seasonal',_('Seasonal')),
    ('intermittent',_('Intermittent')),
    ('manual',_('Manual')),
  )
  
  # Database fields
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True)
  description = models.CharField(_('description'), max_length=settings.DESCRIPTIONSIZE, null=True, blank=True)
  category = models.CharField(_('category'), max_length=settings.CATEGORYSIZE, null=True, blank=True, db_index=True)
  subcategory = models.CharField(_('subcategory'), max_length=settings.CATEGORYSIZE, null=True, blank=True, db_index=True)
  customer = models.ForeignKey(Customer, verbose_name=_('customer'), null=True, blank=True, db_index=True)
  item = models.ForeignKey(Item, verbose_name=_('item'), db_index=True)
  method = models.CharField(_('Forecast method'), max_length=20, null=True, blank=True, choices=methods, default='automatic',
    help_text=_('Method used to generate a base forecast'),
    )
  calendar = models.ForeignKey(Calendar, verbose_name=_('calendar'), null=False)
  operation = models.ForeignKey(Operation, verbose_name=_('delivery operation'), null=True, blank=True,
    related_name='used_forecast', help_text=_('Operation used to satisfy this demand'))
  priority = models.PositiveIntegerField(_('priority'), default=10, choices=Demand.demandpriorities,
    help_text=_('Priority of the demand (lower numbers indicate more important demands)'))
  minshipment = models.DecimalField(_('minimum shipment'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True, blank=True,
    help_text=_('Minimum shipment quantity when planning this demand'))
  maxlateness = models.DecimalField(_('maximum lateness'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, null=True, blank=True,
    help_text=_("Maximum lateness allowed when planning this demand"))
  discrete = models.BooleanField(_('discrete'),default=True, help_text=_('Round forecast numbers to integers'))

  # Convenience methods
  def __unicode__(self): return self.name

  def setTotal(self, startdate, enddate, quantity):
    '''
    Update the forecast quantity.
    The logic followed is three-fold:
      - If one or more forecast entries already exist in the daterange, the
        quantities of those entries are proportionally rescaled to fit the
        new quantity.
      - When no entry exists yet, we simply create a single forecast entry 
        for the specified daterange.
    '''
    # Assure the end date is later than the start date.
    if startdate > enddate:
      tmp = startdate
      startdate = enddate
      enddate = tmp
    # Assure the type of the quantity
    if not isinstance(quantity,Decimal): quantity = Decimal(str(quantity))
    # Round the quantity, if discrete flag is on
    if self.discrete: quantity = quantity.to_integral()
    # Step 0: Check for forecast entries intersecting with the current daterange
    startdate = startdate.date()
    enddate = enddate.date()
    entries = self.entries.filter(enddate__gt=startdate).filter(startdate__lt=enddate)
    if not entries:
      # Case 1: No intersecting forecast entries exist yet. 
      # We just create an entry for the given start and end date
      # Note: if the calendar values are updated later on, such changes are
      # obviously not reflected any more in the forecast entries.
      self.entries.create(startdate=startdate,enddate=enddate,quantity=str(quantity)).save()
    else:
      # Case 2: Entries already exist in this daterange, which will be rescaled
      # Case 1, step 1: calculate current quantity and "clip" the existing entries
      # if required.
      current = 0
      for i in entries:
        # Calculate the length of this bucket in seconds
        duration = i.enddate - i.startdate
        duration = duration.days+86400*duration.seconds
        if i.startdate == startdate and i.enddate == enddate:
          # This entry has exactly the same daterange: update the quantity and exit
          i.quantity = str(quantity)
          i.save()
          return
        elif i.startdate < startdate and i.enddate > enddate:
          # This bucket starts before the daterange and also ends later.
          # We need to split the entry in three.
          # Part one: after our daterange, create a new entry
          p = i.enddate - enddate
          q = i.quantity * (p.days+86400*p.seconds) / duration
          if self.discrete: q = round(q)
          self.entries.create( \
             startdate = enddate,
             enddate = i.enddate,
             quantity = str(q),
             ).save()
          # Part two: our date range, create a new entry
          self.entries.create( \
             startdate = startdate,
             enddate = enddate,
             quantity = str(quantity),
             ).save()
          # Part three: before our daterange, update the existing entry
          p = startdate - i.startdate
          i.enddate = startdate
          i.quantity = i.quantity * (p.days+86400*p.seconds) / duration
          if self.discrete: i.quantity = round(i.quantity)
          i.quantity = str(i.quantity)
          i.save()
          # Done with this case...
          return
        elif i.startdate >= startdate and i.enddate <= enddate:
          # Entry falls completely in the range
          # TODO Incomplete???
          current += i.quantity
        elif i.startdate < enddate and i.enddate >= enddate:
          # This entry starts in the range and ends later.
          # Split the entry in two.
          p = i.enddate - enddate
          fraction = Decimal(i.quantity * (p.days+86400*p.seconds) / duration)
          current += i.quantity - fraction
          self.entries.create( \
             startdate = i.startdate,
             enddate = enddate,
             quantity = str(i.quantity - fraction),
             ).save()
          i.startdate = enddate
          if self.discrete: i.quantity = str(round(fraction))
          else: i.quantity = str(fraction)
          i.save()
        elif i.enddate > startdate and i.startdate <= startdate:
          # This entry ends in the range and starts earlier.
          # Split the entry in two.
          p = startdate - i.startdate
          fraction = Decimal(i.quantity * (p.days+86400*p.seconds) / duration)
          current += i.quantity - fraction
          self.entries.create( \
             startdate = startdate,
             enddate = i.enddate,
             quantity = str(i.quantity - fraction),
             ).save()
          i.enddate = startdate
          if self.discrete: i.quantity = str(round(fraction))
          else: i.quantity = str(fraction)
          i.save()
      # Case 1, step 2: Rescale the existing entries
      # Note that we retrieve an updated set of buckets from the database here...
      entries = self.entries.filter(enddate__gt=startdate).filter(startdate__lt=enddate)
      factor = quantity / current
      if factor == 0:
        for i in entries: i.delete()
      elif self.discrete:
        # Only put integers
        remainder = 0
        for i in entries:
          q = Decimal(i.quantity * factor + remainder)
          i.quantity = q.to_integral()
          remainder = q - i.quantity
          i.quantity = str(i.quantity)
          i.save()
      else:
        # No rounding required
        for i in entries:
          i.quantity *= factor
          i.quantity = str(i.quantity)
          i.save()

  class Meta(AuditModel.Meta):
    db_table = 'forecast'
    verbose_name = _('forecast')
    verbose_name_plural = _('forecasts')
    ordering = ['name']
    permissions = (
      ("generate_baseline", "Can generate a baseline forecast"),
      )


class ForecastDemand(AuditModel):
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True)
  forecast = models.ForeignKey(Forecast, verbose_name=_('forecast'), null=False, db_index=True, related_name='entries')
  startdate = models.DateField(_('start date'), null=False)
  enddate = models.DateField(_('end date'), null=False)
  quantity = models.DecimalField(_('quantity'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default=0)

  # Convenience methods
  def __unicode__(self): return self.forecast.name + " " + str(self.startdate) + " - " + str(self.enddate)

  class Meta(AuditModel.Meta):
    db_table = 'forecastdemand'
    verbose_name = _('forecast demand')
    verbose_name_plural = _('forecast demands')


class ForecastPlan(models.Model):
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True)
  forecast = models.ForeignKey(Forecast, verbose_name=_('forecast'), db_index=True, related_name='plans')
  customerlvl = models.PositiveIntegerField(null=True, editable=False, blank=True)
  itemlvl = models.PositiveIntegerField(null=True, editable=False, blank=True)
  startdate = models.DateTimeField(_('start date'), null=False, db_index=True)
  orderstotal = models.DecimalField(_('total orders'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  ordersopen = models.DecimalField(_('open orders'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  forecastbaseline = models.DecimalField(_('forecast baseline'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  forecastadjustment = models.DecimalField(_('forecast adjustment'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  forecasttotal = models.DecimalField(_('forecast total'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  forecastnet = models.DecimalField(_('forecast net'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')
  forecastconsumed = models.DecimalField(_('forecast consumed'), max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00')

  def __unicode__(self):
    return "%s - %s" % (self.forecast.name, str(self.startdate))

  class Meta:
    db_table = 'forecastplan'
    ordering = ['id']
    verbose_name = _('forecast plan')
    verbose_name_plural = _('forecast plans')
