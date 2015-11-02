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
import inspect

from django.conf import settings
from django.contrib.admin.utils import unquote
from django.db import connections
from django.http.response import StreamingHttpResponse
from django.http import JsonResponse
from freppledb.common.models import Parameter
from freppledb.forecast.commands import createSolver
from freppledb.forecast.models import Forecast, ForecastPlan
from freppledb.input.models import Buffer, Calendar, CalendarBucket
from freppledb.input.models import Item, DistributionOrder, PurchaseOrder
from freppledb.inventoryplanning.models import InventoryPlanning, InventoryPlanningOutput


# Load the frepple extension module in our web server process
import frepple


import logging
logger = logging.getLogger(__name__)



class Replanner:
  def __init__(self, db):
    if 'demand_forecast' not in [ a[0] for a in inspect.getmembers(frepple) ]:
      print("==== Loading1")
      frepple.loadmodule('c:\\develop\\frepple.enterprise\\bin\\mod_forecast.so')     # TODO file location
      print("==== Loading2")
      frepple.loadmodule('c:\\develop\\frepple.enterprise\\bin\\mod_inventoryplanning.so')

    # TODO load all parameters from the database
    # TODO common objects should have names that specific to the database
    self.frepple_calendar = frepple.calendar(
      name=Parameter.getValue('forecast.calendar', db, None),
      default=0
      )
    try:
      frepple.settings.current = datetime.strptime(
        Parameter.objects.using(db).get(name="currentdate").value,
        "%Y-%m-%d %H:%M:%S"
        )
    except:
      frepple.settings.current = datetime.now()
    self.horizon_history = frepple.settings.current - timedelta(days=int(Parameter.getValue('forecast.Horizon_history', db, 10000)))
    self.horizon_future = frepple.settings.current + timedelta(days=int(Parameter.getValue('forecast.Horizon_future', db, 365)))
    self.fcst_buckets = []
    for bckt in CalendarBucket.objects.all().using(db).filter(calendar__name=self.frepple_calendar.name).order_by('startdate'):
      frepple_bckt = self.frepple_calendar.addBucket(bckt.id)
      frepple_bckt.start = bckt.startdate
      frepple_bckt.end = bckt.enddate
      frepple_bckt.value = bckt.value
      if bckt.startdate >= frepple.settings.current and bckt.startdate < self.horizon_future:
        self.fcst_buckets.append(bckt.startdate)

    # Solvers
    cursor = connections[db].cursor()
    self.forecast_solver = createSolver(cursor)
    self.mrp_solver = frepple.solver_mrp(
      constraints=15,
      plantype=2,
      loglevel=int(Parameter.getValue('plan.loglevel', db, 0)),
      lazydelay=int(Parameter.getValue('lazydelay', db, '86400')),
      allowsplits=(Parameter.getValue('allowsplits', db, 'true') == "true"),
      rotateresources=(Parameter.getValue('plan.rotateResources', db, 'true') == "true"),
      plansafetystockfirst=(Parameter.getValue('plan.planSafetyStockFirst', db, 'false') == "false"),
      iterationmax=int(Parameter.getValue('plan.iterationmax', db, '0'))
      )


# Dictionary with replanning modules
replanners = {}


def ReplanItemLocation(request, arg):
  '''
  Recompute the inventory profile for a certain locationpart on the fly
  using a new set of parameters.
  '''
  # Only accept ajax requests on this URL
  #if not request.is_ajax() or not request.method() != 'POST':
  #  raise Http404('Only ajax post requests allowed')
  result = {}

  # Unescape special characters in the argument, which is encoded django-admin style.
  itemlocation = unquote(arg)

  # Initialize the frepple module, and all objects we need to keep across simulations
  replanner = replanners.get(request.database)
  if not replanner:
    replanners[request.database] = replanner = Replanner(request.database)

  # Build a small model in memory to replan
  tmp = 'simulating %s at %s' % (itemlocation, datetime.now())
  db_buf = Buffer.objects.all().using(request.database).get(name=itemlocation)
  frepple_buf = frepple.buffer(
    name=tmp,
    onhand=db_buf.onhand
    )
  frepple_supply_oper = frepple.operation_fixed_time(
    name="%s supply" % tmp
    )
  frepple_depdemand_oper = frepple.operation_fixed_time(
    name="%s dependent demand" % tmp
    )
  frepple_locdemand_oper = frepple.operation_fixed_time(
    name="%s local demand" % tmp
    )
  frepple.flow(operation=frepple_supply_oper, buffer=frepple_buf, quantity=1, type="flow_end")
  frepple.flow(operation=frepple_depdemand_oper, buffer=frepple_buf, quantity=-1, type="flow_start")
  frepple.flow(operation=frepple_locdemand_oper, buffer=frepple_buf, quantity=-1, type="flow_start")

  # Create forecast model
  db_forecast = Forecast.objects.all().using(request.database).get(name=itemlocation)
  frepple_forecast = frepple.demand_forecast(name=tmp)
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

  # Recalculate the baseline forecast
  query = ForecastPlan.objects.all().using(request.database).filter(forecast__name=itemlocation, startdate__gte=replanner.horizon_history, startdate__lt=frepple.settings.current).order_by('startdate')
  replanner.forecast_solver.timeseries(
    frepple_forecast,
    [ (float(i.orderstotal) if i.orderstotal else 0) + (float(i.ordersadjustment) if i.ordersadjustment else 0) for i in query],
    replanner.fcst_buckets
    )

  # Get the forecast metrics
  result["forecasterror"] = frepple_forecast.smape_error * 100
  result["method"] = frepple_forecast.method
  result["deviation"] = frepple_forecast.deviation

  # Apply the forecast overrides
  query = ForecastPlan.objects.all().using(request.database).filter(forecast__name=itemlocation, enddate__gte=frepple.settings.current, startdate__lte=replanner.horizon_future).order_by('startdate')
  for i in query:
    if i.forecastadjustment:
      frepple_forecast.setQuantity(i.forecastadjustment, i.startdate, i.startdate, False)

  # calculate the baseline forecast

  # load buffer
  # load roq and ss calendar
  # artificially recreate demand from db
  # load confirmed supply
  # load parameters from db
  # load forecast

  # Stream back the response
  return JsonResponse(result)

