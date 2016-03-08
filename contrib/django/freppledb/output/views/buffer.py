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
from freppledb.input.models import Buffer, Item, Location
from freppledb.output.models import FlowPlan
from freppledb.common.db import sql_max, sql_min, python_date, string_agg
from freppledb.common.report import GridReport, GridPivot, GridFieldText, GridFieldNumber
from freppledb.common.report import GridFieldDateTime, GridFieldBool, GridFieldInteger


class OverviewReport(GridPivot):
  '''
  A report showing the inventory profile of buffers.
  '''
  template = 'output/buffer.html'
  title = _('Inventory report')
  basequeryset = Buffer.objects.only('name', 'item__name', 'location__name', 'lft', 'rght', 'onhand')
  model = Buffer
  permissions = (('view_inventory_report', 'Can view inventory report'),)
  rows = (
    GridFieldText('buffer', title=_('buffer'), key=True, editable=False, field_name='name', formatter='detail', extra="role:'input/buffer'"),
    GridFieldText('item', title=_('item'), editable=False, field_name='item__name', formatter='detail', extra="role:'input/item'"),
    GridFieldText('location', title=_('location'), editable=False, field_name='location__name', formatter='detail', extra="role:'input/location'"),
    )
  crosses = (
    ('startoh', {'title': _('start inventory')}),
    ('produced', {'title': _('produced')}),
    ('consumed', {'title': _('consumed')}),
    ('endoh', {'title': _('end inventory')}),
    )

  @classmethod
  def initialize(reportclass, request):
    if reportclass._attributes_added != 2:
      reportclass._attributes_added = 2
      reportclass.attr_sql = ''
      # Adding custom item attributes
      for f in getAttributeFields(Item, related_name_prefix="item", initially_hidden=True):
        reportclass.rows += (f,)
        reportclass.attr_sql += 'item.%s, ' % f.name.split('__')[-1]
      # Adding custom location attributes
      for f in getAttributeFields(Location, related_name_prefix="location", initially_hidden=True):
        reportclass.rows += (f,)
        reportclass.attr_sql += 'location.%s, ' % f.name.split('__')[-1]

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'plan'
      return {
        'title': capfirst(force_text(Buffer._meta.verbose_name) + " " + args[0]),
        'post_title': ': ' + capfirst(force_text(_('plan'))),
        }
    else:
      return {}

  @classmethod
  def query(reportclass, request, basequery, sortsql='1 asc'):
    cursor = connections[request.database].cursor()
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=False)

    # Assure the item hierarchy is up to date
    Buffer.rebuildHierarchy(database=basequery.db)

    # Execute a query  to get the onhand value at the start of our horizon
    startohdict = {}
    query = '''
      select buffers.name, sum(oh.onhand)
      from (%s) buffers
      inner join buffer
      on buffer.lft between buffers.lft and buffers.rght
      inner join (
      select out_flowplan.thebuffer as thebuffer, out_flowplan.onhand as onhand
      from out_flowplan,
        (select thebuffer, max(id) as id
         from out_flowplan
         where flowdate < '%s'
         group by thebuffer
        ) maxid
      where maxid.thebuffer = out_flowplan.thebuffer
      and maxid.id = out_flowplan.id
      ) oh
      on oh.thebuffer = buffer.name
      group by buffers.name
      ''' % (basesql, request.report_startdate)
    cursor.execute(query, baseparams)
    for row in cursor.fetchall():
      startohdict[row[0]] = float(row[1])

    # Execute the actual query
    query = '''
      select
        invplan.buffer_id, item.name, location.name, %s
        invplan.bucket, invplan.startdate, invplan.enddate,
        invplan.consumed, invplan.produced
      from (
        select
          buf.name as buffer_id,
          d.bucket as bucket, d.startdate as startdate, d.enddate as enddate,
          coalesce(sum(%s),0.0) as consumed,
          coalesce(-sum(%s),0.0) as produced
        from (%s) buf
        -- Multiply with buckets
        cross join (
             select name as bucket, startdate, enddate
             from common_bucketdetail
             where bucket_id = %%s and enddate > %%s and startdate < %%s
             ) d
        -- Include child buffers
        inner join buffer
        on buffer.lft between buf.lft and buf.rght
        -- Consumed and produced quantities
        left join out_flowplan
        on buffer.name = out_flowplan.thebuffer
        and d.startdate <= out_flowplan.flowdate
        and d.enddate > out_flowplan.flowdate
        and out_flowplan.flowdate >= %%s
        and out_flowplan.flowdate < %%s
        -- Grouping and sorting
        group by buf.name, buf.item_id, buf.location_id, buf.onhand, d.bucket, d.startdate, d.enddate
        ) invplan
      left outer join buffer on
        invplan.buffer_id = buffer.name
      left outer join item on
        buffer.item_id = item.name
      left outer join location on
        buffer.location_id = location.name
      order by %s, invplan.startdate
      ''' % (
        reportclass.attr_sql, sql_max('out_flowplan.quantity', '0.0'), sql_min('out_flowplan.quantity', '0.0'),
        basesql, sortsql
      )
    cursor.execute(query, baseparams + (request.report_bucket, request.report_startdate, request.report_enddate,
        request.report_startdate, request.report_enddate))

    # Build the python result
    prevbuf = None
    for row in cursor.fetchall():
      numfields = len(row)
      if row[0] != prevbuf:
        prevbuf = row[0]
        startoh = startohdict.get(prevbuf, 0)
        endoh = startoh + float(row[numfields-2] - row[numfields-1])
      else:
        startoh = endoh
        endoh += float(row[numfields-2] - row[numfields-1])
      res =  {
        'buffer': row[0],
        'item': row[1],
        'location': row[2],
        'bucket': row[numfields-5],
        'startdate': python_date(row[numfields-4]),
        'enddate': python_date(row[numfields-3]),
        'startoh': round(startoh, 1),
        'produced': round(row[numfields-2], 1),
        'consumed': round(row[numfields-1], 1),
        'endoh': round(endoh, 1),
        }
      # Add attribute fields
      idx = 3
      for f in getAttributeFields(Item, related_name_prefix="item"):
        res[f.field_name] = row[idx]
        idx += 1
      for f in getAttributeFields(Location, related_name_prefix="location"):
        res[f.field_name] = row[idx]
        idx += 1
      yield res


