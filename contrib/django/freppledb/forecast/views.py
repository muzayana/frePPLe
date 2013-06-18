#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime, date
import json

from django.db import connections, transaction
from django.utils.translation import ugettext_lazy as _
from django.utils.text import capfirst
from django.utils.encoding import force_unicode
from django.http import HttpResponse, HttpResponseForbidden

from freppledb.forecast.models import Forecast, ForecastDemand
from freppledb.common.db import python_date
from freppledb.common.report import GridPivot, GridFieldText, GridFieldInteger, GridFieldDate
from freppledb.common.report import GridReport, GridFieldBool, GridFieldLastModified
from freppledb.common.report import GridFieldChoice, GridFieldNumber


class ForecastList(GridReport):
  '''
  A list report to show forecasts.
  '''
  template = 'forecast/forecastlist.html'
  title = _("Forecast List")
  basequeryset = Forecast.objects.all()
  model = Forecast
  frozenColumns = 1

  rows = (
    GridFieldText('name', title=_('name'), key=True, formatter='forecast'),
    GridFieldText('item', title=_('item'), field_name='item__name', formatter='item'),
    GridFieldText('customer', title=_('customer'), field_name='customer__name', formatter='customer'),
    GridFieldText('calendar', title=_('calendar'), field_name='calendar__name', formatter='calendar'),
    GridFieldText('description', title=_('description')),
    GridFieldText('category', title=_('category')),
    GridFieldText('subcategory', title=_('subcategory')),
    GridFieldChoice('method', title=_('method'), choices=Forecast.methods),
    GridFieldText('operation', title=_('operation'), field_name='operation__name', formatter='operation'),
    GridFieldInteger('priority', title=_('priority')),
    GridFieldNumber('maxlateness', title=_('maximum lateness')),
    GridFieldNumber('minshipment', title=_('minimum shipment')),
    GridFieldBool('discrete', title=_('discrete')),
    GridFieldLastModified('lastmodified'),
    )


class ForecastDemandList(GridReport):
  '''
  A list report to show forecastdemands.
  '''
  template = 'forecast/forecastdemandlist.html'
  title = _("Forecast demand List")
  basequeryset = ForecastDemand.objects.all()
  model = ForecastDemand
  frozenColumns = 1

  rows = (
    GridFieldInteger('identifier', title=_('identifier'), key=True),
    GridFieldText('forecast', title=_('forecast'), formatter='forecast'),
    GridFieldDate('startdate', title=_('start date')),
    GridFieldDate('enddate', title=_('end date')),
    GridFieldNumber('quantity', title=_('quantity')),
    GridFieldLastModified('lastmodified'),
    )


