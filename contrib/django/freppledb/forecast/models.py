#
# Copyright (C) 2012 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from decimal import Decimal

from django.db import models, DEFAULT_DB_ALIAS
from django.utils.translation import pgettext, ugettext_lazy as _

from freppledb.common.models import AuditModel
from freppledb.input.models import Customer, Operation, Item, Calendar, Demand, Location


class Forecast(AuditModel):
  # Forecasting methods
  methods = (
    ('automatic', _('Automatic')),
    ('constant', pgettext("forecast method", 'Constant')),
    ('trend', pgettext("forecast method", 'Trend')),
    ('seasonal', pgettext("forecast method", 'Seasonal')),
    ('intermittent', pgettext("forecast method", 'Intermittent')),
    ('moving average', pgettext("forecast method", 'Moving average')),
    ('manual', _('Manual')),
  )

  # Database fields
  name = models.CharField(_('name'), max_length=300, primary_key=True)
  description = models.CharField(_('description'), max_length=500, null=True, blank=True)
  category = models.CharField(_('category'), max_length=300, null=True, blank=True, db_index=True)
  subcategory = models.CharField(_('subcategory'), max_length=300, null=True, blank=True, db_index=True)
  customer = models.ForeignKey(Customer, verbose_name=_('customer'), db_index=True, null=True, blank=True)
  item = models.ForeignKey(Item, verbose_name=_('item'), db_index=True)
  location = models.ForeignKey(Location, verbose_name=_('location'), db_index=True, null=True, blank=True)
  method = models.CharField(
    _('Forecast method'), max_length=20, null=True, blank=True,
    choices=methods, default='automatic',
    help_text=_('Method used to generate a base forecast')
    )
  operation = models.ForeignKey(
    Operation, verbose_name=_('delivery operation'), null=True, blank=True,
    related_name='used_forecast', help_text=_('Operation used to satisfy this demand')
    )
  priority = models.PositiveIntegerField(
    _('priority'), default=10, choices=Demand.demandpriorities,
    help_text=_('Priority of the demand (lower numbers indicate more important demands)')
    )
  minshipment = models.DecimalField(
    _('minimum shipment'), null=True, blank=True,
    max_digits=15, decimal_places=4,
    help_text=_('Minimum shipment quantity when planning this demand'))
  maxlateness = models.DecimalField(
    _('maximum lateness'), null=True, blank=True,
    max_digits=15, decimal_places=4,
    help_text=_("Maximum lateness allowed when planning this demand")
    )
  discrete = models.BooleanField(
    _('discrete'), default=True,
    help_text=_('Round forecast numbers to integers')
    )
  planned = models.BooleanField(
    _('planned'), default=True,
    help_text=_('Use this forecast for planning')
    )
  out_smape = models.DecimalField(
    _('estimated forecast error'), null=True, blank=True,
    max_digits=15, decimal_places=4
    )
  out_method = models.CharField(
    _('calculated forecast method'), max_length=20, null=True, blank=True
    )
  out_deviation = models.DecimalField(
    _('calculated standard deviation'), null=True, blank=True,
    max_digits=15, decimal_places=4
    )

  class Meta(AuditModel.Meta):
    db_table = 'forecast'
    verbose_name = _('forecast')
    verbose_name_plural = _('forecasts')
    ordering = ['name']

  def __str__(self):
    return self.name

  def setTotal(self, startdate, enddate, quantity):
    '''
    Update the forecast quantity in the forecast demand table.
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
    if not isinstance(quantity, Decimal):
      quantity = Decimal(str(quantity))
    # Round the quantity, if discrete flag is on
    if self.discrete:
      quantity = quantity.to_integral()
    # Step 0: Check for forecast entries intersecting with the current daterange
    startdate = startdate.date()
    enddate = enddate.date()
    entries = self.entries.filter(enddate__gt=startdate).filter(startdate__lt=enddate)
    if not entries:
      # Case 1: No intersecting forecast entries exist yet.
      # We just create an entry for the given start and end date
      # Note: if the calendar values are updated later on, such changes are
      # obviously not reflected any more in the forecast entries.
      self.entries.create(startdate=startdate, enddate=enddate, quantity=str(quantity)).save()
    else:
      # Case 2: Entries already exist in this daterange, which will be rescaled
      # Case 1, step 1: calculate current quantity and "clip" the existing entries
      # if required.
      current = 0
      for i in entries:
        # Calculate the length of this bucket in seconds
        duration = i.enddate - i.startdate
        duration = duration.days + 86400 * duration.seconds
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
          q = i.quantity * (p.days + 86400 * p.seconds) / duration
          if self.discrete:
            q = round(q)
          self.entries.create(
            startdate=enddate,
            enddate=i.enddate,
            quantity=str(q),
            ).save()
          # Part two: our date range, create a new entry
          self.entries.create(
            startdate=startdate,
            enddate=enddate,
            quantity=str(quantity),
            ).save()    # TODO Possible bug? Does this save to the correct database?
          # Part three: before our daterange, update the existing entry
          p = startdate - i.startdate
          i.enddate = startdate
          i.quantity = i.quantity * (p.days + 86400 * p.seconds) / duration
          if self.discrete:
            i.quantity = round(i.quantity)
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
          fraction = Decimal(i.quantity * (p.days + 86400 * p.seconds) / duration)
          current += i.quantity - fraction
          self.entries.create(
            startdate=i.startdate,
            enddate=enddate,
            quantity=str(i.quantity - fraction),
            ).save()
          i.startdate = enddate
          if self.discrete:
            i.quantity = str(round(fraction))
          else:
            i.quantity = str(fraction)
          i.save()
        elif i.enddate > startdate and i.startdate <= startdate:
          # This entry ends in the range and starts earlier.
          # Split the entry in two.
          p = startdate - i.startdate
          fraction = Decimal(i.quantity * (p.days + 86400 * p.seconds) / duration)
          current += i.quantity - fraction
          self.entries.create(
            startdate=startdate,
            enddate=i.enddate,
            quantity=str(i.quantity - fraction)
            ).save()
          i.enddate = startdate
          if self.discrete:
            i.quantity = str(round(fraction))
          else:
            i.quantity = str(fraction)
          i.save()
      # Case 1, step 2: Rescale the existing entries
      # Note that we retrieve an updated set of buckets from the database here...
      entries = self.entries.filter(enddate__gt=startdate).filter(startdate__lt=enddate)
      factor = quantity / current
      if factor == 0:
        for i in entries:
          i.delete()
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


  def updatePlan(self, startdate, enddate, fcstadj, ordersadj, units, database=DEFAULT_DB_ALIAS, forecastplan=None, save=True):
    '''
    Update adjustments in the forecastplan table.
    # TODO include rounding

    The *disaggregation* logic is as follows:
      - First find all lowest level "leaf" forecast plan records.
      - If the new value is 'none':
         - Case A:
           Erase all child overrides
      - Else if there are existing overriden values:
         - If the existing overridden quantities exceeds the new value, or if there are no unoverridden quantities:
           Case B:
           Scale the existing overrides and set all other values explicitly to 0.
           A corner case is the situation where are all existing overrides are 0.
         - Else:
           Case C:
           Scale non-overriden values to sum up correctly.
           Existing overridden values are left untouched.
           A corner case is when all non-overriden values are 0.
         - For order adjustment we always scale the existing overrides.
      - Else if existing total is greater than 0:
        Case D:
        Scale all existing records respecting their proportion of the total.
      - Else:
        Case E:
        Divide the quantity evenly over all existing candidates.

    This is followed by an *aggregation* step:
      - For every "leaf" forecast plan record that is changed:
         - Find its parent forecasts
         - Add the delta to the parent quantity
      - The parent values are cached in memory, to avoid that we update the
        same parent record many times in the database.
    '''
    # Assure the end date is later than the start date.
    if startdate > enddate:
      tmp = startdate
      startdate = enddate
      enddate = tmp

    # Round the quantity, if discrete flag is on and we edit units
    if self.discrete and units:
      if fcstadj:
        fcstadj = fcstadj.to_integral()
      if ordersadj:
        ordersadj = ordersadj.to_integral()

    if forecastplan:
      # Filter the lowest level forecast records involved passed as argument
      leafPlan = [ i for i in forecastplan if i.startdate.date() >= startdate and i.startdate.date() < enddate ]
    else:
      leafQuery = ForecastPlan.objects.select_for_update().using(database).filter(
        forecast__item__lft__gte = self.item.lft,
        forecast__item__lft__lt = self.item.rght,
        forecast__customer__lft__gte = self.customer.lft,
        forecast__customer__lft__lt = self.customer.rght,
        startdate__gte = startdate,
        startdate__lt = enddate,
        forecast__planned = True  # TODO need a more generic way to find the leaf forecasts. Only works for bottom up fcst
        )
      leafPlan = [ i for i in leafQuery ]
    if not leafPlan:
      raise Exception("No forecastplan entries found")

    # Analyze the current status of the leaf forecasts
    parents = {}
    curFcstAdj = None
    curWithoutFcstAdj = 0
    curFcstAdjValue = None
    curFcstTotal = 0
    curFcstTotalValue = 0
    curOrdersTotal = 0
    curOrdersAdj = None
    curWithoutOrdersAdj = 0
    curOrdersAdjValue = None
    curOrdersTotalValue = 0
    for i in leafPlan:
      curFcstTotal += i.forecasttotal
      curOrdersTotal += i.orderstotal
      curFcstTotalValue += i.forecasttotalvalue
      curOrdersTotalValue += i.orderstotalvalue
      if i.forecastadjustment is not None:
        if curFcstAdj is None:
          curFcstAdj = i.forecastadjustment
        else:
          curFcstAdj += i.forecastadjustment
      else:
        curWithoutFcstAdj += 1
      if i.ordersadjustment is not None:
        if curOrdersAdj is None:
          curOrdersAdj = i.ordersadjustment
        else:
          curOrdersAdj += i.ordersadjustment
      else:
        curWithoutOrdersAdj += i.orderstotal
      if i.forecastadjustment is not None:
        if curFcstAdjValue is None:
          curFcstAdjValue = i.forecastadjustmentvalue
        else:
          curFcstAdjValue += i.forecastadjustmentvalue
      if i.ordersadjustmentvalue is not None:
        if curOrdersAdjValue is None:
          curOrdersAdjValue = i.ordersadjustmentvalue
        else:
          curOrdersAdjValue += i.ordersadjustmentvalue
      # Identify parent records
      if save:
        parentQuery = ForecastPlan.objects.select_for_update().using(database).filter(
          forecast__item__lft__lte = i.forecast.item.lft,
          forecast__item__rght__gt = i.forecast.item.lft,
          forecast__customer__lft__lte = i.forecast.customer.lft,
          forecast__customer__rght__gt = i.forecast.customer.lft,
          startdate = i.startdate
          )
      else:
        parentQuery = ForecastPlan.objects.using(database).filter(
          forecast__item__lft__lte = i.forecast.item.lft,
          forecast__item__rght__gt = i.forecast.item.lft,
          forecast__customer__lft__lte = i.forecast.customer.lft,
          forecast__customer__rght__gt = i.forecast.customer.lft,
          startdate = i.startdate
          )
      i.parentkeys = {}
      for j in parentQuery:
        if j.forecast.name != i.forecast.name:
          i.parentkeys[(j.forecast.name, j.startdate)] = j
          parents[(j.forecast.name, j.startdate)] = j
          j.newForecastAdjustment = j.forecastadjustment is None
          j.newOrdersAdjustment = j.ordersadjustment is None

    # Handle forecast adjustments
    if fcstadj is None:
      # Case A: Reset all existing overrides to null
      for i in leafPlan:
        if i.forecastadjustment is not None:
          delta1 = i.forecastadjustment
          deltavalue1 = i.forecastadjustment * i.forecast.item.price
          delta2 = i.forecastadjustment - i.forecastbaseline
          deltavalue2 = (i.forecastadjustment - i.forecastbaseline) * i.forecast.item.price
          i.forecasttotal = i.forecastbaseline
          i.forecasttotalvalue = i.forecastbaseline * i.forecast.item.price
          i.forecastadjustment = None
          i.forecastadjustmentvalue = None
          for j in i.parentkeys:
            if parents[j].forecastadjustment is not None and parents[j].forecastadjustment > delta1:
              parents[j].forecastadjustment -= delta1
              parents[j].forecastadjustmentvalue -= deltavalue1
            else:
              parents[j].forecastadjustment = None
              parents[j].forecastadjustmentvalue = None
            parents[j].forecasttotal -= delta2
            parents[j].forecasttotalvalue -= deltavalue2
            if parents[j].forecasttotal < 0:
              parents[j].forecasttotal = parents[j].forecastbaseline
            if parents[j].forecasttotalvalue < 0:
              parents[j].forecasttotalvalue = parents[j].forecastbaselinevalue
    elif units:
      # Editing based on quantity
      if curFcstAdj is not None:
        if curFcstAdj >= fcstadj or curWithoutFcstAdj == 0 or abs(curFcstTotal - curFcstAdj) <= 0.001:
          # Case B: Scale the existing overrides and set all other values explicitly to 0
          if curFcstAdj:
            factor = fcstadj / curFcstAdj
          else:
            # Special case: all overrides are 0.
            factor = fcstadj / len(leafPlan)
          for i in leafPlan:
            if i.forecastadjustment is None:
              if abs(curFcstTotal - curFcstAdj) > 0.001:
                for j in i.parentkeys:
                  if parents[j].newForecastAdjustment:
                    if parents[j].forecastadjustment is None:
                      parents[j].forecastadjustment = Decimal(0.0)
                      parents[j].forecastadjustmentvalue = Decimal(0.0)
                i.forecastadjustment = i.forecastadjustmentvalue = i.forecasttotal = i.forecasttotalvalue = Decimal(0.0)
            else:
              if curFcstAdj:
                delta = (Decimal(1)-factor) * i.forecastadjustment
                i.forecastadjustment *= factor
                i.forecasttotal = i.forecastadjustment
              else:
                # Special case: all overrides are 0. Multiplying wouldn't work.
                delta = -factor
                i.forecastadjustment = i.forecastadjustment = factor
              i.forecastadjustmentvalue = i.forecasttotalvalue = i.forecastadjustment * i.forecast.item.price
              deltavalue = delta * i.forecast.item.price
              for j in i.parentkeys:
                parents[j].forecasttotal -= delta
                parents[j].forecasttotalvalue -= deltavalue
                if parents[j].newForecastAdjustment:
                  if parents[j].forecastadjustment is None:
                    parents[j].forecastadjustment = i.forecastadjustment
                    parents[j].forecastadjustmentvalue = i.forecastadjustmentvalue
                  else:
                    parents[j].forecastadjustment += i.forecastadjustment
                    parents[j].forecastadjustmentvalue += i.forecastadjustmentvalue
                else:
                  parents[j].forecastadjustment -= delta
                  parents[j].forecastadjustmentvalue -= deltavalue
        else:
          # Case C: Scale non-overriden values to sum up correctly
          factor = (fcstadj - curFcstAdj) / (curFcstTotal - curFcstAdj)
          for i in leafPlan:
            if i.forecastadjustment is None:
              delta = (Decimal(1)-factor) * i.forecasttotal
              i.forecastadjustment = i.forecasttotal = i.forecasttotal * factor
              deltavalue = delta * i.forecast.item.price
              i.forecastadjustmentvalue = i.forecasttotalvalue = i.forecastadjustment * i.forecast.item.price
              for j in i.parentkeys:
                parents[j].forecasttotal -= delta
                parents[j].forecasttotalvalue -= deltavalue
                if parents[j].newForecastAdjustment:
                  if parents[j].forecastadjustment is None:
                    parents[j].forecastadjustment = i.forecastadjustment
                    parents[j].forecastadjustmentvalue = i.forecastadjustmentvalue
                  else:
                    parents[j].forecastadjustment += i.forecastadjustment
                    parents[j].forecastadjustmentvalue += i.forecastadjustmentvalue
                else:
                  parents[j].forecastadjustment -= delta
                  parents[j].forecastadjustmentvalue -= deltavalue
      elif curFcstTotal > 0:
        # Case D: Scale all existing records respecting their proportion of the total
        factor = fcstadj / curFcstTotal
        for i in leafPlan:
          if i.forecastadjustment is None:
            delta = (Decimal(1)-factor) * i.forecasttotal
            deltavalue = delta * i.forecast.item.price
            i.forecastadjustment = i.forecasttotal * factor
            i.forecasttotal = i.forecastadjustment
            i.forecastadjustmentvalue = i.forecasttotalvalue = i.forecastadjustment * i.forecast.item.price
            for j in i.parentkeys:
              parents[j].forecasttotal -= delta
              parents[j].forecasttotalvalue -= deltavalue
              if parents[j].newForecastAdjustment:
                if parents[j].forecastadjustment is None:
                  parents[j].forecastadjustment = i.forecastadjustment
                  parents[j].forecastadjustmentvalue = i.forecastadjustment * i.forecast.item.price
                else:
                  parents[j].forecastadjustment += i.forecastadjustment
                  parents[j].forecastadjustmentvalue += i.forecastadjustment * i.forecast.item.price
              else:
                parents[j].forecastadjustment -= delta
                parents[j].forecastadjustmentvalue -= deltavalue
      else:
        # Case E: Divide the quantity evenly over all existing candidates
        factor = fcstadj / len(leafPlan)
        for i in leafPlan:
          delta = i.forecasttotal - factor
          deltavalue = delta * i.forecast.item.price
          i.forecastadjustment = i.forecasttotal = factor
          i.forecastadjustmentvalue = i.forecasttotalvalue = i.forecastadjustment * i.forecast.item.price
          for j in i.parentkeys:
            parents[j].forecasttotal -= delta
            parents[j].forecasttotalvalue -= deltavalue
            if parents[j].newForecastAdjustment:
              if parents[j].forecastadjustment is None:
                parents[j].forecastadjustment = i.forecastadjustment
                parents[j].forecastadjustmentvalue = i.forecastadjustmentvalue
              else:
                parents[j].forecastadjustment += i.forecastadjustment
                parents[j].forecastadjustmentvalue += i.forecastadjustmentvalue
            else:
              parents[j].forecastadjustment -= delta
              parents[j].forecastadjustmentvalue -= delta
    else:
      # Editing based on value
      if curFcstAdjValue is not None:
        if curFcstAdjValue >= fcstadj or curWithoutFcstAdj == 0 or abs(curFcstTotalValue - curFcstAdjValue) <= 0.001:
          # Case B: Scale the existing overrides and set all other values explicitly to 0
          if curFcstAdjValue:
            factor = fcstadj / curFcstAdjValue
          else:
            # Special case: all overrides are 0.
            factor = fcstadj / len(leafPlan)
          for i in leafPlan:
            if i.forecastadjustmentvalue is None:
              if abs(curFcstTotalValue - curFcstAdjValue) > 0.001:
                for j in i.parentkeys:
                  if parents[j].newForecastAdjustment:
                    if parents[j].forecastadjustment is None:
                      parents[j].forecastadjustment = Decimal(0.0)
                      parents[j].forecastadjustmentvalue = Decimal(0.0)
                i.forecastadjustment = i.forecastadjustmentvalue = i.forecasttotal = i.forecasttotalvalue = Decimal(0.0)
            elif i.forecast.item.price:
              if curFcstAdjValue:
                deltavalue = (Decimal(1)-factor) * i.forecastadjustmentvalue
                i.forecastadjustmentvalue *= factor
                i.forecasttotalvalue = i.forecastadjustmentvalue
              else:
                # Special case: all overrides are 0. Multiplying wouldn't work.
                deltavalue = -factor
                i.forecastadjustmentvalue = i.forecastadjustmentvalue = factor
              i.forecastadjustment = i.forecasttotal = i.forecastadjustmentvalue / i.forecast.item.price
              delta = deltavalue / i.forecast.item.price
              for j in i.parentkeys:
                parents[j].forecasttotal -= delta
                parents[j].forecasttotalvalue -= deltavalue
                if parents[j].newForecastAdjustment:
                  if parents[j].forecastadjustment is None:
                    parents[j].forecastadjustment = i.forecastadjustment
                    parents[j].forecastadjustmentvalue = i.forecastadjustmentvalue
                  else:
                    parents[j].forecastadjustment += i.forecastadjustment
                    parents[j].forecastadjustmentvalue += i.forecastadjustmentvalue
                else:
                  parents[j].forecastadjustment -= delta
                  parents[j].forecastadjustmentvalue -= deltavalue
        else:
          # Case C: Scale non-overriden values to sum up correctly
          factor = (fcstadj - curFcstAdjValue) / (curFcstTotalValue - curFcstAdjValue)
          for i in leafPlan:
            if i.forecastadjustmentvalue is None and i.forecast.item.price:
              deltavalue = (Decimal(1)-factor) * i.forecasttotalvalue
              i.forecastadjustmentvalue = i.forecasttotalvalue = i.forecasttotalvalue * factor
              delta = deltavalue / i.forecast.item.price
              i.forecastadjustment = i.forecasttotal = i.forecastadjustmentvalue / i.forecast.item.price
              for j in i.parentkeys:
                parents[j].forecasttotal -= delta
                parents[j].forecasttotalvalue -= deltavalue
                if parents[j].newForecastAdjustment:
                  if parents[j].forecastadjustment is None:
                    parents[j].forecastadjustment = i.forecastadjustment
                    parents[j].forecastadjustmentvalue = i.forecastadjustmentvalue
                  else:
                    parents[j].forecastadjustment += i.forecastadjustment
                    parents[j].forecastadjustmentvalue += i.forecastadjustmentvalue
                else:
                  parents[j].forecastadjustment -= delta
                  parents[j].forecastadjustmentvalue -= deltavalue
      elif curFcstTotal > 0:
        # Case D: Scale all existing records respecting their proportion of the total
        factor = fcstadj / curFcstTotalValue
        for i in leafPlan:
          if i.forecastadjustmentvalue is None and i.forecast.item.price:
            deltavalue = (Decimal(1)-factor) * i.forecasttotalvalue
            delta = deltavalue / i.forecast.item.price
            i.forecastadjustmentvalue = i.forecasttotalvalue * factor
            i.forecasttotalvalue = i.forecastadjustmentvalue
            i.forecastadjustment = i.forecasttotal = i.forecastadjustmentvalue / i.forecast.item.price
            for j in i.parentkeys:
              parents[j].forecasttotal -= delta
              parents[j].forecasttotalvalue -= deltavalue
              if parents[j].newForecastAdjustment:
                if parents[j].forecastadjustment is None:
                  parents[j].forecastadjustment = i.forecastadjustment
                  parents[j].forecastadjustmentvalue = i.forecastadjustment * i.forecast.item.price
                else:
                  parents[j].forecastadjustment += i.forecastadjustment
                  parents[j].forecastadjustmentvalue += i.forecastadjustment * i.forecast.item.price
              else:
                parents[j].forecastadjustment -= delta
                parents[j].forecastadjustmentvalue -= deltavalue
      else:
        # Case E: Divide the quantity evenly over all existing candidates
        factor = fcstadj / len(leafPlan)
        for i in leafPlan:
          if not i.forecast.item.price:
            continue
          deltavalue = i.forecasttotalvalue - factor
          delta = deltavalue / i.forecast.item.price
          i.forecastadjustmentvalue = i.forecasttotalvalue = factor
          i.forecastadjustment = i.forecasttotal = i.forecastadjustmentvalue / i.forecast.item.price
          for j in i.parentkeys:
            parents[j].forecasttotal -= delta
            parents[j].forecasttotalvalue -= deltavalue
            if parents[j].newForecastAdjustment:
              if parents[j].forecastadjustment is None:
                parents[j].forecastadjustment = i.forecastadjustment
                parents[j].forecastadjustmentvalue = i.forecastadjustmentvalue
              else:
                parents[j].forecastadjustment += i.forecastadjustment
                parents[j].forecastadjustmentvalue += i.forecastadjustmentvalue
            else:
              parents[j].forecastadjustment -= delta
              parents[j].forecastadjustmentvalue -= delta

    # Handle order adjustments
    # The logic is identical to the forecast updates, except that we don't
    # update the total.
    if ordersadj is None:
      # Case A: Reset all existing overrides to null
      for i in leafPlan:
        if i.ordersadjustment is not None:
          delta = i.ordersadjustment
          deltavalue = i.ordersadjustment * i.forecast.item.price
          i.ordersadjustment = None
          i.ordersadjustmentvalue = None
          for j in i.parentkeys:
            if parents[j].ordersadjustment is not None and parents[j].ordersadjustment > delta:
              parents[j].ordersadjustment -= delta
              parents[j].ordersadjustmentvalue -= deltavalue
            else:
              parents[j].ordersadjustment = None
              parents[j].ordersadjustmentvalue = None
    elif units:
      # Editing based on quantity
      if curOrdersAdj is not None:
        if curOrdersAdj >= ordersadj or curWithoutOrdersAdj == 0 or abs(curOrdersTotal - curOrdersAdj) <= 0.001:
          # Case B: Scale the existing overrides and set all other values explicitly to 0
          if curOrdersAdj:
            factor = ordersadj / curOrdersAdj
          else:
            # Special case: all overrides are 0.
            factor = ordersadj / len(leafPlan)
          for i in leafPlan:
            if i.ordersadjustment is None:
              if abs(curOrdersTotal - curOrdersAdj) <= 0.001:
                for j in i.parentkeys:
                  if parents[j].newOrdersAdjustment:
                    if parents[j].ordersadjustment is None:
                      parents[j].ordersadjustment = Decimal(0.0)
                      parents[j].ordersadjustmentvalue = Decimal(0.0)
                i.ordersadjustment = i.ordersadjustmentvalue = Decimal(0.0)
            else:
              if curOrdersAdj:
                delta = (Decimal(1)-factor) * i.ordersadjustment
                i.ordersadjustment *= factor
              else:
                # Special case: all overrides are 0. Multiplying wouldn't work.
                delta = -factor
                i.ordersadjustment = factor
              i.ordersadjustmentvalue = i.ordersadjustment * i.forecast.item.price
              deltavalue = delta * i.forecast.item.price
              for j in i.parentkeys:
                if parents[j].newOrdersAdjustment:
                  if parents[j].ordersadjustment is None:
                    parents[j].ordersadjustment = i.ordersadjustment
                    parents[j].ordersadjustmentvalue = i.ordersadjustmentvalue
                  else:
                    parents[j].ordersadjustment += i.ordersadjustment
                    parents[j].ordersadjustmentvalue += i.ordersadjustmentvalue
                else:
                  parents[j].ordersadjustment -= delta
                  parents[j].ordersadjustmentvalue -= deltavalue
        else:
          # Case C: Scale non-overriden values to sum up correctly
          factor = (ordersadj - curOrdersAdj) / curWithoutOrdersAdj
          for i in leafPlan:
            if i.ordersadjustment is None:
              delta = (Decimal(1)-factor) * i.orderstotal
              i.ordersadjustment = i.orderstotal * factor
              deltavalue = delta * i.forecast.item.price
              i.ordersadjustmentvalue = i.ordersadjustment * i.forecast.item.price
              for j in i.parentkeys:
                if parents[j].newOrdersAdjustment:
                  if parents[j].ordersadjustment is None:
                    parents[j].ordersadjustment = i.ordersadjustment
                    parents[j].ordersadjustmentvalue = i.ordersadjustmentvalue
                  else:
                    parents[j].ordersadjustment += i.ordersadjustment
                    parents[j].ordersadjustmentvalue += i.ordersadjustmentvalue
                else:
                  parents[j].ordersadjustment -= delta
                  parents[j].ordersadjustmentvalue -= deltavalue
      elif curOrdersTotal > 0:
        # Case D: Scale all existing records respecting their proportion of the total
        factor = ordersadj / curOrdersTotal
        for i in leafPlan:
          if i.ordersadjustment is None:
            delta = (Decimal(1)-factor) * i.orderstotal
            deltavalue = delta * i.forecast.item.price
            i.ordersadjustment = i.orderstotal * factor
            i.ordersadjustmentvalue = i.ordersadjustment * i.forecast.item.price
            for j in i.parentkeys:
              if parents[j].newOrdersAdjustment:
                if parents[j].ordersadjustment is None:
                  parents[j].ordersadjustment = i.ordersadjustment
                  parents[j].ordersadjustmentvalue = i.ordersadjustment * i.forecast.item.price
                else:
                  parents[j].ordersadjustment += i.ordersadjustment
                  parents[j].ordersadjustmentvalue += i.ordersadjustment * i.forecast.item.price
              else:
                parents[j].ordersadjustment -= delta
                parents[j].ordersadjustmentvalue -= deltavalue
      else:
        # Case E: Divide the quantity evenly over all existing candidates
        factor = ordersadj / len(leafPlan)
        for i in leafPlan:
          delta = i.orderstotal - factor
          deltavalue = delta * i.forecast.item.price
          i.ordersadjustment = factor
          i.ordersadjustmentvalue = i.ordersadjustment * i.forecast.item.price
          for j in i.parentkeys:
            if parents[j].newOrdersAdjustment:
              if parents[j].ordersadjustment is None:
                parents[j].ordersadjustment = i.ordersadjustment
                parents[j].ordersadjustmentvalue = i.ordersadjustmentvalue
              else:
                parents[j].ordersadjustment += i.ordersadjustment
                parents[j].ordersadjustmentvalue += i.ordersadjustmentvalue
            else:
              parents[j].ordersadjustment -= delta
              parents[j].ordersadjustmentvalue -= delta
    else:
      # Editing based on value
      if curOrdersAdjValue is not None:
        if curOrdersAdjValue >= ordersadj or curWithoutOrdersAdj == 0 or abs(curOrdersTotalValue - curOrdersAdjValue) <= 0.001:
          # Case B: Scale the existing overrides and set all other values explicitly to 0
          if curOrdersAdjValue:
            factor = ordersadj / curOrdersAdjValue
          else:
            # Special case: all overrides are 0.
            factor = ordersadj / len(leafPlan)
          for i in leafPlan:
            if i.ordersadjustmentvalue is None:
              if abs(curOrdersTotalValue - curOrdersAdjValue) > 0.001:
                for j in i.parentkeys:
                  if parents[j].newOrdersAdjustment:
                    if parents[j].ordersadjustment is None:
                      parents[j].ordersadjustment = Decimal(0.0)
                      parents[j].ordersadjustmentvalue = Decimal(0.0)
                i.ordersadjustment = i.ordersadjustmentvalue = Decimal(0.0)
            elif i.forecast.item.price:
              if curOrdersAdj:
                deltavalue = (Decimal(1)-factor) * i.ordersadjustmentvalue
                i.ordersadjustmentvalue *= factor
              else:
                # Special case: all overrides are 0. Multiplying wouldn't work.
                deltavalue = -factor
                i.ordersadjustmentvalue = factor
              i.ordersadjustment = i.ordersadjustmentvalue / i.forecast.item.price
              delta = deltavalue / i.forecast.item.price
              for j in i.parentkeys:
                if parents[j].newOrdersAdjustment:
                  if parents[j].ordersadjustment is None:
                    parents[j].ordersadjustment = i.ordersadjustment
                    parents[j].ordersadjustmentvalue = i.ordersadjustmentvalue
                  else:
                    parents[j].ordersadjustment += i.ordersadjustment
                    parents[j].ordersadjustmentvalue += i.ordersadjustmentvalue
                else:
                  parents[j].ordersadjustment -= delta
                  parents[j].ordersadjustmentvalue -= deltavalue
        else:
          # Case C: Scale non-overriden values to sum up correctly
          factor = (ordersadj - curOrdersAdjValue) / (curOrdersTotalValue - curOrdersAdjValue)
          for i in leafPlan:
            if i.ordersadjustmentvalue is None and i.forecast.item.price:
              deltavalue = (Decimal(1)-factor) * i.orderstotalvalue
              i.ordersadjustmentvalue = i.orderstotalvalue * factor
              delta = deltavalue / i.forecast.item.price
              i.ordersadjustment = i.ordersadjustmentvalue / i.forecast.item.price
              for j in i.parentkeys:
                if parents[j].newOrdersAdjustment:
                  if parents[j].ordersadjustment is None:
                    parents[j].ordersadjustment = i.ordersadjustment
                    parents[j].ordersadjustmentvalue = i.ordersadjustmentvalue
                  else:
                    parents[j].ordersadjustment += i.ordersadjustment
                    parents[j].ordersadjustmentvalue += i.ordersadjustmentvalue
                else:
                  parents[j].ordersadjustment -= delta
                  parents[j].ordersadjustmentvalue -= deltavalue
      elif curOrdersTotal > 0:
        # Case D: Scale all existing records respecting their proportion of the total
        factor = ordersadj / curOrdersTotalValue
        for i in leafPlan:
          if i.ordersadjustmentvalue is None and i.forecast.item.price:
            deltavalue = (Decimal(1)-factor) * i.orderstotalvalue
            delta = deltavalue / i.forecast.item.price
            i.ordersadjustmentvalue = i.orderstotalvalue * factor
            i.ordersadjustment = i.ordersadjustmentvalue / i.forecast.item.price
            for j in i.parentkeys:
              if parents[j].newOrdersAdjustment:
                if parents[j].ordersadjustment is None:
                  parents[j].ordersadjustment = i.ordersadjustment
                  parents[j].ordersadjustmentvalue = i.ordersadjustment * i.forecast.item.price
                else:
                  parents[j].ordersadjustment += i.ordersadjustment
                  parents[j].ordersadjustmentvalue += i.ordersadjustment * i.forecast.item.price
              else:
                parents[j].ordersadjustment -= delta
                parents[j].ordersadjustmentvalue -= deltavalue
      else:
        # Case E: Divide the quantity evenly over all existing candidates
        factor = ordersadj / len(leafPlan)
        for i in leafPlan:
          if not i.forecast.item.price:
            continue
          deltavalue = i.orderstotalvalue - factor
          delta = deltavalue / i.forecast.item.price
          i.ordersadjustmentvalue = factor
          i.ordersadjustment = i.ordersadjustmentvalue / i.forecast.item.price
          for j in i.parentkeys:
            if parents[j].newOrdersAdjustment:
              if parents[j].ordersadjustment is None:
                parents[j].ordersadjustment = i.ordersadjustment
                parents[j].ordersadjustmentvalue = i.ordersadjustmentvalue
              else:
                parents[j].ordersadjustment += i.ordersadjustment
                parents[j].ordersadjustmentvalue += i.ordersadjustmentvalue
            else:
              parents[j].ordersadjustment -= delta
              parents[j].ordersadjustmentvalue -= delta

    # Save the results in the database
    if save:
      for i in leafPlan:
        i.save(update_fields=[
          'forecastadjustment', 'ordersadjustment', 'forecasttotal',
          'forecastadjustmentvalue', 'ordersadjustmentvalue', 'forecasttotalvalue'
          ])
      for i in parents.values():
        i.save(update_fields=[
          'forecastadjustment', 'ordersadjustment', 'forecasttotal',
          'forecastadjustmentvalue', 'ordersadjustmentvalue', 'forecasttotalvalue'
          ])


class ForecastDemand(AuditModel):
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True)
  forecast = models.ForeignKey(Forecast, verbose_name=_('forecast'), null=False, db_index=True, related_name='entries')
  startdate = models.DateField(_('start date'), null=False)
  enddate = models.DateField(_('end date'), null=False)
  quantity = models.DecimalField(_('quantity'), max_digits=15, decimal_places=4, default=0)

  def __str__(self):
    return self.forecast.name + " " + str(self.startdate) + " - " + str(self.enddate)

  class Meta(AuditModel.Meta):
    db_table = 'forecastdemand'
    verbose_name = _('forecast demand')
    verbose_name_plural = _('forecast demands')


class ForecastPlan(models.Model):
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True)
  forecast = models.ForeignKey(Forecast, verbose_name=_('forecast'), db_index=True, related_name='plans')
  startdate = models.DateTimeField(_('start date'), null=False, db_index=True)
  enddate = models.DateTimeField(_('end date'), null=False, db_index=True)
  orderstotal = models.DecimalField(_('total orders'), max_digits=15, decimal_places=4, default='0.00')
  ordersadjustment = models.DecimalField(_('orders adjustment'), max_digits=15, decimal_places=4, null=True, blank=True)
  ordersopen = models.DecimalField(_('open orders'), max_digits=15, decimal_places=4, default='0.00')
  ordersplanned = models.DecimalField(_('planned orders'), max_digits=15, decimal_places=4, default='0.00')
  forecastbaseline = models.DecimalField(_('forecast baseline'), max_digits=15, decimal_places=4, default='0.00')
  forecastadjustment = models.DecimalField(_('forecast adjustment'), max_digits=15, decimal_places=4, null=True, blank=True)
  forecasttotal = models.DecimalField(_('forecast total'), max_digits=15, decimal_places=4, default='0.00')
  forecastnet = models.DecimalField(_('forecast net'), max_digits=15, decimal_places=4, default='0.00')
  forecastconsumed = models.DecimalField(_('forecast consumed'), max_digits=15, decimal_places=4, default='0.00')
  forecastplanned = models.DecimalField(_('planned forecast'), max_digits=15, decimal_places=4, default='0.00')
  orderstotalvalue = models.DecimalField(_('total orders'), max_digits=15, decimal_places=4, default='0.00')
  ordersadjustmentvalue = models.DecimalField(_('orders adjustment'), max_digits=15, decimal_places=4, null=True, blank=True)
  ordersopenvalue = models.DecimalField(_('open orders'), max_digits=15, decimal_places=4, default='0.00')
  ordersplannedvalue = models.DecimalField(_('planned orders'), max_digits=15, decimal_places=4, default='0.00')
  forecastbaselinevalue = models.DecimalField(_('forecast baseline'), max_digits=15, decimal_places=4, default='0.00')
  forecastadjustmentvalue = models.DecimalField(_('forecast adjustment'), max_digits=15, decimal_places=4, null=True, blank=True)
  forecasttotalvalue = models.DecimalField(_('forecast total'), max_digits=15, decimal_places=4, default='0.00')
  forecastnetvalue = models.DecimalField(_('forecast net'), max_digits=15, decimal_places=4, default='0.00')
  forecastconsumedvalue = models.DecimalField(_('forecast consumed'), max_digits=15, decimal_places=4, default='0.00')
  forecastplannedvalue = models.DecimalField(_('planned forecast'), max_digits=15, decimal_places=4, default='0.00')

  def __str__(self):
    return "%s - %s" % (self.forecast.name, str(self.startdate))

  class Meta:
    db_table = 'forecastplan'
    index_together = [ ["forecast", "startdate"], ]
    ordering = ['id']
    verbose_name = _('forecast plan')
    verbose_name_plural = _('forecast plans')