class DetailReport(GridReport):
  '''
  A list report to show flowplans.
  '''
  template = 'output/flowplan.html'
  title = _("Inventory detail report")
  model = FlowPlan
  permissions = (('view_inventory_report', 'Can view inventory report'),)
  frozenColumns = 0
  editable = False
  multiselect = False

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      base = FlowPlan.objects.filter(thebuffer__exact=args[0])
    else:
      base = FlowPlan.objects
    return base.select_related() \
      .extra(select={
        'operation_in': "select name from operation where out_operationplan.operation = operation.name",
        'demand': ("select %s(q || ' : ' || d, ', ') from ("
                   "select round(sum(quantity)) as q, demand as d "
                   "from out_demandpegging "
                   "where out_demandpegging.operationplan = out_flowplan.operationplan_id "
                   "group by demand order by 1 desc, 2) peg"
                   % string_agg())
        })

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'plandetail'
    return {'active_tab': 'plandetail'}

  rows = (
    GridFieldInteger('id', title=_('id'),  key=True,editable=False, hidden=True),
    GridFieldText('thebuffer', title=_('buffer'), editable=False, formatter='detail', extra="role:'input/buffer'"),
    GridFieldText('operationplan__operation', title=_('operation'), editable=False, formatter='detail', extra="role:'input/operation'"),
    GridFieldNumber('quantity', title=_('quantity'), editable=False),
    GridFieldDateTime('flowdate', title=_('date'), editable=False),
    GridFieldNumber('onhand', title=_('onhand'), editable=False),
    GridFieldNumber('operationplan__criticality', title=_('criticality'), editable=False),
    GridFieldBool('operationplan__locked', title=_('locked'), editable=False),
    GridFieldNumber('operationplan__quantity', title=_('operationplan quantity'), editable=False),
    GridFieldText('demand', title=_('demand quantity'), formatter='demanddetail', extra="role:'input/demand'", width=300, editable=False),
    GridFieldInteger('operationplan', title=_('operationplan'), editable=False),
    )
