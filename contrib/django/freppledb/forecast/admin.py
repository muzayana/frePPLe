#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.  
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code 
# or in the form of compiled binaries.
#

# file : $URL: file:///C:/Users/Johan/Dropbox/SVNrepository/frepple/addon/contrib/django/freppledb_extra/admin.py $
# revision : $LastChangedRevision: 449 $  $LastChangedBy: Johan $
# date : $LastChangedDate: 2012-12-28 18:59:56 +0100 (Fri, 28 Dec 2012) $

from django.contrib import admin
from django.utils.translation import ugettext_lazy as _

from freppledb.forecast.models import Forecast, ForecastDemand
from freppledb.admin import data_site
from freppledb.common import MultiDBModelAdmin, MultiDBTabularInline


class ForecastDemand_inline(MultiDBTabularInline):
  model = ForecastDemand
  extra = 5


class Forecast_admin(MultiDBModelAdmin):
  model = Forecast
  raw_id_fields = ('customer', 'item', 'calendar', 'operation')
  fieldsets = (
            (None, {'fields': ('name', 'item', 'customer', 'calendar', 'description', 'category','subcategory', 'priority')}),
            (_('Planning parameters'), {'fields': ('discrete', 'operation', 'minshipment', 'maxlateness'), 'classes': ('collapse')}),
        )
  radio_fields = {'priority': admin.HORIZONTAL, }
  inlines = [ ForecastDemand_inline, ]
  save_on_top = True
data_site.register(Forecast,Forecast_admin)
