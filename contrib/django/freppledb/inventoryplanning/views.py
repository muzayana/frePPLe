#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime, timedelta
from decimal import Decimal
import gc
import inspect
import json

from django.conf import settings
from django.contrib.admin.utils import unquote
from django.contrib.admin.models import LogEntry, CHANGE
from django.contrib.admin.views.decorators import staff_member_required
from django.contrib.contenttypes.models import ContentType
from django.db.models import Q, Min, Max
from django.db import connections, transaction
from django.db.models.fields.related import RelatedField
from django.forms.models import modelform_factory
from django.http import Http404, JsonResponse
from django.http.response import StreamingHttpResponse, HttpResponse, HttpResponseServerError
from django.utils.decorators import method_decorator
from django.utils.translation import ugettext_lazy as _
from django.utils.encoding import force_text
from django.utils.text import get_text_list
from django.views.generic import View

from freppledb.common.report import GridFieldText, GridReport
from freppledb.common.report import GridFieldLastModified, GridFieldChoice
from freppledb.common.report import GridFieldNumber, GridFieldBool, GridFieldInteger
from freppledb.common.models import Comment, Parameter, BucketDetail
from freppledb.forecast.models import Forecast, ForecastPlan
from freppledb.input.models import Buffer, Location, Calendar, CalendarBucket, Demand
from freppledb.input.models import Item, DistributionOrder, PurchaseOrder, ItemSupplier, ItemDistribution
from freppledb.inventoryplanning.models import InventoryPlanning, InventoryPlanningOutput

import logging
logger = logging.getLogger(__name__)


class Replanner:

  loglevel = 0

  def __init__(self, db):
    # Load the frepple extension module in our web server process
    import frepple

    if 'demand_forecast' not in [ a[0] for a in inspect.getmembers(frepple) ]:
      frepple.loadmodule('mod_forecast.so')
      frepple.loadmodule('mod_inventoryplanning.so')

    # The current date
    try:
      self.current_date = datetime.strptime(
        Parameter.objects.using(db).get(name="currentdate").value,
        "%Y-%m-%d %H:%M:%S"
        )
    except:
      self.current_date = datetime.now()

    # Forecast calendar and its buckets
    tmp = Parameter.getValue('forecast.calendar', db, None)
    self.frepple_calendar = frepple.calendar(
      name="%s %s " % (db, tmp),
      default=0
      )
    self.horizon_history = self.current_date - timedelta(days=int(Parameter.getValue('forecast.Horizon_history', db, 10000)))
    self.horizon_future = self.current_date + timedelta(days=int(Parameter.getValue('forecast.Horizon_future', db, 365)))
    self.fcst_buckets = []
    t = ForecastPlan.objects.all().using(db).aggregate(Min('startdate'), Max('startdate'))
    self.forecastplan_min = t['startdate__min']
    self.forecastplan_max = t['startdate__max']
    for bckt in CalendarBucket.objects.all().using(db).filter(
         calendar__name=tmp, startdate__gte=self.forecastplan_min, startdate__lte=self.forecastplan_max
         ).order_by('startdate'):
      frepple_bckt = self.frepple_calendar.addBucket(bckt.id)
      frepple_bckt.start = bckt.startdate
      frepple_bckt.end = bckt.enddate
      frepple_bckt.value = bckt.value
      if bckt.startdate >= self.current_date and bckt.startdate < self.horizon_future:
        self.fcst_buckets.append(bckt.startdate)

    # Forecast solver
    cursor = connections[db].cursor()
    kw = {'loglevel': self.loglevel}
    cursor.execute('''select name, value
       from common_parameter
       where name like 'forecast.%%'
       ''')
    for key, value in cursor.fetchall():
      if key in ('forecast.Horizon_future', 'forecast.Horizon_history', 'forecast.loglevel'):
        continue
      elif key in ('forecast.DueWithinBucket',):
        kw[key[9:]] = value
      elif key in ('forecast.calendar',):
        kw[key[9:]] = self.frepple_calendar
      elif key in ('forecast.Iterations', 'forecast.loglevel', 'forecast.Skip'
                   'forecast.MovingAverage_order', 'forecast.Net_CustomerThenItemHierarchy',
                   'forecast.Net_MatchUsingDeliveryOperation', 'forecast.Net_NetEarly',
                   'forecast.Net_NetLate', ):
        kw[key[9:]] = int(value)
      else:
        kw[key[9:]] = float(value)
    self.forecast_solver =  frepple.solver_forecast(**kw)

    # Inventory planning solver
    self.ip_solver = frepple.solver_inventoryplanning(
      calendar=self.frepple_calendar,
      horizon_start=int(Parameter.getValue('inventoryplanning.horizon_start', db, 0)),
      horizon_end=int(Parameter.getValue('inventoryplanning.horizon_end', db, 365)),
      holding_cost=float(Parameter.getValue('inventoryplanning.holding_cost', db, 0.05)),
      fixed_order_cost=float(Parameter.getValue('inventoryplanning.fixed_order_cost', db, 20)),
      loglevel=self.loglevel,
      service_level_on_average_inventory=(Parameter.getValue("inventoryplanning.service_level_on_average_inventory", db, "true") == "true")
      )

    # Propagation solver
    self.mrp_solver = frepple.solver_mrp(
      constraints=15,
      plantype=1, # Constrained plan
      loglevel=self.loglevel,
      lazydelay=int(Parameter.getValue('lazydelay', db, '86400')),
      allowsplits=(Parameter.getValue('allowsplits', db, 'true') == "true"),
      rotateresources=(Parameter.getValue('plan.rotateResources', db, 'true') == "true"),
      plansafetystockfirst=(Parameter.getValue('plan.planSafetyStockFirst', db, 'false') == "false"),
      iterationmax=int(Parameter.getValue('plan.iterationmax', db, '0'))
      )


# Dictionary with replanning models
replanners = {}


