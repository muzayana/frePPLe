#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
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
from django.utils.encoding import force_unicode

from freppledb.input.models import Buffer
from freppledb.output.models import FlowPlan
from freppledb.common.db import sql_max, sql_min, python_date
from freppledb.common.report import GridReport, GridPivot, GridFieldText, GridFieldNumber, GridFieldDateTime, GridFieldBool, GridFieldInteger


class OverviewReport(GridPivot):
  '''
  A report showing the inventory profile of buffers.
  '''
  template = 'output/buffer.html'
  title = _('Inventory report')
  basequeryset = Buffer.objects.only('name','item__name','location__name','lft','rght','onhand')
  model = Buffer
  rows = (
    GridFieldText('buffer', title=_('buffer'), key=True, field_name='name', formatter='buffer', editable=False),
    GridFieldText('item', title=_('item'), field_name='item__name', formatter='item', editable=False),
    GridFieldText('location', title=_('location'), field_name='location__name', formatter='location', editable=False),
    GridFieldText(None, width=100, extra='formatter:graph', editable=False),
    )
  crosses = (
    ('startoh', {'title': _('start inventory'),}),
    ('produced', {'title': _('produced'),}),
    ('consumed', {'title': _('consumed'),}),
    ('endoh', {'title': _('end inventory'),}),
    )

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      return {
        'title': capfirst(force_unicode(Buffer._meta.verbose_name) + " " + args[0]),
        'post_title': ': ' + capfirst(force_unicode(_('plan'))),
        }
    else:
      return {}

  @staticmethod
  def query(request, basequery, bucket, startdate, enddate, sortsql='1 asc'):
    cursor = connections[request.database].cursor()
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=True)

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
      ''' % (basesql, startdate)
    cursor.execute(query, baseparams)
    for row in cursor.fetchall(): startohdict[row[0]] = float(row[1])

    # Execute the actual query
    query = '''
      select buf.name as row1, buf.item_id as row2, buf.location_id as row3,
             d.bucket as col1, d.startdate as col2, d.enddate as col3,
             coalesce(sum(%s),0.0) as consumed,
             coalesce(-sum(%s),0.0) as produced
        from (%s) buf
        -- Multiply with buckets
        cross join (
             select name as bucket, startdate, enddate
             from common_bucketdetail
             where bucket_id = '%s' and enddate > '%s' and startdate < '%s'
             ) d
        -- Include child buffers
        inner join buffer
        on buffer.lft between buf.lft and buf.rght
        -- Consumed and produced quantities
        left join out_flowplan
        on buffer.name = out_flowplan.thebuffer
        and d.startdate <= out_flowplan.flowdate
        and d.enddate > out_flowplan.flowdate
        and out_flowplan.flowdate >= '%s'
        and out_flowplan.flowdate < '%s'
        -- Grouping and sorting
        group by buf.name, buf.item_id, buf.location_id, buf.onhand, d.bucket, d.startdate, d.enddate
        order by %s, d.startdate
      ''' % (sql_max('out_flowplan.quantity','0.0'), sql_min('out_flowplan.quantity','0.0'),
        basesql, bucket, startdate, enddate, startdate, enddate, sortsql)
    cursor.execute(query, baseparams)

    # Build the python result
    prevbuf = None
    for row in cursor.fetchall():
      if row[0] != prevbuf:
        prevbuf = row[0]
        try: startoh = startohdict[prevbuf]
        except: startoh = 0
        endoh = startoh + float(row[6] - row[7])
      else:
        startoh = endoh
        endoh += float(row[6] - row[7])
      yield {
        'buffer': row[0],
        'item': row[1],
        'location': row[2],
        'bucket': row[3],
        'startdate': python_date(row[4]),
        'enddate': python_date(row[5]),
        'startoh': round(startoh,1),
        'produced': round(row[6],1),
        'consumed': round(row[7],1),
        'endoh': round(endoh,1),
        }


class DetailReport(GridReport):
  '''
  A list report to show flowplans.
  '''
  template = 'output/flowplan.html'
  title = _("Inventory detail report")
  model = FlowPlan
  frozenColumns = 0
  editable = False
  multiselect = False

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      return FlowPlan.objects.filter(thebuffer__exact=args[0]).extra(select={'operation_in': "select name from operation where out_operationplan.operation = operation.name",})
    else:
      return FlowPlan.objects.extra(select={'operation_in': "select name from operation where out_operationplan.operation = operation.name",})

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    return {'active_tab': 'plandetail'}

  rows = (
    GridFieldText('thebuffer', title=_('buffer'), key=True, formatter='buffer', editable=False),
    GridFieldText('operationplan__operation', title=_('operation'), formatter='operation', editable=False),
    GridFieldNumber('quantity', title=_('quantity'), editable=False),
    GridFieldDateTime('flowdate', title=_('date'), editable=False),
    GridFieldNumber('onhand', title=_('onhand'), editable=False),
    GridFieldBool('operationplan__locked', title=_('locked'), editable=False),
    GridFieldInteger('operationplan', title=_('operationplan'), editable=False),
    )

