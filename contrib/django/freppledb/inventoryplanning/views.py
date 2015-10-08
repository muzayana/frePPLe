#
# Copyright (C) 2015 by frePPLe bvba
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

from django.conf import settings
from django.contrib.admin.utils import unquote
from django.contrib.admin.models import LogEntry, CHANGE, ADDITION
from django.contrib.admin.views.decorators import staff_member_required
from django.contrib.contenttypes.models import ContentType
from django.db.models import Q
from django.db import connections, transaction
from django.db.models.fields.related import RelatedField
from django.forms.models import modelform_factory
from django.http import Http404
from django.http.response import StreamingHttpResponse, HttpResponse, HttpResponseServerError
from django.utils.decorators import method_decorator
from django.utils.translation import ugettext_lazy as _
from django.utils.encoding import force_text
from django.utils.text import get_text_list
from django.views.generic import View

from freppledb.common.report import GridFieldText, GridReport
from freppledb.common.report import GridFieldLastModified, GridFieldChoice
from freppledb.common.report import GridFieldNumber, GridFieldBool, GridFieldInteger

from freppledb.inventoryplanning.models import InventoryPlanning, InventoryPlanningOutput
from freppledb.input.models import Buffer, Location, Calendar, CalendarBucket
from freppledb.input.models import Item, DistributionOrder, PurchaseOrder
from freppledb.common.models import Comment, Parameter
from freppledb.forecast.models import Forecast


import logging
logger = logging.getLogger(__name__)