class OverviewReport(GridPivot):
  '''
  A report allowing easy editing of forecast numbers.
  '''
  template = 'forecast/forecast.html'
  title = _('Forecast Report')
  basequeryset = Forecast.objects.all()
  model = Forecast
  editable = True
  rows = (
    GridFieldText('forecast', title=_('forecast'), key=True, field_name='name', formatter='forecast', editable=False),
    GridFieldText('item', title=_('item'), key=True, field_name='item__name', formatter='item', editable=False),
    GridFieldText('customer', title=_('customer'), key=True, field_name='customer__name', formatter='customer', editable=False),
    GridFieldText(None, width="(5*numbuckets<200 ? 5*numbuckets : 200)", extra='formatter:graph', editable=False),
    )
  crosses = (
    ('orderstotal',{'title': _('total orders'), 'editable': lambda req: req.user.has_perm('input.change_forecastdemand'),}),
    ('ordersopen',{'title': _('open orders')}),
    ('forecastbaseline',{'title': _('forecast baseline')}),
    ('forecastadjustment',{'title': _('forecast adjustment')}),
    ('forecasttotal',{'title': _('forecast total')}),
    ('forecastnet',{'title': _('forecast net')}),
    ('forecastconsumed',{'title': _('forecast consumed')}),
    ('planned',{'title': _('planned net forecast')}),
    ('past',{'visible':False}),
    )

  @classmethod
  def parseJSONupload(reportclass, request):
    # Check permissions
    if not request.user.has_perm('forecast.change_forecastdemand'):
      return HttpResponseForbidden(_('Permission denied'))

    # Loop over the data records
    transaction.enter_transaction_management(using=request.database)
    transaction.managed(True, using=request.database)
    resp = HttpResponse()
    ok = True
    try:
      for rec in json.JSONDecoder().decode(request.read()):
        try:
          # Find the forecast
          start = datetime.strptime(rec['startdate'],'%Y-%m-%d')
          end = datetime.strptime(rec['enddate'],'%Y-%m-%d')
          fcst = Forecast.objects.using(request.database).get(name = rec['id'])
          # Update the forecast
          fcst.setTotal(start,end,rec['value'])
        except Exception as e:
          ok = False
          resp.write(e)
          resp.write('<br/>')
    finally:
      transaction.commit(using=request.database)
      transaction.leave_transaction_management(using=request.database)
    if ok: resp.write("OK")
    resp.status_code = ok and 200 or 403
    return resp

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      return {
        'title': capfirst(force_unicode(Forecast._meta.verbose_name) + " " + args[0]),
        'post_title': ': ' + capfirst(force_unicode(_('plan'))),
        }
    else:
      return {}

  @staticmethod
  def query(request, basequery, bucket, startdate, enddate, sortsql='1 asc'):
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=True)
    # Execute the query
    cursor = connections[request.database].cursor()

    try:
      cursor.execute("SELECT value FROM common_parameter where name='currentdate'")
      d = cursor.fetchone()
      currentdate = datetime.strptime(d[0], "%Y-%m-%d %H:%M:%S").date()
    except:
      currentdate = date.now()

    query = '''
        select y.name as row1, y.item_id as row2, y.customer_id as row3,
               y.bucket as col1, y.startdate as col2, y.enddate as col3,
               min(y.orderstotal),
               min(y.ordersopen),
               min(y.forecastbaseline),
               min(y.forecastadjustment),
               min(y.forecasttotal),
               min(y.forecastnet),
               min(y.forecastconsumed),
               coalesce(sum(out_demand.planquantity),0)
        from (
          select fcst.name as name, fcst.item_id as item_id, fcst.customer_id as customer_id,
             d.bucket as bucket, d.startdate as startdate, d.enddate as enddate,
             coalesce(sum(forecastplan.orderstotal),0) as orderstotal,
             coalesce(sum(forecastplan.ordersopen),0) as ordersopen,
             coalesce(sum(forecastplan.forecastbaseline),0) as forecastbaseline,
             coalesce(sum(forecastplan.forecastadjustment),0) as forecastadjustment,
             coalesce(sum(forecastplan.forecasttotal),0) as forecasttotal,
             coalesce(sum(forecastplan.forecastnet),0) as forecastnet,
             coalesce(sum(forecastplan.forecastconsumed),0) as forecastconsumed
          from (%s) fcst
          -- Multiply with buckets
          cross join (
             select name as bucket, startdate, enddate
             from common_bucketdetail
             where bucket_id = '%s' and enddate > '%s' and startdate < '%s'
             ) d
          -- Forecast plan
          left join forecastplan
          on fcst.name = forecastplan.forecast_id
          and forecastplan.startdate >= d.startdate
          and forecastplan.startdate < d.enddate
          -- Grouping
          group by fcst.name, fcst.item_id, fcst.customer_id,
                 d.bucket, d.startdate, d.enddate
        ) y
        -- Planned quantity
        left join out_demand
        on out_demand.demand like y.name || ' - %%%%'
        and y.startdate <= out_demand.plandate
        and y.enddate > out_demand.plandate
        and out_demand.plandate >= '%s'
        and out_demand.plandate < '%s'
        -- Ordering and grouping
        group by y.name, y.item_id, y.customer_id,
           y.bucket, y.startdate, y.enddate
        order by %s, y.startdate
        ''' % (basesql,bucket,startdate,enddate,startdate,enddate,sortsql)
    cursor.execute(query, baseparams)

    # Build the python result
    for row in cursor.fetchall():
      yield {
        'forecast': row[0],
        'item': row[1],
        'customer': row[2],
        'bucket': row[3],
        'startdate': python_date(row[4]),
        'enddate': python_date(row[5]),
        'past': python_date(row[4]) < currentdate and 1 or 0,
        'orderstotal': row[6],
        'ordersopen': row[7],
        'forecastbaseline': row[8],
        'forecastadjustment': row[9],
        'forecasttotal': row[10],
        'forecastnet': row[11],
        'forecastconsumed': row[12],
        'planned': row[13],
        }
