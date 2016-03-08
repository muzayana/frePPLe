#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db import connections
from django.utils.translation import ugettext_lazy as _
from django.utils.text import capfirst
from django.utils.encoding import force_text

from freppledb.boot import getAttributeFields
from freppledb.input.models import Item
from freppledb.output.models import Demand
from freppledb.common.db import python_date
from freppledb.common.report import GridReport, GridPivot, GridFieldText, GridFieldNumber, GridFieldDateTime, GridFieldInteger


class OverviewReport(GridPivot):
  '''
  A report showing the independent demand for each item.
  '''
  template = 'output/demand.html'
  title = _('Demand report')
  basequeryset = Item.objects.all()
  model = Item
  permissions = (("view_demand_report", "Can view demand report"),)
  rows = (
    GridFieldText('item', title=_('item'), key=True, editable=False, field_name='name', formatter='detail', extra="role:'input/item'"),
    )
  crosses = (
    ('forecast', {'title': _('net forecast')}),
    ('orders', {'title': _('orders')}),
    ('demand', {'title': _('total demand')}),
    ('supply', {'title': _('total supply')}),
    ('backlog', {'title': _('backlog')}),
    )

  @classmethod
  def initialize(reportclass, request):
    if reportclass._attributes_added != 2:
      reportclass._attributes_added = 2
      reportclass.attr_sql = ''
      # Adding custom item attributes
      for f in getAttributeFields(Item, initially_hidden=True):
        reportclass.attr_sql += 'item.%s, ' % f.name.split('__')[-1]

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'plan'
      return {
        'title': capfirst(force_text(Item._meta.verbose_name) + " " + args[0]),
        'post_title': ': ' + capfirst(force_text(_('plan'))),
        }
    else:
      return {}

  @classmethod
  def query(reportclass, request, basequery, sortsql='1 asc'):
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=False)
    cursor = connections[request.database].cursor()

    # Assure the item hierarchy is up to date
    Item.rebuildHierarchy(database=basequery.db)

    # Execute a query to get the backlog at the start of the horizon
    startbacklogdict = {}
    query = '''
      select items.name, sum(quantity)
      from (%s) items
      inner join item
      on item.lft between items.lft and items.rght
      inner join out_demand
      on item.name = out_demand.item
        and (plandate is null or plandate >= '%s')
        and due < '%s'
      group by items.name
      ''' % (basesql, request.report_startdate, request.report_startdate)
    cursor.execute(query, baseparams)
    for row in cursor.fetchall():
      if row[0]:
        startbacklogdict[row[0]] = float(row[1])

    # Execute the query    TODO THIS QUERY ASSUMES FORECAST MODULE IS INSTALLED!
    query = '''
        select y.name, %s
               y.bucket, y.startdate, y.enddate,
               min(y.orders),
               coalesce(sum(fcst.quantity),0),
               min(y.planned)
        from (
          select x.name as name, x.lft as lft, x.rght as rght,
               x.bucket as bucket, x.startdate as startdate, x.enddate as enddate,
               coalesce(sum(demand.quantity),0) as orders,
               min(x.planned) as planned
          from (
          select items.name as name, items.lft as lft, items.rght as rght,
                 d.bucket as bucket, d.startdate as startdate, d.enddate as enddate,
                 coalesce(sum(out_demand.quantity),0) as planned
          from (%s) items
          -- Multiply with buckets
          cross join (
             select name as bucket, startdate, enddate
             from common_bucketdetail
             where bucket_id = '%s' and enddate > '%s' and startdate < '%s'
             ) d
          -- Include hierarchical children
          inner join item
          on item.lft between items.lft and items.rght
          -- Planned quantity
          left join out_demand
          on item.name = out_demand.item
          and d.startdate <= out_demand.plandate
          and d.enddate > out_demand.plandate
          and out_demand.plandate >= '%s'
          and out_demand.plandate < '%s'
          -- Grouping
          group by items.name, items.lft, items.rght, d.bucket, d.startdate, d.enddate
        ) x
        -- Requested quantity
        inner join item
          on item.lft between x.lft and x.rght
        left join demand
          on item.name = demand.item_id
          and x.startdate <= demand.due
          and x.enddate > demand.due
          and demand.due >= '%s'
          and demand.due < '%s'
          and demand.status = 'open'
        -- Grouping
        group by x.name, x.lft, x.rght, x.bucket, x.startdate, x.enddate
        ) y
        -- Forecasted quantity
        inner join item
        on item.lft between y.lft and y.rght
        left join (select forecast.item_id as item_id, forecastplan.startdate as startdate,
          forecastplan.forecastnet as quantity
          from forecastplan, forecast
          where forecastplan.forecast_id = forecast.name
          ) fcst
        on item.name = fcst.item_id
        and fcst.startdate >= y.startdate
        and fcst.startdate < y.enddate
        -- Ordering and grouping
        group by %s y.name, y.lft, y.rght, y.bucket, y.startdate, y.enddate
        order by %s, y.startdate
       ''' % (reportclass.attr_sql, basesql, request.report_bucket, request.report_startdate,
              request.report_enddate, request.report_startdate,
              request.report_enddate, request.report_startdate,
              request.report_enddate, reportclass.attr_sql, sortsql)
    cursor.execute(query, baseparams)

    # Build the python result
    previtem = None
    for row in cursor.fetchall():
      numfields = len(row)
      if row[0] != previtem:
        backlog = startbacklogdict.get(row[0], 0)
        previtem = row[0]
      backlog += float(row[numfields-3]) + float(row[numfields-2]) - float(row[numfields-1])
      res = {
        'item': row[0],
        'bucket': row[numfields-6],
        'startdate': python_date(row[numfields-5]),
        'enddate': python_date(row[numfields-4]),
        'orders': round(row[numfields-3], 1),
        'forecast': round(row[numfields-2], 1),
        'demand': round(float(row[numfields-3]) + float(row[numfields-2]), 1),
        'supply': round(row[numfields-1], 1),
        'backlog': round(backlog, 1),
        }
      idx = 1
      for f in getAttributeFields(Item):
        res[f.field_name] = row[idx]
        idx += 1
      yield res


class DetailReport(GridReport):
  '''
  A list report to show delivery plans for demand.
  '''
  template = 'output/demandplan.html'
  title = _("Demand plan detail")
  basequeryset = Demand.objects.extra(select={'forecast': "select name from forecast where out_demand.demand like forecast.name || ' - %%'"})
  model = Demand
  basequeryset = Demand.objects.all()
  permissions = (("view_demand_report", "Can view demand report"),)
  frozenColumns = 0
  editable = False
  multiselect = False
  rows = (
    GridFieldInteger('id', title=_('id'), key=True,editable=False, hidden=True),
    GridFieldText('demand', title=_('demand'), editable=False, formatter='detail', extra="role:'input/demand'"),
    GridFieldText('item', title=_('item'), editable=False, formatter='detail', extra="role:'input/item'"),
    GridFieldText('customer', title=_('customer'), editable=False, formatter='detail', extra="role:'input/customer'"),
    GridFieldNumber('quantity', title=_('quantity'), editable=False),
    GridFieldNumber('planquantity', title=_('planned quantity'), editable=False),
    GridFieldDateTime('due', title=_('due date'), editable=False),
    GridFieldDateTime('plandate', title=_('planned date'), editable=False),
    GridFieldInteger('operationplan', title=_('operationplan'), editable=False),
    )

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'plandetail'
    return {'active_tab': 'plandetail'}