class InventoryPlanningList(GridReport):
  '''
  A list report to show inventory planning parameters.

  Note:
  This view is simplified and doesn't show all fields we have available in the database
  and which are supported by the solver algorithm.
  '''
  template = 'inventoryplanning/inventoryplanninglist.html'
  title = _("inventory planning parameters")
  basequeryset = InventoryPlanning.objects.all()
  model = InventoryPlanning
  frozenColumns = 1

  rows = (
    GridFieldText('buffer', title=_('buffer'), field_name="buffer__name", key=True, formatter='buffer'),
    GridFieldChoice('roq_type', title=_('ROQ type'),
      choices=InventoryPlanning.calculationtype, extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_min_qty', title=_('ROQ minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('roq_max_qty', title=_('ROQ maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('roq_multiple_qty', title=_('ROQ multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_min_poc', title=_('ROQ minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('roq_max_poc', title=_('ROQ maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldChoice('ss_type', title=_('Safety stock type'),
      choices=InventoryPlanning.calculationtype, extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_qty', title=_('safety stock minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('ss_max_qty', title=_('safety stock maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('ss_multiple_qty', title=_('safety stock multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_poc', title=_('safety stock minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('ss_max_poc', title=_('safety stock maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('service_level', title=_('service level'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('leadtime_deviation', title=_('lead time deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('demand_deviation', title=_('demand deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldChoice('demand_distribution', title=_('demand distribution'),
      choices=InventoryPlanning.distributions, extra="formatoptions:{defaultValue:''}"),
    GridFieldBool('nostock', title=_("Do not stock")),
    GridFieldText('source', title=_('source')),
    GridFieldLastModified('lastmodified')
    )


class DRP(GridReport):
  '''
     Data assumptions:
       - No overlapping calendar entries in the ROQ or SS calendars
  '''
  template = 'inventoryplanning/drp.html'
  title = _("Distribution planning")
  basequeryset = InventoryPlanningOutput.objects.all()
  model = InventoryPlanningOutput
  height = 150
  frozenColumns = 1
  multiselect = False
  editable = False
  hasTimeBuckets = True
  maxBucketLevel = 3

  rows = (
    GridFieldText('buffer', title=_('buffer'), field_name="buffer", key=True, formatter='buffer', hidden=True),
    GridFieldText('item', title=_('item'), field_name="item", formatter='item'),
    GridFieldText('location', title=_('location'), field_name="location", formatter='location'),
    GridFieldInteger('leadtime', title=_('lead time'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('localforecast', title=_('local forecast'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('safetystock', title=_('safety stock'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('reorderquantity', title=_('reorder quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('onhand', title=_('on hand'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('overduesalesorders', title=_('overdue sales orders'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('opensalesorders', title=_('open sales orders'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('openpurchases', title=_('open purchases'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('opentransfers', title=_('open transfers'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('proposedpurchases', title=_('proposed purchases'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('proposedtransfers', title=_('proposed transfers'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('servicelevel', title=_('service level'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('localforecast', title=_('local forecast'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('localorders', title=_('local orders'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('dependentforecast', title=_('dependent forecast'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('totaldemand', title=_('total demand'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('localforecastvalue', title=_('local forecast value'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('localordersvalue', title=_('local orders value'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('dependentforecastvalue', title=_('dependent forecast value'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('totaldemandvalue', title=_('total demand value'), extra="formatoptions:{defaultValue:''}"),
    )

  @staticmethod
  def query(request, basequery):

    cursor = connections[request.database].cursor()
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=False)
    sortsql = DRP._apply_sort_index(request, prefs=request.prefs)

    # Assure the hierarchies are up to date  # TODO skip this check for performance reasons?
    #Buffer.rebuildHierarchy(database=basequery.db)
    #Item.rebuildHierarchy(database=basequery.db)
    #Location.rebuildHierarchy(database=basequery.db)

    # Execute the query
    # TODO don't display buffers, but items and locations, and aggregate stuff
    # TODO add location and item attributes
    query = '''
      select
        buffer_id, item_id, location_id,
        extract(epoch from results.leadtime)/86400, localforecast,
        safetystock, reorderquantity, results.onhand,
        overduesalesorders, opensalesorders, openpurchases, opentransfers,
        proposedpurchases, proposedtransfers
      from (%s) results
      inner join buffer
        on buffer.name = results.buffer_id
      order by %s
      ''' % (basesql, sortsql)
    cursor.execute(query, baseparams)

    # Build the python result
    for row in cursor.fetchall():
      yield {
        'buffer': row[0],
        'item': row[1],
        'location': row[2],
        'leadtime': row[3],
        'localforecast': row[4],
        'safetystock': row[5],
        'reorderquantity': row[6],
        'onhand': row[7],
        'overduesalesorders': row[8],
        'opensalesorders': row[9],
        'openpurchases': row[10],
        'opentransfers': row[11],
        'proposedpurchases': row[12],
        'proposedtransfers': row[13]
        }


class DRPitemlocation(View):

  buffer_type = None
  item_type = None
  location_type = None

  sql_oh = """
    select
      buffers.name, sum(oh.onhand)
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
    """

  sql_transactions = """
    select * from
      (
      select
        id, enddate as date, 'PO' as type, reference, status, quantity,
        startdate, enddate, item_id, location_id, supplier_id, criticality,
        lastmodified
      from purchase_order
      where item_id = %s and location_id = %s
      union all
      select
        id, startdate, 'DO out', reference, status, -quantity, startdate,
        enddate, item_id, NULL, NULL, criticality, lastmodified
      from distribution_order
      where item_id = %s and origin_id = %s and consume_material = true
      union all
      select
        id, enddate, 'DO in', reference, status, quantity, startdate,
        enddate, item_id, destination_id, origin_id, criticality,
        lastmodified
      from distribution_order
      where item_id = %s and destination_id = %s
      ) transactions
    order by date, quantity
    """

  sql_plan = """
    select d.bucket,
       avg(roq_calc.value) roq,
       avg(roq_override.value) roqoverride,
       avg(ss_calc.value) ss,
       avg(ss_override.value) ssoverride,
       1 dmdfcstlocal, 1 dmdorderslocal,
       1 dmddependent,
       -coalesce(sum(least(out_flowplan.quantity, 0)),0.0) dmdtotal,
       1 poconfirmed, 1 doconfirmed,
       1 poproposed, 1 doproposed,
       coalesce(sum(greatest(out_flowplan.quantity,0)),0.0) supply
    from buffer
    inner join inventoryplanning
    on buffer.name = inventoryplanning.buffer_id
    left outer join out_inventoryplanning
    on buffer.name = out_inventoryplanning.buffer_id
    -- join buckets
    cross join (
           select name as bucket, startdate, enddate
           from common_bucketdetail
           where bucket_id = %s and enddate > %s and startdate < %s
           ) d
    -- join calendars
    left outer join calendarbucket ss_calc
      on  ss_calc.calendar_id = 'SS for ' || buffer.name
      and ss_calc.source = 'Inventory planning'
      and d.startdate >= ss_calc.startdate
      and d.startdate < ss_calc.enddate
    left outer join calendarbucket ss_override
      on  ss_override.calendar_id = 'SS for ' || buffer.name
      and (ss_override.source <> 'Inventory planning' or ss_override.source is null)
      and d.startdate >= ss_override.startdate
      and d.startdate < ss_override.enddate
    left outer join calendarbucket roq_calc
      on roq_calc.calendar_id = 'ROQ for ' || buffer.name
      and roq_calc.source = 'Inventory planning'
      and d.startdate >= roq_calc.startdate
      and d.startdate < roq_calc.enddate
    left outer join calendarbucket roq_override
      on roq_override.calendar_id = 'ROQ for ' || buffer.name
      and (roq_override.source <> 'Inventory planning' or roq_override.source is null)
      and d.startdate >= roq_override.startdate
      and d.startdate < roq_override.enddate
    -- Consumed and produced quantities
    left join out_flowplan
    on buffer.name = out_flowplan.thebuffer
    and d.startdate <= out_flowplan.flowdate
    and d.enddate > out_flowplan.flowdate
    -- join operationplan TODO
    where item_id = %s and location_id = %s
    group by d.bucket, d.startdate
    order by d.startdate
    """

  # Implicit assumption is that this filter picks up data from a single buffer.
  sql_forecast = """
    select d.bucket as bucket,
       coalesce(sum(forecastplan.orderstotal),0) as orderstotal,
       coalesce(sum(forecastplan.ordersopen),0) as ordersopen,
       sum(forecastplan.ordersadjustment) as ordersadjustment,
       coalesce(sum(forecastplan.forecastbaseline),0) as forecastbaseline,
       sum(forecastplan.forecastadjustment) as forecastadjustment,
       coalesce(sum(forecastplan.forecasttotal),0) as forecasttotal,
       coalesce(sum(forecastplan.forecastnet),0) as forecastnet,
       coalesce(sum(forecastplan.forecastconsumed),0) as forecastconsumed,
       coalesce(sum(forecastplan.orderstotalvalue),0) as orderstotalvalue,
       coalesce(sum(forecastplan.ordersopenvalue),0) as ordersopenvalue,
       sum(forecastplan.ordersadjustmentvalue) as ordersadjustmentvalue,
       coalesce(sum(forecastplan.forecastbaselinevalue),0) as forecastbaselinevalue,
       sum(forecastplan.forecastadjustmentvalue) as forecastadjustmentvalue,
       coalesce(sum(forecastplan.forecasttotalvalue),0) as forecasttotalvalue,
       coalesce(sum(forecastplan.forecastnetvalue),0) as forecastnetvalue,
       coalesce(sum(forecastplan.forecastconsumedvalue),0) as forecastconsumedvalue

    from forecast
    -- Join buckets
    cross join (
       select name as bucket, startdate, enddate
       from common_bucketdetail
       where bucket_id = %s
       and startdate between (select min(startdate) from forecastplan)
             and (select max(startdate) from forecastplan)
       ) d
    -- Forecast plan
    left outer join forecastplan
    on forecast.name = forecastplan.forecast_id
      and forecastplan.startdate >= d.startdate
      and forecastplan.startdate < d.enddate
    -- Location join and filter
    cross join location locfilter
    inner join location locjoin
    on forecast.location_id = locjoin.name
      and locjoin.lft between locfilter.lft and locfilter.rght
    -- Item join and filter
    cross join item itemfilter
    inner join item itemjoin
    on forecast.item_id = itemjoin.name
      and itemjoin.lft between itemfilter.lft and itemfilter.rght
    -- Filter
    where locfilter.name = %s
      and itemfilter.name = %s
    -- Grouping
    group by d.bucket, d.startdate
    order by d.startdate
    """

  def getData(self, request, itemlocation):
    # This query retrieves all data for a certain itemlocation.
    # Advantage is that all data are sent to the user's browser in a single response,
    # and the user can navigate them without

    buckets = request.user.horizonbuckets
    DRP.getBuckets(request)
    ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)
    item_name = ip.buffer.item.name if ip.buffer.item else None
    location_name = ip.buffer.location.name if ip.buffer.location else None

    # Retrieve parameters
    fcst = Forecast.objects.using(request.database).filter(item=item_name, location=location_name).first()
    out_ip = InventoryPlanningOutput.objects.using(request.database).filter(pk=itemlocation).first()
    yield ''.join([
      '{"type":"itemlocation", "name":',  json.dumps(itemlocation), ",",
      '"parameters":', json.dumps({
        "roq_type": ip.roq_type if ip.roq_type is not None else 'calculated',
        "ss_type": ip.ss_type if ip.ss_type is not None else 'calculated',
        "ss_multiple_qty": str(ip.ss_multiple_qty) if ip.ss_multiple_qty is not None else None,
        "ss_min_qty": str(ip.ss_min_qty) if ip.ss_min_qty is not None else None,
        "roq_min_poc": int(ip.roq_min_poc) if ip.roq_min_poc is not None else None,
        "ss_min_poc": int(ip.ss_min_poc) if ip.ss_min_poc is not None else None,
        "roq_min_qty": str(ip.roq_min_qty) if ip.roq_min_qty is not None else None,
        "demand_distribution": ip.demand_distribution if ip.demand_distribution else 'automatic',
        "ss_max_qty": int(ip.ss_max_qty) if ip.ss_max_qty is not None else None,
        "leadtime_deviation": str(ip.leadtime_deviation) if ip.leadtime_deviation is not None else None,
        "roq_max_qty": int(ip.roq_max_qty) if ip.roq_max_qty is not None else None,
        "roq_multiple_qty": int(ip.roq_multiple_qty) if ip.roq_multiple_qty is not None else None,
        "nostock": str(ip.nostock) if ip.nostock is not None else None,
        "roq_max_poc": int(ip.roq_max_poc) if ip.roq_max_poc is not None else None,
        "service_level": str(ip.service_level) if ip.service_level is not None else None,
        "demand_deviation": int(ip.demand_deviation) if ip.demand_deviation is not None else None,
        "ss_max_poc": int(ip.ss_max_poc) if ip.ss_max_poc is not None else None,
        "roq_calculated": int(out_ip.calculatedreorderquantity) if out_ip and out_ip.calculatedreorderquantity is not None else None,
        "ss_calculated": int(out_ip.calculatedsafetystock) if out_ip and out_ip.calculatedsafetystock is not None else None,
        "forecastmethod": fcst.method if fcst else 'automatic',
        "forecasterror": "12 %"
        })
      ])

    cursor = connections[request.database].cursor()

    # Retrieve forecast data
    yield ',"forecast":['
    first = True
    cursor.execute(self.sql_forecast, (buckets, location_name, item_name))
    for rec in cursor.fetchall():
      if not first:
        yield ","
      else:
        first = False
      yield json.dumps({
        'bucket': rec[0],
        'orderstotal': round(rec[1]),
        'ordersopen': round(rec[2]),
        'ordersadjustment': round(rec[3]) if rec[3] is not None else None,
        'forecastbaseline': round(rec[4]),
        'forecastadjustment': round(rec[5]) if rec[5] is not None else None,
        'forecasttotal': round(rec[6]),
        'forecastnet': round(rec[7]),
        'forecastconsumed': round(rec[8]),
        'orderstotalvalue': round(rec[9]),
        'ordersopenvalue': round(rec[10]),
        'ordersadjustmentvalue': round(rec[11]) if rec[11] is not None else None,
        'forecastbaselinevalue': round(rec[12]),
        'forecastadjustmentvalue': round(rec[13]) if rec[13] is not None else None,
        'forecasttotalvalue': round(rec[14]),
        'forecastnetvalue': round(rec[15]),
        'forecastconsumedvalue': round(rec[16]),
        })

    # Retrieve onhand at the start of the planning horizon
    cursor.execute('''
      select out_flowplan.onhand
        from out_flowplan
        inner join (
          select max(id) as id
          from out_flowplan
          where flowdate < '%s'
          and thebuffer = %%s
          ) maxid
        on maxid.id = out_flowplan.id
        and out_flowplan.thebuffer = %%s
      ''' % request.report_startdate,
      (itemlocation, itemlocation)
      )
    startoh = cursor.fetchone()[0]
    endoh = startoh

    # Retrieve inventory plan
    yield '],"plan":['
    first = True
    cursor.execute(self.sql_plan, (request.report_bucket, request.report_startdate, request.report_enddate, item_name, location_name))
    for rec in cursor.fetchall():
      if not first:
        yield ","
      else:
        first = False
      dmdtotal = round(rec[8]) if rec[8] is not None else None
      supply = round(rec[13]) if rec[13] is not None else None
      endoh += supply - dmdtotal
      yield json.dumps({
        'bucket': rec[0],
        'roq': round(rec[1]) if rec[1] is not None else None,
        'roqoverride': round(rec[2]) if rec[2] is not None else None,
        'ss': round(rec[3]) if rec[3] is not None else None,
        'ssoverride': round(rec[4]) if rec[4] is not None else None,
        'startoh': round(startoh),
        'dmdfcstlocal': round(rec[5]) if rec[5] is not None else None,
        'dmdorderslocal': round(rec[6]) if rec[6] is not None else None,
        'dmddependent': round(rec[7]) if rec[7] is not None else None,
        'dmdtotal': dmdtotal,
        'doconfirmed': round(rec[9]) if rec[9] is not None else None,
        'poconfirmed': round(rec[10]) if rec[10] is not None else None,
        'poproposed': round(rec[11]) if rec[11] is not None else None,
        'doproposed': round(rec[12]) if rec[12] is not None else None,
        'supply': supply,
        'endoh': round(endoh)
        })
      startoh = endoh

    # Retrieve transactions
    yield '],"transactions":['
    first = True
    cursor.execute(self.sql_transactions, (item_name, location_name, item_name, location_name, item_name, location_name))
    for rec in cursor.fetchall():
      if not first:
        yield ","
      else:
        first = False
      yield json.dumps({
        'id': rec[0],
        'date': str(rec[1]),
        'type': rec[2],
        'reference': rec[3],
        'status': rec[4],
        'quantity': str(rec[5]),
        'startdate': str(rec[6]),
        'enddate': str(rec[7]),
        'item': rec[8],
        'location': rec[9],
        'origin': rec[10],
        'criticality': str(rec[11]),
        'lastmodified': str(rec[12])
        })

    # Retrieve comments
    if self.buffer_type is None:
      self.buffer_type = ContentType.objects.get_for_model(Buffer)
      self.item_type = ContentType.objects.get_for_model(Item)
      self.location_type = ContentType.objects.get_for_model(Location)
    comments = Comment.objects.using(request.database).filter(
      Q(content_type=self.buffer_type.id, object_pk=ip.buffer.name)
      | Q(content_type=self.item_type.id, object_pk=ip.buffer.item.name if ip.buffer.item else None)
      | Q(content_type=self.location_type.id, object_pk=ip.buffer.location.name if ip.buffer.location else None)
      ).order_by('-lastmodified')
    yield '],"comments":['
    first = True
    for i in comments:
      if first:
        first = False
      else:
        yield ","
      if i.content_type == self.buffer_type:
        t = "itemlocation"
      elif i.content_type == self.item_type:
        t = "item"
      else:
        t = "location"
      yield json.dumps({
        "user": "%s (%s)" % (i.user.username, i.user.get_full_name()),
        "lastmodified": str(i.lastmodified),
        "comment": i.comment,
        "type": t
        })
    yield "]}"

    # Retrieve history: lazy?

    # Save current selected item-location detail to the preferences
    prefs = request.user.getPreference(DRP.getKey())
    if prefs:
      prefs['name'] = itemlocation
      prefs['type'] = "itemlocation"
      request.user.setPreference(DRP.getKey(), prefs)


  @method_decorator(staff_member_required)
  def get(self, request, arg):
    # Only accept ajax requests on this URL
    if not request.is_ajax():
      raise Http404('Only ajax requests allowed')

    # Verify permissions TODO

    # Unescape special characters in the argument, which is encoded django-admin style.
    itemlocation = unquote(arg)

    # Stream back the response
    response = StreamingHttpResponse(
      content_type='application/json; charset=%s' % settings.DEFAULT_CHARSET,
      streaming_content=self.getData(request, itemlocation)
      )
    response['Cache-Control'] = "no-cache, no-store"
    return response


  @method_decorator(staff_member_required)
  def post(self, request, arg):
    # Only accept ajax requests on this URL
    if not request.is_ajax():
      raise Http404('Only ajax requests allowed')

    try:
      # Unescape special characters in the argument, which is encoded django-admin style.
      itemlocation = unquote(arg)

      # Look up the relevant object
      ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)

      # Retrieve the posted data
      data = json.JSONDecoder().decode(request.read().decode(request.encoding or settings.DEFAULT_CHARSET))
      print("posted:", itemlocation, data)

      # Save comments
      with transaction.atomic(using=request.database):

        # Save the plan overrides
        if 'plan' in data:
          roq_calendar = None
          ss_calendar = None
          for row in data['plan']:
            if 'roqoverride' in row:
              if not roq_calendar:
                roq_calendar, created = Calendar.objects.using(request.database).get_or_create(name="ROQ for %s" % itemlocation)
                if created:
                  roq_calendar.source = 'Inventory planning'
                  roq_calendar.default = 1
                  roq_calendar.save(using=request.database)
              if row['roqoverride'] == '':
                # Delete a bucket
                CalendarBucket.objects.using(request.database).filter(calendar=roq_calendar, startdate=datetime.strptime(row['startdate'], '%Y-%m-%d')).exclude(source='Inventory planning').delete()
              else:
                # Create or update a bucket
                cal_bucket, created = CalendarBucket.objects.using(request.database).get_or_create(calendar=roq_calendar, startdate=datetime.strptime(row['startdate'], '%Y-%m-%d'))
                cal_bucket.value = row['roqoverride']
                cal_bucket.enddate = datetime.strptime(row['enddate'], '%Y-%m-%d')
                cal_bucket.priority = 0
                if cal_bucket.source == 'Inventory planning':
                  cal_bucket.source = None
                cal_bucket.save(using=request.database)
            if 'ssoverride' in row:
              if not ss_calendar:
                ss_calendar, created = Calendar.objects.using(request.database).get_or_create(name="SS for %s" % itemlocation)
                if created:
                  ss_calendar.source = 'Inventory planning'
                  ss_calendar.default = 1
                  ss_calendar.save(using=request.database)
              if row['ssoverride'] == '':
                # Delete a bucket
                CalendarBucket.objects.using(request.database).filter(calendar=ss_calendar, startdate=datetime.strptime(row['startdate'], '%Y-%m-%d')).exclude(source='Inventory planning').delete()
              else:
                cal_bucket, created = CalendarBucket.objects.using(request.database).get_or_create(calendar=ss_calendar, startdate=datetime.strptime(row['startdate'], '%Y-%m-%d'))
                cal_bucket.value = row['ssoverride']
                cal_bucket.enddate = datetime.strptime(row['enddate'], '%Y-%m-%d')
                cal_bucket.priority = 0
                if cal_bucket.source == 'Inventory planning':
                  cal_bucket.source = None
                cal_bucket.save(using=request.database)

        # Save the forecast overrides
        if 'forecast' in data:
          fcst = None
          for row in data['forecast']:
            if not fcst:
              # Assumption: we find only a single forecast matching this
              # item+location combination
              fcst = Forecast.objects.all().using(request.database).get(item=ip.buffer.item, location=ip.buffer.location)
            strt = datetime.strptime(row['startdate'], '%Y-%m-%d').date()
            nd = datetime.strptime(row['enddate'], '%Y-%m-%d').date()
            if 'adjHistory3' in row:
              fcst.updatePlan(
                strt.replace(year=strt.year - 3),
                nd.replace(year=nd.year - 3),
                None,
                Decimal(row['adjHistory3']) if (row.get('adjHistory3','') != '') else None,
                True,  # Units
                request.database
                )
            elif 'adjHistory2' in row:
              fcst.updatePlan(
                strt.replace(year=strt.year - 2),
                nd.replace(year=nd.year - 2),
                None,
                Decimal(row['adjHistory2']) if (row.get('adjHistory2','') != '') else None,
                True,  # Units
                request.database
                )
            elif 'adjHistory1' in row:
              fcst.updatePlan(
                strt.replace(year=strt.year - 1),
                nd.replace(year=nd.year - 1),
                None,
                Decimal(row['adjHistory1']) if (row.get('adjHistory1','') != '') else None,
                True,  # Units
                request.database
                )
            elif 'adjForecast' in row:
              fcst.updatePlan(
                datetime.strptime(row['startdate'], '%Y-%m-%d').date(),
                datetime.strptime(row['enddate'], '%Y-%m-%d').date(),
                Decimal(row['adjForecast']) if (row.get('adjForecast','') != '') else None,
                None,
                True,  # Units
                request.database
                )

        # Save the inventory parameters
        # TODO better error handling using a modelform
        if 'parameters' in data:
          param = data['parameters']
          val = param.get('forecastmethod', '').lower()
          if val != '':
            fcst = Forecast.objects.all().using(request.database).get(item=ip.buffer.item, location=ip.buffer.location)
            fcst.method = val
            fcst.save(using=request.database)
          ip.roq_type = param.get('roq_type', None)
          val = param.get('roq_min_qty', '')
          if val != '':
            ip.roq_min_qty = float(val)
          val = param.get('roq_min_poc', '')
          if val != '':
            ip.roq_min_poc = float(val)
          ip.ss_type = param.get('ss_type', None)
          val = param.get('ss_min_qty', '')
          if val != '':
            ip.ss_min_qty = float(val)
          val = param.get('ss_min_poc', '')
          if val != '':
            ip.ss_min_poc = float(val)
          val = param.get('demand_deviation', '')
          if val != '':
            ip.demand_deviation = float(val)
          val = param.get('leadtime_deviation', '')
          if val != '':
            ip.leadtime_deviation = float(val)
          val = param.get('service_level', '')
          if val != '':
            ip.service_level = float(val)
          val = param.get('nostock', '')
          if val != '':
            if val:
              ip.nostock = True
            else:
              ip.nostock = False
          val = param.get('demand_distribution', None)
          if val is not None:
            ip.demand_distribution = val
          ip.save(using=request.database)

        # Save transactions  TODO CODE IS INCOMPLETE
        po_form = None
        do_form = None
        if 'transactions' in data:
          for row in data['transactions']:
            print (row)
            transactiontype = row.get('type', '')
            if transactiontype == 'PO':
              content_type_id = ContentType.objects.get_for_model(PurchaseOrder).pk
              if not po_form:
                obj = PurchaseOrder.objects.using(request.database).get(pk=row['id'])
                po_form = modelform_factory(PurchaseOrder,
                  fields=(
                    'reference', 'status', 'quantity', 'origin', 'id', 'date',
                    'startdate', 'enddate', 'item'
                    ),
                  formfield_callback=lambda f: (isinstance(f, RelatedField) and f.formfield(using=request.database)) or f.formfield()
                  )
                form = po_form(row, instance=obj)
            elif transactiontype == 'DO':
              content_type_id = ContentType.objects.get_for_model(DistributionOrder).pk
              if not do_form:
                do_form = modelform_factory(DistributionOrder,
                  fields=(
                    'reference', 'status', 'quantity', 'origin', 'id', 'date',
                    'startdate', 'enddate', 'item'
                    ),
                  formfield_callback=lambda f: (isinstance(f, RelatedField) and f.formfield(using=request.database)) or f.formfield()
                  )
              obj = PurchaseOrder.objects.using(request.database).get(pk=row['id'])
              form = do_form(row, instance=obj)
              if form.has_changed():
                obj = form.save(commit=False)
                obj.save(using=request.database)
                LogEntry(
                  user_id=request.user.pk,
                  content_type_id=content_type_id,
                  object_id=obj.pk,
                  object_repr=force_text(obj),
                  action_flag=CHANGE,
                  change_message=_('Changed %s.') % get_text_list(form.changed_data, _('and'))
                ).save(using=request.database)
            else:
              raise Exception("Invalid transaction")

        # Save the comment
        if 'commenttype' in data and 'comment' in data:
          if data['commenttype'] == 'item' and ip.buffer.item:
            Comment(
              content_object=ip.buffer.item,
              user=request.user,
              comment=data['comment']
              ).save(using=request.database)
          elif data['commenttype'] == 'location' and ip.buffer.location:
            Comment(
              content_object=ip.buffer.location,
              user=request.user,
              comment=data['comment']
              ).save(using=request.database)
          elif data['commenttype'] == 'itemlocation':
            Comment(
              content_object=ip.buffer,
              user=request.user,
              comment=data['comment']
              ).save(using=request.database)
          else:
            raise Exception("Invalid comment data")

      return HttpResponse(content="OK")

    except Exception as e:
      logger.error("Error saving DRP updates: %s" % e)
      return HttpResponseServerError('Error saving updates')


class DRPitem(DRPitemlocation):
  def getData(self, request, itemlocation):
    # This query retrieves all data for a certain itemlocation.
    # Advantage is that all data are sent to the user's browser in a single response,
    # and the user can navigate them without

    # Retrieve forecast data
    yield '{"type":"item", "name":%s,' %  +  json.dumps(itemlocation)
    yield '"forecast":' + json.dumps([itemlocation, {"test2": "valzezeze", "koko2": 1222}]) # TODO
    yield ","
    # Retrieve inventory plan
    yield '"plan":' + json.dumps({"test": "val", "koko": 1}) # TODO
    yield "}"
    # Retrieve transactions
    # Retrieve comments
    # Retrieve history: lazy?


class DRPlocation(DRPitemlocation):
  def getData(self, request, itemlocation):
    # This query retrieves all data for a certain itemlocation.
    # Advantage is that all data are sent to the user's browser in a single response,
    # and the user can navigate them without

    # Retrieve forecast data
    yield '{"type":"location", "name":' +  json.dumps(itemlocation) + ","
    yield '"forecast":' + json.dumps([itemlocation, {"test2": "valzezeze", "koko2": 1222}])
    yield ","
    # Retrieve inventory plan
    yield '"plan":' + json.dumps({"test": "val", "koko": 1})
    yield "}"
    # Retrieve transactions
    # Retrieve comments
    # Retrieve history: lazy?