class InventoryPlanningList(GridReport):
  '''
  A list report to show inventory planning parameters.

  Note:
  This view is simplified and doesn't show all fields we have available in the database
  and which are supported by the solver algorithm.
  '''
  template = 'admin/base_site_grid.html'
  title = _("inventory planning parameters")
  basequeryset = InventoryPlanning.objects.all()
  model = InventoryPlanning
  frozenColumns = 1

  rows = (
    GridFieldText('buffer', title=_('buffer'), field_name="buffer__name", key=True, formatter='detail', extra="role:'input/buffer'"),
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
  Assumptions:
    - No overlapping calendar entries in the ROQ or SS calendars
    - Assumes lowest time level is 'month'
  '''
  template = 'inventoryplanning/drp.html'
  title = _("Distribution planning")
  permissions = (('view_distribution_report', 'Can view distribution report'),)
  model = InventoryPlanningOutput
  height = 150
  frozenColumns = 3
  multiselect = False
  editable = False
  hasTimeBuckets = True
  showOnlyFutureTimeBuckets = True
  maxBucketLevel = 3

  rows = (
    GridFieldText('buffer', title=_('buffer'), field_name="buffer", key=True, formatter='detail', extra="role:'input/buffer'", hidden=True),
    GridFieldText('item', title=_('item'), field_name="buffer__item__name", formatter='detail', extra="role:'input/item'"),
    GridFieldText('location', title=_('location'), field_name="buffer__location__name", formatter='detail', extra="role:'input/location'"),
    GridFieldInteger('leadtime', title=_('lead time'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('localforecast', title=_('local forecast'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('dependentdemand', title=_('dependent demand'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('safetystock', title=_('safety stock'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('reorderquantity', title=_('reorder quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('onhand', title=_('on hand'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('overduesalesorders', title=_('overdue sales orders'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('opensalesorders', title=_('open sales orders'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('openpurchases', title=_('open purchases'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('opentransfers', title=_('open transfers'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('proposedpurchases', title=_('proposed purchases'), extra="formatoptions:{defaultValue:''}"),
    GridFieldInteger('proposedtransfers', title=_('proposed transfers'), extra="formatoptions:{defaultValue:''}"),
    )


  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    return {
      'openbravo': 'freppledb.openbravo' in settings.INSTALLED_APPS
      }

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    return InventoryPlanningOutput.objects.all()

  @staticmethod
  def query(request, basequery):

    cursor = connections[request.database].cursor()
    basesql, baseparams = basequery.query.get_compiler(basequery.db).as_sql(with_col_aliases=False)
    sortsql = DRP._apply_sort_index(request, prefs=request.prefs)

    # Value or units
    if request.prefs and request.prefs.get('units', 'unit') == 'value':
      suffix = 'value'
    else:
      suffix = ''

    # Execute the query
    # TODO don't display buffers, but items and locations, and aggregate stuff
    # TODO add location and item attributes
    query = '''
      select
        buffer_id, item_id, location_id,
        extract(epoch from results.leadtime)/86400, localforecast%s,
        dependentdemand%s, safetystock%s, reorderquantity%s, results.onhand%s,
        overduesalesorders%s, opensalesorders%s, openpurchases%s, opentransfers%s,
        proposedpurchases%s, proposedtransfers%s
      from (%s) results
      inner join buffer
        on buffer.name = results.buffer_id
      order by %s
      ''' % (
        suffix, suffix, suffix, suffix, suffix,
        suffix, suffix, suffix, suffix, suffix,
        suffix, basesql, sortsql
        )
    cursor.execute(query, baseparams)

    # Build the python result
    for row in cursor.fetchall():
      yield {
        'buffer': row[0],
        'buffer__item__name': row[1],
        'buffer__location__name': row[2],
        'leadtime': row[3],
        'localforecast': row[4],
        'dependentdemand': row[5],
        'safetystock': row[6],
        'reorderquantity': row[7],
        'onhand': row[8],
        'overduesalesorders': row[9],
        'opensalesorders': row[10],
        'openpurchases': row[11],
        'opentransfers': row[12],
        'proposedpurchases': row[13],
        'proposedtransfers': row[14]
        }


class DRPitemlocation(View):

  buffer_type = None
  item_type = None
  location_type = None

  sql_oh = '''
    select out_flowplan.onhand, out_flowplan.onhand * item.price
    from out_flowplan
    inner join buffer
      on buffer.name = out_flowplan.thebuffer
    inner join item
      on buffer.item_id = item.name
    inner join (
      select max(id) as id
      from out_flowplan
      where flowdate < %s
      and thebuffer = %s
      ) maxid
    on maxid.id = out_flowplan.id
      and out_flowplan.thebuffer = %s
    '''

  sql_transactions = """
    select
      id, date, type, reference, status, quantity, quantity*item.price as value, startdate, enddate,
      item_id, location_id, supplier_id, criticality, transactions.lastmodified
    from
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
        enddate, item_id, destination_id, origin_id, criticality, lastmodified
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
    inner join item
      on transactions.item_id = item.name
    where status <> 'closed'
    order by date, quantity
    """

  sql_plan = """
    select d.bucket,
       avg(roq_calc.value%s) roq,
       avg(roq_override.value%s) roqoverride,
       avg(ss_calc.value%s) ss,
       avg(ss_override.value%s) ssoverride,
       -coalesce(sum(case
         when out_flowplan.quantity < 0 and purchase_order.status is null and distribution_order.status is null
           then out_flowplan.quantity%s
       end), 0) dmdlocal,
       -coalesce(sum(case
         when out_flowplan.quantity < 0 and (purchase_order.status is not null or distribution_order.status is not null)
           then out_flowplan.quantity%s
       end), 0) dmddependent,
       -coalesce(sum(least(out_flowplan.quantity%s, 0)),0.0) dmdtotal,
       coalesce(sum(case
         when out_flowplan.quantity > 0 and (purchase_order.status = 'confirmed' or distribution_order.status = 'confirmed')
           then out_flowplan.quantity%s
       end), 0) supplyconfirmed,
       coalesce(sum(case
         when out_flowplan.quantity > 0 and (purchase_order.status = 'proposed' or distribution_order.status = 'proposed')
           then out_flowplan.quantity%s
       end), 0) supplyproposed,
       coalesce(sum(greatest(out_flowplan.quantity%s,0)),0.0) supply
    from buffer
    inner join inventoryplanning
      on buffer.name = inventoryplanning.buffer_id
    inner join item
      on buffer.item_id = item.name
    left outer join out_inventoryplanning
      on buffer.name = out_inventoryplanning.buffer_id
    -- join buckets
    cross join (
           select name as bucket, startdate, enddate
           from common_bucketdetail
           where bucket_id = %%s and enddate > %%s and startdate < %%s
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
    -- join operationplan
    left outer join purchase_order
      on purchase_order.id = out_flowplan.operationplan_id
    left outer join distribution_order
      on distribution_order.id = out_flowplan.operationplan_id
    left outer join operationplan
      on operationplan.id = out_flowplan.operationplan_id
    where buffer.item_id = %%s and buffer.location_id = %%s
    group by d.bucket, d.startdate
    order by d.startdate
    """

  # Implicit assumption is that this filter picks up data from a single buffer.
  sql_forecast = """
    select d.bucket as bucket,
       coalesce(sum(forecastplan.orderstotal%s),0) as orderstotal,
       coalesce(sum(forecastplan.ordersopen%s),0) as ordersopen,
       sum(forecastplan.ordersadjustment%s) as ordersadjustment,
       coalesce(sum(forecastplan.forecastbaseline%s),0) as forecastbaseline,
       sum(forecastplan.forecastadjustment%s) as forecastadjustment,
       coalesce(sum(forecastplan.forecasttotal%s),0) as forecasttotal,
       coalesce(sum(forecastplan.forecastnet%s),0) as forecastnet,
       coalesce(sum(forecastplan.forecastconsumed%s),0) as forecastconsumed
    from forecast
    -- Join buckets
    cross join (
       select name as bucket, startdate, enddate
       from common_bucketdetail
       where bucket_id = %%s
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
    where locfilter.name = %%s
      and itemfilter.name = %%s
    -- Grouping
    group by d.bucket, d.startdate
    order by d.startdate
    """

  def getData(self, request, itemlocation):
    # This view retrieves all relevant data for a certain itemlocation.
    DRP.getBuckets(request)
    ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)
    item_name = ip.buffer.item.name if ip.buffer.item else None
    location_name = ip.buffer.location.name if ip.buffer.location else None

    # Display value or units?
    prefs = request.user.getPreference(DRP.getKey())
    if prefs and prefs.get('units', 'unit') == 'value':
      displayvalue = True
    else:
      displayvalue = False

    # Retrieve parameters
    fcst = Forecast.objects.using(request.database).filter(item=item_name, location=location_name).first()
    out_ip = InventoryPlanningOutput.objects.using(request.database).filter(pk=itemlocation).first()
    yield ''.join([
      '{"type":"itemlocation", "name":',  json.dumps(itemlocation), ",",
      '"displayvalue":', 'true' if displayvalue else 'false',
      ',"parameters":', json.dumps({
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
        "nostock": not(not ip.nostock) if ip.nostock is not None else None,
        "roq_max_poc": int(ip.roq_max_poc) if ip.roq_max_poc is not None else None,
        "service_level": str(ip.service_level) if ip.service_level is not None else None,
        "demand_deviation": int(ip.demand_deviation) if ip.demand_deviation is not None else None,
        "ss_max_poc": int(ip.ss_max_poc) if ip.ss_max_poc is not None else None,
        "roq_calculated": int(out_ip.calculatedreorderquantity) if out_ip and out_ip.calculatedreorderquantity is not None else None,
        "ss_calculated": int(out_ip.calculatedsafetystock) if out_ip and out_ip.calculatedsafetystock is not None else None,
        "forecastmethod": fcst.method if fcst else 'automatic',
        "forecast_out_method": fcst.out_method,
        "forecast_out_smape": round(float(fcst.out_smape if fcst.out_smape else 0), 1)
        })
      ])

    cursor = connections[request.database].cursor()

    # Retrieve forecast data
    yield ',"forecast":['
    first = True
    if displayvalue:
      extra = 'value'
    else:
      extra = ''
    cursor.execute(
      self.sql_forecast % (extra, extra, extra, extra, extra, extra, extra, extra),
      (request.report_bucket, location_name, item_name)
      )
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
        'forecastconsumed': round(rec[8])
        })

    # Retrieve onhand at the start of the planning horizon
    cursor.execute(
      self.sql_oh,
      (request.report_startdate, itemlocation, itemlocation)
      )
    try:
      if displayvalue:
        startoh = cursor.fetchone()[1]
      else:
        startoh = cursor.fetchone()[0]
    except:
      startoh = 0
    endoh = startoh

    # Retrieve inventory plan
    yield '],"plan":['
    first = True
    if displayvalue:
      extra = '*item.price'
    else:
      extra = ''
    cursor.execute(
      self.sql_plan % (extra, extra, extra, extra, extra, extra, extra, extra, extra, extra),
      (request.report_bucket, request.report_startdate, request.report_enddate, item_name, location_name)
      )
    for rec in cursor.fetchall():
      if not first:
        yield ","
      else:
        first = False
      dmdtotal = round(rec[7]) if rec[7] is not None else None
      supply = round(rec[10]) if rec[10] is not None else None
      endoh += supply - dmdtotal
      yield json.dumps({
        'bucket': rec[0],
        'roq': round(rec[1]) if rec[1] is not None else None,
        'roqoverride': round(rec[2]) if rec[2] is not None else None,
        'ss': round(rec[3]) if rec[3] is not None else None,
        'ssoverride': round(rec[4]) if rec[4] is not None else None,
        'startoh': round(startoh),
        'dmdlocal': round(rec[5]) if rec[5] is not None else None,
        'dmddependent': round(rec[6]) if rec[6] is not None else None,
        'dmdtotal': dmdtotal,
        'supplyconfirmed': round(rec[8]) if rec[8] is not None else None,
        'supplyproposed': round(rec[9]) if rec[9] is not None else None,
        'supply': supply,
        'endoh': round(endoh)
        })
      startoh = endoh

    # Retrieve transactions
    yield '],"transactions":['
    first = True
    cursor.execute(
      self.sql_transactions,
      (item_name, location_name, item_name, location_name, item_name, location_name)
      )
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
        'value': str(rec[6]),
        'startdate': str(rec[7]),
        'enddate': str(rec[8]),
        'item': rec[9],
        'location': rec[10],
        'origin': rec[11],
        'criticality': str(rec[12]),
        'lastmodified': str(rec[13])
        })

    # Retrieve comments
    if self.buffer_type is None:
      self.buffer_type = ContentType.objects.get_for_model(Buffer)
      self.item_type = ContentType.objects.get_for_model(Item)
      self.location_type = ContentType.objects.get_for_model(Location)
      self.inventoryplanning_type = ContentType.objects.get_for_model(InventoryPlanning)
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
    yield '],"history":['

    # Retrieve history
    history = LogEntry.objects.using(request.database).filter(
      Q(content_type=self.buffer_type.id, object_id=ip.buffer.name)
      | Q(content_type=self.item_type.id, object_id=ip.buffer.item.name if ip.buffer.item else None)
      | Q(content_type=self.location_type.id, object_id=ip.buffer.location.name if ip.buffer.location else None)
      | Q(content_type=self.inventoryplanning_type.id, object_id=ip.buffer.name)
      ).order_by('-action_time')[:20]
    first = True
    for i in history:
      if first:
        first = False
      else:
        yield ","
      yield json.dumps({
        "user": "%s (%s)" % (i.user.username, i.user.get_full_name()),
        "object_id": i.object_id,
        "content_type": i.content_type.name,
        "change_message": i.change_message,
        "action_time": str(i.action_time)
        })
    yield ']}'

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
    errors = []

    if True: #try:
      # Unescape special characters in the argument, which is encoded django-admin style.
      itemlocation = unquote(arg)

      # Look up the relevant object
      ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)

      # Retrieve the posted data
      data = json.JSONDecoder().decode(request.read().decode(request.encoding or settings.DEFAULT_CHARSET))
      simulate = data.get('simulate', False)
      editvalue = data.get("editvalue", True)

      # Edits by value or by units?
      if editvalue:
        factor = ip.buffer.item.price
        if not factor:
          factor = 1.0
      else:
        factor = 1.0

      # Recompute the plan
      newplan = self.replan(ip, editvalue, data, request, simulate)
      if simulate:
        return JsonResponse(newplan)


      # Save all changes to the database
      if not simulate:
        with transaction.atomic(using=request.database):

          # Save the plan overrides
          if 'plan' in data:
            ip_calendar = None
            if not request.user.has_perm('input.change_calendarbucket'):
              errors.append(force_text(_('Permission denied')))
            else:
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
                    # Delete buckets in the date range
                    CalendarBucket.objects.using(request.database).filter(
                      calendar=roq_calendar,
                      startdate__gte=datetime.strptime(row['startdate'], '%Y-%m-%d'),
                      enddate__lte=datetime.strptime(row['enddate'], '%Y-%m-%d')
                      ).exclude(source='Inventory planning').delete()
                  else:
                    # Create or update buckets in the date range
                    try:
                      val = float(row['roqoverride']) / factor
                      if val < 0:
                        errors.append(force_text(_('Invalid number')))
                        continue
                    except ValueError:
                      errors.append(force_text(_('Invalid number')))
                      continue
                    if not ip_calendar:
                      ip_calendar = Parameter.getValue('inventoryplanning.calendar', request.database)
                    bckts = CalendarBucket.objects.using(request.database).filter(
                      calendar__name=ip_calendar,
                      startdate__gte=datetime.strptime(row['startdate'], '%Y-%m-%d'),
                      enddate__lte=datetime.strptime(row['enddate'], '%Y-%m-%d')
                      )
                    for bckt in bckts:
                      cal_bucket, created = CalendarBucket.objects.using(request.database).get_or_create(
                        calendar=roq_calendar,
                        startdate=bckt.startdate,
                        enddate=bckt.enddate,
                        source=None
                        )
                      cal_bucket.value = val
                      cal_bucket.priority = 0
                      cal_bucket.save(using=request.database)
                if 'ssoverride' in row:
                  if not ss_calendar:
                    ss_calendar, created = Calendar.objects.using(request.database).get_or_create(name="SS for %s" % itemlocation)
                    if created:
                      ss_calendar.source = 'Inventory planning'
                      ss_calendar.default = 1
                      ss_calendar.save(using=request.database)
                  if row['ssoverride'] == '':
                    # Delete buckets in the date range
                    CalendarBucket.objects.using(request.database).filter(
                      calendar=ss_calendar,
                      startdate__gte=datetime.strptime(row['startdate'], '%Y-%m-%d'),
                      enddate__lte=datetime.strptime(row['enddate'], '%Y-%m-%d')
                      ).exclude(source='Inventory planning').delete()
                  else:
                    # Create or update buckets in the date range
                    try:
                      val = float(row['ssoverride']) / factor
                      if val < 0:
                        errors.append(force_text(_('Invalid number')))
                        continue
                    except ValueError:
                      errors.append(force_text(_('Invalid number')))
                      continue
                    if not ip_calendar:
                      ip_calendar = Parameter.getValue('inventoryplanning.calendar', request.database)
                    bckts = CalendarBucket.objects.using(request.database).filter(
                      calendar__name=ip_calendar,
                      startdate__gte=datetime.strptime(row['startdate'], '%Y-%m-%d'),
                      enddate__lte=datetime.strptime(row['enddate'], '%Y-%m-%d')
                      )
                    for bckt in bckts:
                      cal_bucket, created = CalendarBucket.objects.using(request.database).get_or_create(
                        calendar=ss_calendar,
                        startdate=bckt.startdate,
                        enddate=bckt.enddate,
                        source=None
                        )
                      cal_bucket.value = val
                      cal_bucket.priority = 0
                      cal_bucket.save(using=request.database)

          # Save the forecast overrides
          if 'forecast' in data:
            if not request.user.has_perm('forecast.change_forecastdemand'):
              errors.append(force_text(_('Permission denied')))
            else:
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
                    Decimal(row['adjHistory3']) / Decimal(factor) if (row.get('adjHistory3','') != '') else None,
                    True,  # Units
                    request.database
                    )
                elif 'adjHistory2' in row:
                  fcst.updatePlan(
                    strt.replace(year=strt.year - 2),
                    nd.replace(year=nd.year - 2),
                    None,
                    Decimal(row['adjHistory2']) / Decimal(factor) if (row.get('adjHistory2','') != '') else None,
                    True,  # Units
                    request.database
                    )
                elif 'adjHistory1' in row:
                  fcst.updatePlan(
                    strt.replace(year=strt.year - 1),
                    nd.replace(year=nd.year - 1),
                    None,
                    Decimal(row['adjHistory1']) / Decimal(factor) if (row.get('adjHistory1','') != '') else None,
                    True,  # Units
                    request.database
                    )
                elif 'adjForecast' in row:
                  fcst.updatePlan(
                    datetime.strptime(row['startdate'], '%Y-%m-%d').date(),
                    datetime.strptime(row['enddate'], '%Y-%m-%d').date(),
                    Decimal(row['adjForecast']) / Decimal(factor) if (row.get('adjForecast','') != '') else None,
                    None,
                    True,  # Units
                    request.database
                    )

          # Save the inventory parameters
          # TODO better error handling using a modelform
          if 'parameters' in data:
            param = data['parameters']
            if request.user.has_perm('forecast.change_forecast'):
              val = param.get('forecastmethod', '').lower()
              if val != '':
                fcst = Forecast.objects.all().using(request.database).get(item=ip.buffer.item, location=ip.buffer.location)
                fcst.method = val
                fcst.save(using=request.database)
            if request.user.has_perm('inventoryplanning.change_inventoryplanning'):
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

          # Save transactions
          po_form = None
          do_form = None
          if 'transactions' in data:
            for row in data['transactions']:
              transactiontype = row.get('type', '')
              if transactiontype == 'PO':
                if not request.user.has_perm('input.change_purchaseorder'):
                  errors.append(force_text(_('Permission denied')))
                  continue
                content_type_id = ContentType.objects.get_for_model(PurchaseOrder).pk
                obj = PurchaseOrder.objects.using(request.database).get(pk=row['id'])
                if not po_form:
                  po_form = modelform_factory(PurchaseOrder,
                    fields=(
                      'reference', 'status', 'quantity', 'supplier',
                      'startdate', 'enddate', 'item'
                      ),
                    formfield_callback=lambda f: (isinstance(f, RelatedField) and f.formfield(using=request.database)) or f.formfield()
                    )
                form = po_form({
                  'reference': row['reference'],
                  'status': row['status'],
                  'quantity': row['quantity'],
                  'supplier': row['origin'],
                  'startdate': row['startdate'],
                  'enddate': row['enddate'],
                  'item': row['item'],
                  }, instance=obj)
              elif transactiontype in ('DO out', 'DO in'):
                if not request.user.has_perm('input.change_distributionorder'):
                  errors.append(force_text(_('Permission denied')))
                  continue
                content_type_id = ContentType.objects.get_for_model(DistributionOrder).pk
                if not do_form:
                  do_form = modelform_factory(DistributionOrder,
                    fields=(
                      'reference', 'status', 'quantity', 'origin',
                      'startdate', 'enddate', 'item'
                      ),
                    formfield_callback=lambda f: (isinstance(f, RelatedField) and f.formfield(using=request.database)) or f.formfield()
                    )
                obj = DistributionOrder.objects.using(request.database).get(pk=row['id'])
                form = do_form({
                  'reference': row['reference'],
                  'status': row['status'],
                  'quantity': row['quantity'],
                  'origin': row['origin'],
                  'startdate': row['startdate'],
                  'enddate': row['enddate'],
                  'item': row['item'],
                  }, instance=obj)
              else:
                errors.append("Invalid transaction type: '%s'" % transactiontype)
              if not form.is_valid():
                errors.extend([e for e in form.non_field_errors()])
                for field in form:
                  errors.extend(["%s : %s" % (field.name, e) for e in field.errors ])
              elif form.has_changed():
                obj = form.save(commit=False)
                obj.quantity = abs(obj.quantity)
                obj.save(using=request.database)
                LogEntry(
                  user_id=request.user.pk,
                  content_type_id=content_type_id,
                  object_id=obj.pk,
                  object_repr=force_text(obj),
                  action_flag=CHANGE,
                  change_message=_('Changed %s.') % get_text_list(form.changed_data, _('and'))
                  ).save(using=request.database)

          # Save the comment
          if 'commenttype' in data and 'comment' in data:
            if not request.user.has_perm('common.add_comment'):
              errors.append(force_text(_('Permission denied')))
            elif data['commenttype'] == 'item' and ip.buffer.item:
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
              errors.append("Invalid comment data")

    #except Exception as e:
    #  errors.append(str(e))

    if errors:
      logger.error("Error saving DRP updates: %s" % "".join(errors))
      return HttpResponseServerError('Error saving DRP updates: %s' % "<br/>".join(errors))
    else:
      return HttpResponse(content="OK")


  def replan(self, ip, editvalue, changes, request, simulate):
    '''
    Recompute the inventory profile for a certain buffer on the fly
    using a new set of parameters.
    '''
    # Load the frepple extension module in our web server process
    import frepple

    itemlocation = ip.buffer.name
    result = {
      "type": "itemlocation",
      "name": itemlocation,
      "displayvalue": editvalue,
      "parameters": {},
      "plan": [],
      "transactions": [],
      "forecast": []
      }

    if editvalue:
      factor = ip.buffer.item.price
      if not factor:
        factor = 1.0
    else:
      factor = 1.0

    # Initialize the frepple module, and all objects we need to keep across simulations
    replanner = replanners.get(request.database)
    if not replanner:
      replanners[request.database] = replanner = Replanner(request.database)

    # Build a small model in memory to replan
    frepple.settings.current = replanner.current_date
    tmp = '%s simulating %s at %s' % (request.database, itemlocation, datetime.now())
    db_buf = Buffer.objects.all().using(request.database).get(name=itemlocation)
    frepple_location = frepple.location(name=ip.buffer.location.name)
    frepple_item = frepple.item(
      name=tmp,
      price=ip.buffer.item.price
      )
    frepple_buf = frepple.buffer(
      name=tmp,
      onhand=db_buf.onhand,
      item=frepple_item,
      location=frepple_location
      )
    for i in ItemSupplier.objects.all().using(request.database).filter(item=ip.buffer.item.name):
      frepple_itemsupplier = frepple.itemsupplier(
        supplier=frepple.supplier(name=i.supplier.name),
        location=frepple.location(name=i.location.name) if i.location else None,
        item=frepple_item,
        leadtime=i.leadtime,
        )
      if i.sizeminimum:
        frepple_itemsupplier.size_minimum = i.sizeminimum
      if i.sizemultiple:
        frepple_itemsupplier.size_multiple = i.sizemultiple
      if i.priority:
        frepple_itemsupplier.priority = i.priority
      if i.effective_start:
        frepple_itemsupplier.effective_start = i.effective_start
      if i.effective_end:
        frepple_itemsupplier.effective_end = i.effective_end
    for i in ItemDistribution.objects.all().using(request.database).filter(item=ip.buffer.item.name):
      frepple_itemdistribution = frepple.itemdistribution(
        origin=frepple.location(name=i.origin.name),
        destination=frepple.location(name=i.location.name) if i.location else None,
        item=frepple_item,
        leadtime=i.leadtime,
        )
      if i.sizeminimum:
        frepple_itemdistribution.size_minimum = i.sizeminimum
      if i.sizemultiple:
        frepple_itemdistribution.size_multiple = i.sizemultiple
      if i.priority:
        frepple_itemdistribution.priority = i.priority
      if i.effective_start:
        frepple_itemdistribution.effective_start = i.effective_start
      if i.effective_end:
        frepple_itemdistribution.effective_end = i.effective_end
    frepple_depdemand_oper = frepple.operation_fixed_time(
      name="%s dependent demand" % tmp
      )
    frepple_locdemand_oper = frepple.operation_fixed_time(
      name="%s local demand" % tmp
      )
    frepple.flow(operation=frepple_depdemand_oper, buffer=frepple_buf, quantity=-1, type="flow_start")
    frepple.flow(operation=frepple_locdemand_oper, buffer=frepple_buf, quantity=-1, type="flow_start")

    # Load the open sales orders
    idx = 1
    for dmd in Demand.objects.all().using(request.database).filter(
      item=ip.buffer.item.name, location=ip.buffer.location.name, status='open'
      ):
        frepple_demand = frepple.demand(
          name="%s %s" % (tmp, idx),
          item=frepple_item,
          location=frepple_location,
          due=dmd.due,
          quantity=dmd.quantity,
          priority=dmd.priority
          )
        if dmd.minshipment:
          frepple_demand.minshipment = dmd.minshipment
        if dmd.maxlateness is not None:
          frepple_demand.maxlateness = dmd.maxlateness
        idx += 1

    # Create forecast model
    db_forecast = Forecast.objects.all().using(request.database).get(name=itemlocation)
    frepple_forecast = frepple.demand_forecast(
      name=tmp,
      item=frepple_item,
      location=frepple_location
      )
    if 'parameters' in changes:
      changed_method = changes['parameters'].get('forecastmethod', None)
      if changed_method:
        db_forecast.method = changed_method
    if db_forecast.method == 'constant':
      frepple_forecast.methods = 1
    elif db_forecast.method == 'trend':
      frepple_forecast.methods = 2
    elif db_forecast.method == 'seasonal':
      frepple_forecast.methods = 4
    elif db_forecast.method == 'intermittent':
      frepple_forecast.methods = 8
    elif db_forecast.method == 'moving average':
      frepple_forecast.methods = 16
    elif db_forecast.method == 'manual':
      frepple_forecast.methods = 0
    frepple_forecast.operation = frepple_locdemand_oper

    # Read sales history and forecast overrides from the database
    query = ForecastPlan.objects.all().using(request.database).filter(
      forecast__name=itemlocation, startdate__gte=replanner.horizon_history, startdate__lt=replanner.horizon_future
      ).order_by('startdate')
    db_forecastplan = [ i for i in query ]

    # Read open distribution orders
    for do in DistributionOrder.objects.all().using(request.database).filter(
      item=ip.buffer.item.name,
      destination_id=ip.buffer.location.name,
      status='confirmed'
      ):
        frepple.operation_itemdistribution.createOrder(
          destination=frepple.location(name=do.destination.name) if do.destination else None,
          id=do.id, reference=do.reference,
          item=frepple_item,
          origin=frepple.location(name=do.origin.name) if do.origin else None,
          quantity=do.quantity, start=do.startdate, end=do.enddate,
          consume_material=do.consume_material if do.consume_material != None else True,
          status=do.status, source=do.source
          )

    # Read open purchase orders
    for po in PurchaseOrder.objects.all().using(request.database).filter(
      item=ip.buffer.item.name,
      location_id=ip.buffer.location.name,
      status='confirmed'
      ):
        frepple.operation_itemsupplier.createOrder(
          location=frepple.location(name=po.location.name) if po.location else None,
          id=po.id, reference=po.reference,
          item=frepple_item,
          supplier=frepple.supplier(name=po.supplier.name) if po.supplier else None,
          quantity=po.quantity, start=po.startdate, end=po.enddate,
          status=po.status, source=po.source
          )

    # Apply history adjustments sent from the server
    if 'forecast' in changes:
      for row in changes['forecast']:
        strt = datetime.strptime(row['startdate'], '%Y-%m-%d').date()
        nd = datetime.strptime(row['enddate'], '%Y-%m-%d').date()
        if 'adjHistory3' in row:
          db_forecast.updatePlan(
            strt.replace(year=strt.year - 3),
            nd.replace(year=nd.year - 3),
            None,
            Decimal(row['adjHistory3']) / Decimal(factor) if (row.get('adjHistory3','') != '') else None,
            True,  # Units
            request.database,
            db_forecastplan,  # Pass forecastplan nodes to update
            False             # Do not save the results
            )
        elif 'adjHistory2' in row:
          db_forecast.updatePlan(
            strt.replace(year=strt.year - 2),
            nd.replace(year=nd.year - 2),
            None,
            Decimal(row['adjHistory2']) / Decimal(factor) if (row.get('adjHistory2','') != '') else None,
            True,  # Units
            request.database,
            db_forecastplan,  # Pass forecastplan nodes to update
            False             # Do not save the results
            )
        elif 'adjHistory1' in row:
          db_forecast.updatePlan(
            strt.replace(year=strt.year - 1),
            nd.replace(year=nd.year - 1),
            None,
            Decimal(row['adjHistory1']) / Decimal(factor) if (row.get('adjHistory1','') != '') else None,
            True,  # Units
            request.database,
            db_forecastplan,  # Pass forecastplan nodes to update
            False             # Do not save the results
            )
        elif 'adjForecast' in row:
          db_forecast.updatePlan(
            strt,
            nd,
            Decimal(row['adjForecast']) / Decimal(factor) if (row.get('adjForecast','') != '') else None,
            None,
            True,  # Units
            request.database,
            db_forecastplan,  # Pass forecastplan nodes to update
            False             # Do not save the results
            )

    if db_forecast.method != 'manual':
      # Generate the baseline forecast
      replanner.forecast_solver.timeseries(
        frepple_forecast,
        [
          (float(i.orderstotal) if i.orderstotal else 0) + (float(i.ordersadjustment) if i.ordersadjustment else 0)
          for i in db_forecastplan
          if i.startdate < frepple.settings.current
        ],
        replanner.fcst_buckets
        )

      # Copy results from frePPLe forecast model into db model
      for i in frepple_forecast.members:
        for j in db_forecastplan:
          if i.due >= j.startdate and i.due <= j.enddate:
            j.forecastbaseline = i.total
            break

      # Get the forecast metrics
      result['parameters']["forecast_out_smape"] = round(frepple_forecast.smape_error * 10000) / 100
      result['parameters']["forecast_out_method"] = frepple_forecast.method

    else:
      # Manual forecast

      # Reset the baseline forecast to 0
      for j in db_forecastplan:
        j.forecastbaseline = j.forecasttotal = 0

      # Get the forecast metrics
      result['parameters']["forecast_out_smape"] = 0
      result['parameters']["forecast_out_method"] = 'manual'

    # Apply the forecast overrides from the database or sent from the server
    for i in db_forecastplan:
      if i.forecastadjustment and i.startdate >= frepple.settings.current:
        frepple_forecast.setQuantity(i.forecastadjustment, i.startdate, i.startdate, False)

    # TODO Run forecast netting?

    # Read inventory planning parameters
    db_ip = InventoryPlanning.objects.all().using(request.database).get(pk=itemlocation)
    frepple_buf.ip_flag = True
    if db_ip.roq_min_qty:
      frepple_buf.roq_min_qty = db_ip.roq_min_qty
    if db_ip.roq_max_qty:
      frepple_buf.roq_max_qty = db_ip.roq_max_qty
    if db_ip.roq_multiple_qty:
      frepple_buf.roq_multiple_qty = db_ip.roq_multiple_qty
    if db_ip.roq_min_poc:
      frepple_buf.roq_min_poc = db_ip.roq_min_poc
    if db_ip.roq_max_poc:
      frepple_buf.roq_max_poc = db_ip.roq_max_poc
    if db_ip.leadtime_deviation:
      frepple_buf.leadtime_deviation = db_ip.leadtime_deviation
    if db_ip.demand_deviation:
      frepple_buf.demand_deviation = db_ip.demand_deviation
    if db_ip.demand_distribution:
      frepple_buf.demand_distribution = db_ip.demand_distribution
    if db_ip.service_level:
      frepple_buf.service_level = db_ip.service_level
    if db_ip.ss_min_qty:
      frepple_buf.ss_min_qty = db_ip.ss_min_qty
    if db_ip.ss_max_qty:
      frepple_buf.ss_max_qty = db_ip.ss_max_qty
    if db_ip.ss_multiple_qty:
      frepple_buf.ss_multiple_qty = db_ip.ss_multiple_qty
    if db_ip.ss_min_poc:
      frepple_buf.ss_min_poc = db_ip.ss_min_poc
    if db_ip.ss_max_poc:
      frepple_buf.ss_max_poc = db_ip.ss_max_poc
    if db_ip.nostock:
      frepple_buf.nostock = db_ip.nostock
    if db_ip.roq_type:
      frepple_buf.roq_type = db_ip.roq_type
    else:
      # The engine uses "combined" as default. We apply a different default here!
      frepple_buf.roq_type = "calculated"
    if db_ip.ss_type:
      frepple_buf.ss_type = db_ip.ss_type
    else:
      # The engine uses "combined" as default. We apply a different default here!
      frepple_buf.ss_type = "calculated"

    # Apply inventory planning changes sent from the client
    if 'parameters' in changes:
      val = changes['parameters'].get('roq_min_qty', None)
      if val:
        frepple_buf.roq_min_qty = val
      val = changes['parameters'].get('roq_max_qty', None)
      if val:
        frepple_buf.roq_max_qty = val
      val = changes['parameters'].get('roq_multiple_qty', None)
      if val:
        frepple_buf.roq_multiple_qty = val
      val = changes['parameters'].get('roq_min_poc', None)
      if val:
        frepple_buf.roq_min_poc = val
      val = changes['parameters'].get('roq_max_poc', None)
      if val:
        frepple_buf.roq_max_poc = val
      val = changes['parameters'].get('leadtime_deviation', None)
      if val:
        frepple_buf.leadtime_deviation = val
      val = changes['parameters'].get('demand_deviation', None)
      if val:
        frepple_buf.demand_deviation = val
      val = changes['parameters'].get('demand_distribution', None)
      if val:
        frepple_buf.demand_distribution = val
      val = changes['parameters'].get('service_level', None)
      if val:
        frepple_buf.service_level = val
      val = changes['parameters'].get('ss_min_qty', None)
      if val:
        frepple_buf.ss_min_qty = val
      val = changes['parameters'].get('ss_max_qty', None)
      if val:
        frepple_buf.ss_max_qty = val
      val = changes['parameters'].get('ss_multiple_qty', None)
      if val:
        frepple_buf.ss_multiple_qty = val
      val = changes['parameters'].get('ss_min_poc', None)
      if val:
        frepple_buf.ss_min_poc = val
      val = changes['parameters'].get('ss_max_poc', None)
      if val:
        frepple_buf.ss_max_poc = val
      val = changes['parameters'].get('nostock', None)
      if val:
        frepple_buf.nostock = val
      val = changes['parameters'].get('roq_type', None)
      if val:
        frepple_buf.roq_type = val
      val = changes['parameters'].get('ss_type', None)
      if val:
        frepple_buf.ss_type = val

    # Load inventory planning overrides from the database
    roq_cal = frepple.calendar(name="ROQ for %s" % tmp)
    ss_cal = frepple.calendar(name="SS for %s" % tmp)
    for db_cal in CalendarBucket.objects.all().using(request.database).filter(
      calendar="ROQ for %s" % ip.buffer.item.name
      ).exclude(source='Inventory planning'):
        bck = roq_cal.addBucket(db_cal.id)
        bck.end = db_cal.enddate
        bck.start = db_cal.startdate
        bck.value = db_cal.value
        bck.priority = db_cal.priority
        bck.source = db_cal.source
    for db_cal in CalendarBucket.objects.all().using(request.database).filter(
      calendar="SS for %s" % ip.buffer.item.name
      ).exclude(source='Inventory planning'):
        bck = ss_cal.addBucket(db_cal.id)
        bck.end = db_cal.enddate
        bck.start = db_cal.startdate
        bck.value = db_cal.value
        bck.priority = db_cal.priority
        bck.source = db_cal.source

    def getBucket(bcktlist, dt, ovr):
      found = None
      for bck in bcktlist:
        if dt >= bck.start and dt < bck.end:
          if ovr:
            if bck.source != 'Inventory planning' and (not found or found.priority > bck.priority):
              found = bck
          else:
            if bck.source == 'Inventory planning' and (not found or found.priority > bck.priority):
              found = bck
      bck = None
      return found.value if found else None

    # Apply inventory planning overrides received from the client
    if 'plan' in changes:
      for ovr in changes["plan"]:
        if 'roqoverride' in ovr:
          if ovr['roqoverride'] == '':
            pass # todo: delete existing entry
          else:
            bck = roq_cal.addBucket()
            bck.value = float(ovr['roqoverride'])
            bck.end = datetime.strptime(ovr['enddate'], '%Y-%m-%d')
            bck.start = datetime.strptime(ovr['startdate'], '%Y-%m-%d')
            bck.priority = -1
            bck.source = 'manual'
        if 'ssoverride' in ovr:
          if ovr['ssoverride'] == '':
            pass # todo: delete existing entry
          else:
            bck = ss_cal.addBucket()
            bck.value = float(ovr['ssoverride'])
            bck.end = datetime.strptime(ovr['enddate'], '%Y-%m-%d')
            bck.start = datetime.strptime(ovr['startdate'], '%Y-%m-%d')
            bck.priority = -1
            bck.source = 'manual'

    # Calculate safety stock and reorder quantity
    # TODO this solves for ALL buffers, not only the replanned one
    replanner.ip_solver.solve()

    # Create constrained plan
    replanner.mrp_solver.solve()

    # Collect inventory planning results
    result['parameters']["roq_calculated"] = round(frepple_buf.ip_calculated_roq)
    result['parameters']["ss_calculated"] = round(frepple_buf.ip_calculated_ss)
    roq_cal_buckets = [ i for i in roq_cal.buckets ]
    ss_cal_buckets = [ i for i in ss_cal.buckets ]

    # Copy results from frePPLe forecast model into db model
    for i in frepple_forecast.members:
      for j in db_forecastplan:
        if i.due >= j.startdate and i.due <= j.enddate:
          j.forecasttotal = i.total
          j.forecastnet = i.quantity
          j.forecastconsumed = i.consumed
          break

    # Collect the forecast results
    # We aggregate the low-level results into the requested reporting buckets
    agg_buckets_query = BucketDetail.objects.using(request.database).filter(
      bucket=request.user.horizonbuckets,
      startdate__gte=replanner.forecastplan_min,
      startdate__lte=replanner.forecastplan_max
      )
    agg_buckets = [ i for i in agg_buckets_query ]
    agg_bucket_idx = 0
    orderstotal = 0
    ordersopen = 0
    ordersadjustment = None
    forecastbaseline = 0
    forecastadjustment = None
    forecasttotal = 0
    forecastnet = 0
    forecastconsumed = 0
    for i in db_forecastplan:
      if i.startdate <= agg_buckets[agg_bucket_idx].startdate:
        if i.orderstotal:
          orderstotal += i.orderstotal
        if i.ordersopen:
          ordersopen += i.ordersopen
        if i.ordersadjustment:
          if ordersadjustment:
            ordersadjustment += i.ordersadjustment
          else:
            ordersadjustment = Decimal(i.ordersadjustment)
        if i.forecastbaseline:
          forecastbaseline += i.forecastbaseline
        if i.forecastadjustment:
          if forecastadjustment:
            forecastadjustment += i.forecastadjustment
          else:
            forecastadjustment = Decimal(i.forecastadjustment)
        if i.forecasttotal:
          forecasttotal += i.forecasttotal
        if i.forecastnet:
          forecastnet += i.forecastnet
        if i.forecastconsumed:
          forecastconsumed += i.forecastconsumed
      else:
        result["forecast"].append({
          'bucket': agg_buckets[agg_bucket_idx].name,
          'orderstotal': int(orderstotal),
          'ordersopen': int(ordersopen),
          'ordersadjustment': int(ordersadjustment) if ordersadjustment is not None else None,
          'forecastbaseline': int(forecastbaseline),
          'forecastadjustment': int(forecastadjustment) if forecastadjustment is not None else None,
          'forecasttotal': int(forecasttotal),
          'forecastnet': int(forecastnet),
          'forecastconsumed': int(forecastconsumed)
          })
        if i.orderstotal:
          orderstotal = i.orderstotal
        else:
          orderstotal = 0
        if i.ordersopen:
          ordersopen = i.ordersopen
        else:
          ordersopen = 0
        if i.ordersadjustment:
          ordersadjustment = i.ordersadjustment
        else:
          ordersadjustment = None
        if i.forecastbaseline:
          forecastbaseline = i.forecastbaseline
        else:
          forecastbaseline = 0
        if i.forecastadjustment:
          forecastadjustment = i.forecastadjustment
        else:
          forecastadjustment = None
        if i.forecasttotal:
          forecasttotal = i.forecasttotal
        else:
          forecasttotal = 0
        if i.forecastnet:
          forecastnet = i.forecastnet
        else:
          forecastnet = 0
        if i.forecastconsumed:
          forecastconsumed = i.forecastconsumed
        else:
          forecastconsumed = 0
        agg_bucket_idx += 1
      if agg_bucket_idx >= len(agg_buckets):
          break
    if agg_bucket_idx < len(agg_buckets):
      result["forecast"].append({
        'bucket': agg_buckets[agg_bucket_idx].name,
        'orderstotal': int(orderstotal),
        'ordersopen': int(ordersopen),  # TODO
        'ordersadjustment': int(ordersadjustment) if ordersadjustment is not None else None,
        'forecastbaseline': int(forecastbaseline),
        'forecastadjustment': int(forecastadjustment) if forecastadjustment is not None else None,
        'forecasttotal': int(forecasttotal),
        'forecastnet': int(forecastnet),
        'forecastconsumed': int(forecastconsumed)
        })

    # Retrieve inventory plan and transactions
    agg_bucket_idx = 0
    while agg_buckets[agg_bucket_idx].enddate <= replanner.current_date:
      agg_bucket_idx += 1
    start_oh = frepple_buf.onhand
    demand_local = 0
    demand_dependent = 0
    supply_confirmed = 0
    supply_proposed = 0
    end_oh = 0
    for fl in frepple_buf.flowplans:
      if fl.date < agg_buckets[agg_bucket_idx].startdate:
        continue
      if fl.date > agg_buckets[agg_bucket_idx].enddate:
        while False: #fl.date > agg_buckets[agg_bucket_idx+1].startdate:
          result["plan"].append({
            "bucket": agg_buckets[agg_bucket_idx].name,
            "startoh": start_oh,
            "dmdlocal": 0,
            "dmddependent": 0,
            "dmdtotal": 0,
            "supplyconfirmed": 0,
            "supplyproposed": 0,
            "supply": 0,
            "endoh": start_oh,
            "roq": getBucket(roq_cal_buckets, agg_buckets[agg_bucket_idx].startdate, False) or 1,
            "roqoverride": getBucket(roq_cal_buckets, agg_buckets[agg_bucket_idx].startdate, True),
            "ss": getBucket(ss_cal_buckets, agg_buckets[agg_bucket_idx].startdate, False) or 0,
            "ssoverride": getBucket(ss_cal_buckets, agg_buckets[agg_bucket_idx].startdate, True)
            })
          agg_bucket_idx += 1
          if agg_bucket_idx >= len(agg_buckets):
            break
        # Collect results of previous bucket
        end_oh = start_oh + supply_confirmed + supply_proposed - demand_local - demand_dependent
        result["plan"].append({
          "bucket": agg_buckets[agg_bucket_idx].name,
          "startoh": start_oh,
          "dmdlocal": demand_local,
          "dmddependent": demand_dependent,
          "dmdtotal": demand_local + demand_dependent,
          "supplyconfirmed": supply_confirmed,
          "supplyproposed": supply_proposed,
          "supply": supply_confirmed + supply_proposed,
          "endoh": end_oh,
          "roq": getBucket(roq_cal_buckets, agg_buckets[agg_bucket_idx].startdate, False) or 1,
          "roqoverride": getBucket(roq_cal_buckets, agg_buckets[agg_bucket_idx].startdate, True),
          "ss": getBucket(ss_cal_buckets, agg_buckets[agg_bucket_idx].startdate, False) or 0,
          "ssoverride": getBucket(ss_cal_buckets, agg_buckets[agg_bucket_idx].startdate, True)
          })
        # New bucket started
        start_oh = end_oh
        demand_local = demand_dependent = supply_confirmed = supply_proposed = 0
        agg_bucket_idx += 1
        if agg_bucket_idx >= len(agg_buckets):
          break
      if fl.quantity > 0:
        if fl.operationplan.status == 'confirmed':
          supply_confirmed += fl.quantity
        else:
          supply_proposed += fl.quantity
      else:
        if fl.operationplan.demand:
          demand_local -= fl.quantity
        else:
          demand_dependent -= fl.quantity
      frepple_operationplan = fl.operationplan
      if isinstance(frepple_operationplan.operation, frepple.operation_itemsupplier):
        result["transactions"].append({
          "criticality": frepple_operationplan.criticality, # TODO incremental calculation can give different value
          "date": str(frepple_operationplan.end),
          "startdate": str(frepple_operationplan.start),
          "enddate": str(frepple_operationplan.end),
          "id": frepple_operationplan.id, # TODO incremental calculation can give different value
          "item": ip.buffer.item.name,
          "location": fl.buffer.location.name,
          "origin": frepple_operationplan.operation.itemsupplier.supplier.name,
          "quantity": frepple_operationplan.quantity,
          "reference": frepple_operationplan.reference,
          "status": frepple_operationplan.status,
          "type": "PO",
          "value": frepple_operationplan.quantity * fl.buffer.item.price,
          "lastmodified": str(datetime.now())
          })
      elif isinstance(frepple_operationplan.operation, frepple.operation_itemdistribution):
        result["transactions"].append({
          "criticality": frepple_operationplan.criticality, # TODO incremental calculation can give different value
          "date": str(frepple_operationplan.end if fl.quantity > 0 else frepple_operationplan.start),
          "startdate": str(frepple_operationplan.start),
          "enddate": str(frepple_operationplan.end),
          "id": frepple_operationplan.id, # TODO incremental calculation can give different value
          "item": ip.buffer.item.name,
          "location": fl.buffer.location.name,
          "origin": frepple_operationplan.operation.origin.location.name,
          "quantity": frepple_operationplan.quantity,
          "reference": frepple_operationplan.reference,
          "status": frepple_operationplan.status,
          "type": "DO in" if fl.quantity > 0 else "DO out",
          "value": frepple_operationplan.quantity * fl.buffer.item.price,
          "lastmodified": str(datetime.now())
          })
    # Empty buckets at the end of the plan
    while agg_bucket_idx < len(agg_buckets):
      result["plan"].append({
        "bucket": agg_buckets[agg_bucket_idx].name,
        "startoh": start_oh,
        "dmdlocal": 0,
        "dmddependent": 0,
        "dmdtotal": 0,
        "supplyconfirmed": 0,
        "supplyproposed": 0,
        "supply": 0,
        "endoh": start_oh,
        "roq": getBucket(roq_cal_buckets, agg_buckets[agg_bucket_idx].startdate, False),
        "roqoverride": getBucket(roq_cal_buckets, agg_buckets[agg_bucket_idx].startdate, True),
        "ss": getBucket(ss_cal_buckets, agg_buckets[agg_bucket_idx].startdate, False),
        "ssoverride": getBucket(ss_cal_buckets, agg_buckets[agg_bucket_idx].startdate, True)
        })
      agg_bucket_idx += 1
    # Don't send empty plans back (and the client browser will continue just
    # to display the previous plan).
    if not result["plan"]:
      del result["plan"]
    if not result["transactions"]:
      del result["transactions"]

    # Cleaning up
    # We remove any references to frePPLe objects in Python, before
    # calling the remove function in frePPLe. This avoids trouble with the
    # Python garbage collector and avoids warnings from frePPLe.
    frepple_forecast = frepple_buf = frepple_depdemand_oper = None
    frepple_item = frepple_locdemand_oper = roq_cal = ss_cal = None
    roq_cal_buckets = ss_cal_buckets = frepple_itemsupplier = None
    frepple_itemdistribution = frepple_demand = frepple_location = None
    frepple_operationplan = i = j = fl = None
    gc.collect()
    frepple.demand(name=tmp, action='R')
    frepple.buffer(name=tmp, action='R')
    frepple.calendar(name="ROQ for %s" % tmp, action='R')
    frepple.calendar(name="SS for %s" % tmp, action='R')
    frepple.operation(name="%s dependent demand" % tmp, action='R')
    frepple.operation(name="%s local demand" % tmp, action='R')
    frepple.item(name=tmp, action='R')

    # Return the new plan
    return result


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
