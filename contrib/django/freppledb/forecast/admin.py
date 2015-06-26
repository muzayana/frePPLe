#
# Copyright (C) 2012-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _

from freppledb.forecast.models import Forecast, ForecastDemand
from freppledb.admin import data_site
from freppledb.common.adminforms import MultiDBModelAdmin, MultiDBTabularInline


class ForecastDemand_admin(MultiDBModelAdmin):
  model = ForecastDemand
  raw_id_fields = ('forecast',)
  save_on_top = True
  exclude = ('source',)
data_site.register(ForecastDemand, ForecastDemand_admin)


class ForecastDemand_inline(MultiDBTabularInline):
  model = ForecastDemand
  extra = 5
  exclude = ('source',)


class Forecast_admin(MultiDBModelAdmin):
  model = Forecast
  raw_id_fields = ('customer', 'item', 'calendar', 'operation')
  fieldsets = (
    (None, {'fields': ('name', 'item', 'customer', 'calendar', 'method', 'description', 'category', 'subcategory', 'priority')}),
    (_('Planning parameters'), {'fields': ('discrete', 'planned', 'operation', 'minshipment', 'maxlateness'), 'classes': ('collapse')}),
    )
  save_on_top = True
data_site.register(Forecast, Forecast_admin)
