#
# Copyright (C) 2012-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime
from decimal import Decimal
import json

from django.db import connections, transaction
from django.utils.translation import ugettext_lazy as _
from django.utils.text import capfirst
from django.utils.encoding import force_text
from django.http import HttpResponse, HttpResponseForbidden, Http404

from freppledb.forecast.models import Forecast, ForecastDemand, ForecastPlan
from freppledb.common.db import python_date
from freppledb.common.report import GridPivot, GridFieldText, GridFieldInteger, GridFieldDate
from freppledb.common.report import GridReport, GridFieldBool, GridFieldLastModified
from freppledb.common.report import GridFieldChoice, GridFieldNumber
from freppledb.input.views import PathReport


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
    GridFieldText('source', title=_('source')),
    GridFieldLastModified('lastmodified'),
    )


class ForecastDemandList(GridReport):
  '''
  A list report to show forecastdemands.
  '''
  template = 'forecast/forecastdemandlist.html'
  title = _("Forecasted Demand List")
  basequeryset = ForecastDemand.objects.all()
  model = ForecastDemand
  frozenColumns = 1

  rows = (
    GridFieldInteger('id', title=_('identifier'), key=True),
    GridFieldText('forecast', title=_('forecast'), formatter='forecast'),
    GridFieldDate('startdate', title=_('start date')),
    GridFieldDate('enddate', title=_('end date')),
    GridFieldNumber('quantity', title=_('quantity')),
    GridFieldText('source', title=_('source')),
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
  permissions = (('view_forecast_report', 'Can view forecast report'),)
  editable = True
  rows = (
    GridFieldText('forecast', title=_('forecast'), key=True, field_name='name', formatter='forecast', editable=False),
    GridFieldText('item', title=_('item'), field_name='item__name', formatter='item', editable=False),
    GridFieldText('customer', title=_('customer'), field_name='customer__name', formatter='customer', editable=False)
    )
  crosses = (
    ('orderstotal', {'title': _('total orders')}),
    ('ordersopen', {'title': _('open orders')}),
    ('ordersadjustment', {'title': _('orders adjustment'), 'editable': lambda req: req.user.has_perm('input.change_forecastdemand')}),
    ('forecastbaseline', {'title': _('forecast baseline')}),
    ('forecastadjustment', {'title': _('forecast adjustment')}),
    ('forecasttotal', {'title': _('forecast total')}),
    ('forecastnet', {'title': _('forecast net')}),
    ('forecastconsumed', {'title': _('forecast consumed')}),
    ('ordersplanned', {'title': _('planned orders')}),
    ('forecastplanned', {'title': _('planned net forecast')}),
    ('past', {'visible': False}),
    ('future', {'visible': False}),
    )

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      return {
        'title': capfirst(force_text(Forecast._meta.verbose_name) + " " + args[0]),
        'post_title': ': ' + capfirst(force_text(_('plan'))),
        }
    else:
      return {}

  @staticmethod
  def query(request, basequery, sortsql='1 asc'):
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=True)
    # Execute the query
    cursor = connections[request.database].cursor()

    try:
      cursor.execute("SELECT value FROM common_parameter where name='currentdate'")
      d = cursor.fetchone()
      currentdate = datetime.strptime(d[0], "%Y-%m-%d %H:%M:%S").date()
    except:
      currentdate = datetime.now().date()

    # Value or units
    prefs = request.user.getPreference(OverviewReport.getKey())
    if prefs and prefs.get('units', 'unit') == 'value':
      suffix = 'value'
    else:
      suffix = ''

    query = '''
        select fcst.name as row1, fcst.item_id as row2, fcst.customer_id as row3,
           d.bucket as col1, d.startdate as col2, d.enddate as col3,
           coalesce(sum(forecastplan.orderstotal%s),0) as orderstotal,
           coalesce(sum(forecastplan.ordersopen%s),0) as ordersopen,
           coalesce(sum(forecastplan.ordersadjustment%s),0) as ordersadjustment,
           coalesce(sum(forecastplan.forecastbaseline%s),0) as forecastbaseline,
           coalesce(sum(forecastplan.forecastadjustment%s),0) as forecastadjustment,
           coalesce(sum(forecastplan.forecasttotal%s),0) as forecasttotal,
           coalesce(sum(forecastplan.forecastnet%s),0) as forecastnet,
           coalesce(sum(forecastplan.forecastconsumed%s),0) as forecastconsumed,
           coalesce(sum(forecastplan.ordersplanned%s),0) as ordersplanned,
           coalesce(sum(forecastplan.forecastplanned%s),0) as forecastplanned
        from (%s) fcst
        -- Multiply with buckets
        cross join (
           select name as bucket, startdate, enddate
           from common_bucketdetail
           where bucket_id = '%s' and enddate > '%s' and startdate < '%s'
           ) d
        -- Forecast plan
        left outer join forecastplan
        on fcst.name = forecastplan.forecast_id
        and forecastplan.startdate >= d.startdate
        and forecastplan.startdate < d.enddate
        -- Grouping
        group by fcst.name, fcst.item_id, fcst.customer_id,
               d.bucket, d.startdate, d.enddate
        order by %s, d.startdate
        ''' % (
          suffix, suffix, suffix, suffix, suffix, suffix, suffix, suffix, suffix, suffix,
          basesql, request.report_bucket, request.report_startdate, request.report_enddate,
          sortsql
          )
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
        'future': python_date(row[5]) > currentdate and 1 or 0,
        'orderstotal': row[6],
        'ordersopen': row[7],
        'ordersadjustment': row[8],
        'forecastbaseline': row[9],
        'forecastadjustment': row[10],
        'forecasttotal': row[11],
        'forecastnet': row[12],
        'forecastconsumed': row[13],
        'ordersplanned': row[14],
        'forecastplanned': row[15],
        }

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
          # Find the forecastplan records that are affected
          start = datetime.strptime(rec['startdate'], '%Y-%m-%d').date()
          end = datetime.strptime(rec['enddate'], '%Y-%m-%d').date()
          fcsts = [ i for i in ForecastPlan.objects.all().using(request.database).filter(forecast__name=rec['id'], startdate__gte=start, startdate__lt=end).only('ordersadjustment', 'forecastadjustment')]

          # Which field to update
          if 'adjHistory' in rec:
            field = 'ordersadjustment'
            value = rec['adjHistory']
          elif 'adjForecast' in rec:
            field = 'forecastadjustment'
            value = rec['adjForecast']
          else:
            raise Exception('Posting invalid field')

          # Sum up existing values
          tot = Decimal(0.0)
          cnt = 0
          for i in fcsts:
            tot += getattr(i, field)
            cnt += 1

          # Adjust forecastplan entries
          if tot > 0:
            # Existing non-zero records are proportionally scaled
            factor = Decimal(value) / tot
            for i in fcsts:
              setattr(i, field, getattr(i, field) * factor)
          elif cnt > 0:
            # All entries are 0 and we initialize them to the average
            eql = Decimal(value) / cnt
            for i in fcsts:
              setattr(i, field, eql)
          else:
            # Not a single active record exists, so we try to create
            fcst = Forecast.objects.all().using(request.database).get(name=rec['id'])
            fcstplan = [ ForecastPlan(forecast=fcst, startdate=j.startdate) for j in fcst.calendar.buckets.filter(startdate__gte=start, enddate__lte=end) ]
            cnt = len(fcstplan)
            if cnt > 0:
              eql = Decimal(value) / cnt
              for i in fcstplan:
                setattr(i, field, eql)
                i.save(using=request.database)
            else:
              raise Exception("Can't create matching forecastplan entries")

          # Store results
          for i in fcsts:
            i.save(using=request.database)

        except Exception as e:
          ok = False
          resp.write(e)
          resp.write('<br/>')
    finally:
      transaction.commit(using=request.database)
      transaction.leave_transaction_management(using=request.database)
    if ok:
      resp.write("OK")
    resp.status_code = ok and 200 or 403
    return resp


class UpstreamForecastPath(PathReport):
  downstream = False
  objecttype = Forecast

  @classmethod
  def getRoot(reportclass, request, entity):
    from django.core.exceptions import ObjectDoesNotExist
    try:
      fcst = Forecast.objects.using(request.database).get(name=entity)
      if fcst.operation:
        return [ (0, None, fcst.operation, 1, 0, None, 0, False) ]
      elif fcst.item.operation:
        return [ (0, None, fcst.item.operation, 1, 0, None, 0, False) ]
      else:
        raise Http404("No supply path defined for forecast %s" % entity)
    except ObjectDoesNotExist:
      raise Http404("forecast %s doesn't exist" % entity)
