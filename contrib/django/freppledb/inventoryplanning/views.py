#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import json

from django.conf import settings
from django.contrib.admin.utils import unquote
from django.contrib.admin.views.decorators import staff_member_required
from django.contrib.contenttypes.models import ContentType
from django.db.models import Q
from django.db import connections
from django.http import Http404
from django.http.response import StreamingHttpResponse, HttpResponse, HttpResponseServerError
from django.utils.decorators import method_decorator
from django.utils.translation import ugettext_lazy as _
from django.views.generic import View

from freppledb.common.report import GridFieldText, GridReport
from freppledb.common.report import GridFieldLastModified, GridFieldChoice
from freppledb.common.report import GridFieldNumber, GridFieldBool, GridFieldDuration

from freppledb.inventoryplanning.models import InventoryPlanning
from freppledb.input.models import Buffer, Item, Location
from freppledb.common.models import Comment


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
    GridFieldText('buffer', title=_('buffer'), key=True, formatter='buffer'),
    GridFieldNumber('roq_min_qty', title=_('ROQ minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('roq_max_qty', title=_('ROQ maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('roq_multiple_qty', title=_('ROQ multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_min_poc', title=_('ROQ minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    #GridFieldNumber('roq_max_poc', title=_('ROQ maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
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
  template = 'inventoryplanning/drp.html'
  title = _("Distribution planning")
  basequeryset = Buffer.objects.all()
  model = Buffer
  height = 150
  frozenColumns = 1
  multiselect = False
  editable = False
  hasTimeBuckets = True
  maxBucketLevel = 3

  rows = (
    GridFieldText('buffer', title=_('buffer'), key=True, formatter='buffer'),
    GridFieldText('item', title=_('item'), formatter='item'),
    GridFieldText('location', title=_('location'), formatter='location'),
    GridFieldDuration('leadtime', title=_('lead time'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('servicelevel', title=_('service level'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localforecast', title=_('local forecast'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localorders', title=_('local orders'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localbackorders', title=_('local backorders'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('dependentforecast', title=_('dependent forecast'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('totaldemand', title=_('total demand'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('safetystock', title=_('safety stock'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('reorderquantity', title=_('reorder quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('proposedpurchases', title=_('proposed purchases'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('proposedtransfers', title=_('proposed transfers'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localforecastvalue', title=_('local forecast value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localordersvalue', title=_('local orders value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localbackordersvalue', title=_('local backorders value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('dependentforecastvalue', title=_('dependent forecast value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('localbackordersvalue', title=_('local backorders value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('totaldemandvalue', title=_('total demand value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('safetystockvalue', title=_('safety stock value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('reorderquantityvalue', title=_('reorder quantity value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('proposedpurchasesvalue', title=_('proposed purchases value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('proposedtransfersvalue', title=_('proposed transfers value'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('onhand', title=_('on hand'), extra="formatoptions:{defaultValue:''}"),
    )

  @staticmethod
  def query(request, basequery, sortsql='1 asc'):
    cursor = connections[request.database].cursor()
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=False)

    # Assure the hierarchies are up to date  # TODO skip this check for performance reasons?
    Buffer.rebuildHierarchy(database=basequery.db)
    Item.rebuildHierarchy(database=basequery.db)
    Location.rebuildHierarchy(database=basequery.db)

    # Execute the query
    # TODO don't display buffers, but items and locations, and aggregate stuff
    # TODO add location and item attributes
    query = '''
      select buf.name, buf.item_id, buf.location_id,
        out_inventoryplanning.leadtime, out_inventoryplanning.servicelevel,
        out_inventoryplanning.totaldemand, out_inventoryplanning.reorderquantity,
        out_inventoryplanning.safetystock, buf.onhand
      from (%s) buf
      inner join inventoryplanning
      on buf.name = inventoryplanning.buffer_id
      left outer join out_inventoryplanning
      on buf.name = out_inventoryplanning.buffer_id
      order by %s
      ''' % (
        basesql, sortsql
      )
    cursor.execute(query, baseparams)

    # Build the python result
    for row in cursor.fetchall():
      yield {
        'buffer': row[0],
        'item': row[1],
        'location': row[2],
        'leadtime': row[3],
        'servicelevel': row[4],
        'totaldemand': row[5],
        'reorderquantity': row[6],
        'safetystock': row[7],
        'onhand': row[8],
        'localforecast': 666,
        'localorders': 666,
        'localbackorders': 666,
        'dependentforecast': 666,
        'totaldemand': 666,
        'safetystock': 666,
        'reorderquantity': 666,
        'proposedpurchases': 666,
        'proposedtransfers': 666,
        'localforecastvalue': 666,
        'localordersvalue': 666,
        'localbackordersvalue': 666,
        'dependentforecastvalue': 666,
        'localbackordersvalue': 666,
        'totaldemandvalue': 666,
        'safetystockvalue': 666,
        'reorderquantityvalue': 666,
        'proposedpurchasesvalue': 666,
        'proposedtransfersvalue': 666
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
        enddate, item_id, origin_id, NULL, criticality, lastmodified
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

# --and enddate > '%s' and startdate < '%s'
  sql_plan = """
    select d.bucket,
       avg(roq_calc.value) roq,
       avg(roq_override.value) roqoverride,
       avg(ss_calc.value) ss,
       avg(ss_override.value) ssoverride,
       1 dmdfcstlocal, 1 dmdorderslocal,
       1 dmddependent, 1 dmdtotal,
       1 poconfirmed, 1 doconfirmed,
       1 poproposed, 1 doproposed, 1 supply
    from buffer
    inner join inventoryplanning
    on buffer.name = inventoryplanning.buffer_id
    left outer join out_inventoryplanning
    on buffer.name = out_inventoryplanning.buffer_id
    -- join buckets
    cross join (
           select name as bucket, startdate, enddate
           from common_bucketdetail
           where bucket_id = %s
           ) d
    -- join calendars
    left outer join calendarbucket ss_calc
      on  ss_calc.calendar_id = 'Safety stock for ' || buffer.name
      and ss_calc.source = 'Inventory planning'
    left outer join calendarbucket ss_override
      on  ss_override.calendar_id = 'Safety stock for ' || buffer.name
      and ss_override.source <> 'Inventory planning'
    left outer join calendarbucket roq_calc
      on roq_calc.calendar_id = 'Reorder quantities for ' || buffer.name
      and roq_calc.source = 'Inventory planning'
    left outer join calendarbucket roq_override
      on roq_override.calendar_id = 'Reorder quantities for ' || buffer.name
      and roq_override.source <> 'Inventory planning'
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
       coalesce(sum(forecastplan.ordersplanned),0) as ordersplanned,
       coalesce(sum(forecastplan.forecastplanned),0) as forecastplanned,
       coalesce(sum(forecastplan.orderstotalvalue),0) as orderstotalvalue,
       coalesce(sum(forecastplan.ordersopenvalue),0) as ordersopenvalue,
       sum(forecastplan.ordersadjustmentvalue) as ordersadjustmentvalue,
       coalesce(sum(forecastplan.forecastbaselinevalue),0) as forecastbaselinevalue,
       sum(forecastplan.forecastadjustmentvalue) as forecastadjustmentvalue,
       coalesce(sum(forecastplan.forecasttotalvalue),0) as forecasttotalvalue,
       coalesce(sum(forecastplan.forecastnetvalue),0) as forecastnetvalue,
       coalesce(sum(forecastplan.forecastconsumedvalue),0) as forecastconsumedvalue,
       coalesce(sum(forecastplan.ordersplannedvalue),0) as ordersplannedvalue,
       coalesce(sum(forecastplan.forecastplannedvalue),0) as forecastplannedvalue
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
    ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)
    item_name = ip.buffer.item.name if ip.buffer.item else None
    location_name = ip.buffer.location.name if ip.buffer.location else None

    # TODO save current selected item-location detail to the preferences

    # Retrieve parameters
    yield ''.join([
      '{"type":"itemlocation", "name":',  json.dumps(itemlocation), ",",
      '"parameters":', json.dumps({
        "ss_multiple_qty": str(ip.ss_multiple_qty) if ip.ss_multiple_qty is not None else None,
        "ss_min_qty": str(ip.ss_min_qty) if ip.ss_multiple_qty is not None else None,
        "roq_min_poc": str(ip.roq_min_poc) if ip.ss_multiple_qty is not None else None,
        "ss_min_poc": str(ip.ss_min_poc) if ip.ss_multiple_qty is not None else None,
        "roq_min_qty": str(ip.roq_min_qty) if ip.ss_multiple_qty is not None else None,
        "demand_distribution": str(ip.demand_distribution) if ip.ss_multiple_qty is not None else None,
        "ss_max_qty": str(ip.ss_max_qty) if ip.ss_multiple_qty is not None else None,
        "leadtime_deviation": str(ip.leadtime_deviation) if ip.ss_multiple_qty is not None else None,
        "roq_max_qty": str(ip.roq_max_qty) if ip.roq_max_qty is not None else None,
        "roq_multiple_qty": str(ip.roq_multiple_qty) if ip.roq_multiple_qty is not None else None,
        "nostock": str(ip.nostock) if ip.nostock is not None else None,
        "roq_max_poc": str(ip.roq_max_poc) if ip.roq_max_poc is not None else None,
        "service_level": str(ip.service_level) if ip.service_level is not None else None,
        "demand_deviation": str(ip.demand_deviation) if ip.demand_deviation is not None else None,
        "ss_max_poc": str(ip.ss_max_poc) if ip.ss_max_poc is not None else None,
        "eoq": 49,  # TODO comes from output table
        "service_level_qty": 49, # TODO comes from output table
        "forecast_method": "auto"
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
        'ordersplanned': round(rec[9]),
        'forecastplanned': round(rec[10]),
        'orderstotalvalue': round(rec[11]),
        'ordersopenvalue': round(rec[12]),
        'ordersadjustmentvalue': round(rec[13]) if rec[13] is not None else None,
        'forecastbaselinevalue': round(rec[14]),
        'forecastadjustmentvalue': round(rec[15]) if rec[15] is not None else None,
        'forecasttotalvalue': round(rec[16]),
        'forecastnetvalue': round(rec[17]),
        'forecastconsumedvalue': round(rec[18]),
        'ordersplannedvalue': round(rec[19]),
        'forecastplannedvalue': round(rec[20])
        })

    # Retrieve inventory plan
    yield '],"plan":['
    first = True
    cursor.execute(self.sql_plan, (buckets, item_name, location_name))
    startoh = 0
    endoh = 0
    for rec in cursor.fetchall():
      if not first:
        yield ","
      else:
        first = False
      endoh += 1
      yield json.dumps({
        'bucket': rec[0],
        'roq': round(rec[1]) if rec[1] is not None else None,
        'roqoverride': round(rec[2]) if rec[2] is not None else None,
        'ss': round(rec[3]) if rec[3] is not None else None,
        'ssoverride': round(rec[4]) if rec[4] is not None else None,
        'startoh': round(startoh),
        'dmdfcstlocal': round(rec[6]) if rec[4] is not None else None,
        'dmdorderslocal': round(rec[7]) if rec[4] is not None else None,
        'dmddependent': round(rec[8]) if rec[4] is not None else None,
        'dmdtotal': round(rec[9]) if rec[4] is not None else None,
        'doconfirmed': round(rec[10]) if rec[4] is not None else None,
        'poconfirmed': round(rec[11]) if rec[4] is not None else None,
        'poproposed': round(rec[12]) if rec[4] is not None else None,
        'doproposed': round(rec[13]) if rec[4] is not None else None,
        'supply': round(rec[14]) if rec[4] is not None else None,
        'endoh': round(endoh)
        })
      startoh += endoh

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

      # Retrieve the comment
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

