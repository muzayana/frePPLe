import inspect
import os
from datetime import datetime, timedelta
from time import time

from django.db import connections, transaction, DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.common.models import Parameter
from freppledb.input.models import Item, Customer
from freppledb.execute.commands import printWelcome, logProgress, createPlan, exportPlan

import frepple


def loadForecast(cursor):
  print('Importing forecast...')
  cnt = 0
  starttime = time()
  cursor.execute('''SELECT name, customer_id, item_id, priority,
    operation_id, minshipment, calendar_id, discrete, maxlateness,
    category, subcategory, method, planned
    FROM forecast''')
  for i in cursor.fetchall():
    cnt += 1
    fcst = frepple.demand_forecast(name=i[0], priority=i[3], category=i[9], subcategory=i[10])
    if i[1]:
      fcst.customer = frepple.customer(name=i[1])
    if i[2]:
      fcst.item = frepple.item(name=i[2])
    if i[4]:
      fcst.operation = frepple.operation(name=i[4])
    if i[5]:
      fcst.minshipment = i[5]
    if i[6]:
      fcst.calendar = frepple.calendar(name=i[6])
    if not i[7]:
      fcst.discrete = False  # null value -> False
    if i[8] is not None:
      fcst.maxlateness = i[8]
    if i[11]:
      if i[11] == 'constant':
        fcst.methods = 1
      elif i[11] == 'trend':
        fcst.methods = 2
      elif i[11] == 'seasonal':
        fcst.methods = 4
      elif i[11] == 'intermittent':
        fcst.methods = 8
      elif i[11] == 'moving average':
        fcst.methods = 16
      elif i[11] == 'manual':
        fcst.methods = 0
    if i[12] is not None and not i[12]:
      fcst.planned = False  # null value -> True
  print('Loaded %d forecasts in %.2f seconds' % (cnt, time() - starttime))


def aggregateDemand(cursor):
  # Aggregate demand history
  starttime = time()
  cursor.execute('''
     update forecastplan
     set orderstotal=0, orderstotalvalue=0, ordersopen=0, ordersopenvalue=0
     where orderstotal <> 0 or ordersopen <> 0 or orderstotalvalue<>0 or ordersopenvalue<>0
     ''')
  transaction.commit(using=cursor.db.alias)
  print('Aggregate - reset records in %.2f seconds' % (time() - starttime))

  # Create a temp table with the aggregated demand
  starttime = time()
  cursor.execute('''
     create temp table demand_history
     on commit preserve rows
     as
      select forecast.name as forecast,
        calendarbucket.startdate as startdate,
        calendarbucket.enddate as enddate,
        sum(demand.quantity) as orderstotal,
        sum(case when demand.status is null or demand.status = 'open' then demand.quantity else 0 end) as ordersopen,
        coalesce(sum(demand.quantity*ditem.price), 0) as orderstotalvalue,
        coalesce(sum(case when demand.status is null or demand.status = 'open' then (demand.quantity*ditem.price) else 0 end), 0) as ordersopenvalue
      from demand
      inner join item as ditem on demand.item_id = ditem.name
      inner join customer as dcustomer on demand.customer_id = dcustomer.name
      inner join item as fitem on ditem.lft between fitem.lft and fitem.rght
      inner join customer as fcustomer on dcustomer.lft between fcustomer.lft and fcustomer.rght
      inner join forecast on fitem.name = forecast.item_id and fcustomer.name = forecast.customer_id
      inner join calendarbucket
        on forecast.calendar_id = calendarbucket.calendar_id
        and calendarbucket.startdate <= demand.due
        and calendarbucket.enddate > demand.due
      where demand.status in ('open','closed')
      group by forecast.name, calendarbucket.startdate, calendarbucket.enddate
     ''')
  cursor.execute('''CREATE UNIQUE INDEX demand_history_idx ON demand_history (forecast, startdate)''')
  print('Aggregate - temp table in %.2f seconds' % (time() - starttime))

  # Create all active history pairs
  starttime = time()
  cursor.execute('''
    insert into forecastplan (
      forecast_id, startdate, enddate, orderstotal, ordersopen,
      forecastbaseline, forecasttotal, forecastnet, forecastconsumed,
      ordersplanned, forecastplanned, orderstotalvalue,
      ordersopenvalue, ordersplannedvalue, forecastbaselinevalue,
      forecasttotalvalue, forecastnetvalue, forecastconsumedvalue, forecastplannedvalue
      )
    select
       demand_history.forecast, demand_history.startdate, demand_history.enddate,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    from demand_history
    left outer join forecastplan
      on demand_history.forecast = forecastplan.forecast_id
      and demand_history.startdate = forecastplan.startdate
    where forecastplan.forecast_id is null
    ''')
  print('Aggregate - init past records in %.2f seconds' % (time() - starttime))

  # Merge aggregate demand history into the forecastplan table
  starttime = time()
  cursor.execute('''update forecastplan
    set orderstotal=demand_history.orderstotal, orderstotalvalue=demand_history.orderstotalvalue,
      ordersopen=demand_history.ordersopen, ordersopenvalue=demand_history.ordersopenvalue
    from demand_history
    where forecastplan.forecast_id = demand_history.forecast
      and forecastplan.startdate = demand_history.startdate
      and (
        forecastplan.orderstotal <> demand_history.orderstotal
        or forecastplan.ordersopen <> demand_history.ordersopen
        or forecastplan.orderstotalvalue <> demand_history.orderstotalvalue
        or forecastplan.ordersopenvalue <> demand_history.ordersopenvalue
        )
    ''')
  transaction.commit(using=cursor.db.alias)
  cursor.execute("drop table demand_history")
  print('Aggregate - update order records in %.2f seconds' % (time() - starttime))

  # Initialize all buckets in the past and future
  starttime = time()
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  horizon_history = int(Parameter.getValue('forecast.Horizon_history', cursor.db.alias, 10000))
  cursor.execute('''
    insert into forecastplan (
        forecast_id, startdate, enddate, orderstotal, ordersopen,
        forecastbaseline, forecasttotal, forecastnet, forecastconsumed, ordersplanned,
        forecastplanned, orderstotalvalue, ordersopenvalue, ordersplannedvalue,
        forecastbaselinevalue, forecasttotalvalue, forecastnetvalue, forecastconsumedvalue,
        forecastplannedvalue
        )
      select
        forecast.name, calendarbucket.startdate, calendarbucket.enddate,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
      from forecast
      inner join item on forecast.item_id = item.name
      inner join customer on forecast.customer_id = customer.name
      inner join calendarbucket
        on forecast.calendar_id = calendarbucket.calendar_id
        and calendarbucket.startdate >= '%s'
        and calendarbucket.startdate < '%s'
      left outer join forecastplan
        on forecastplan.startdate = calendarbucket.startdate
        and forecastplan.forecast_id = forecast.name
      where forecastplan.forecast_id is null
      group by forecast.name, customer.lft, item.lft,
        calendarbucket.startdate, calendarbucket.enddate
    ''' % (
      frepple.settings.current - timedelta(days=horizon_history),
      frepple.settings.current + timedelta(days=horizon_future)
      )
  )
  print('Aggregate - init future records in %.2f seconds' % (time() - starttime))


def processForecastDemand(cursor):
  from freppledb.forecast.models import ForecastPlan

  # Process all records from the forecastdemand table, by moving the
  # data to the forecastplan table.
  # After processing delete the records from the forecastdemand table.
  cursor.execute('''select forecast_id, startdate, enddate, quantity
     from forecastdemand
     order by id
     ''')
  for fcstname, start, end, qty in cursor.fetchall():
    fcsts = ForecastPlan.objects.all().using(cursor.db.alias).filter(forecast__name=fcstname, startdate__gte=start, startdate__lt=end)
    cnt = fcsts.count()
    if cnt:
      fcsts.update(forecastadjustment=qty / cnt)
  cursor.execute('delete from forecastdemand')


def generateBaseline(solver_fcst, cursor):
  data = []
  curfcst = None

  # Build bucket lists
  horizon_history = int(Parameter.getValue('forecast.Horizon_history', cursor.db.alias, 10000))
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  thebuckets = {}
  cursor.execute('''select calendarbucket.calendar_id, startdate
     from calendarbucket
     where calendarbucket.calendar_id in (select distinct forecast.calendar_id  from forecast)
       and startdate >= '%s' and startdate < '%s'
     order by calendarbucket.calendar_id, startdate
     ''' % (frepple.settings.current, frepple.settings.current + timedelta(days=horizon_future)))
  curname = None
  curlist = []
  for i, j in cursor.fetchall():
    if i != curname:
      if curname:
        thebuckets[curname] = curlist
        curlist = []
      curname = i
    curlist.append(j)
  if curname:
    thebuckets[curname] = curlist

  # Read history data and generate forecast
  cursor.execute('''SELECT forecast.name, calendarbucket.startdate,
     coalesce(forecastplan.orderstotal, 0) + coalesce(forecastplan.ordersadjustment, 0) as r
     FROM forecast
     INNER JOIN calendarbucket
       ON calendarbucket.calendar_id = forecast.calendar_id
     LEFT OUTER JOIN forecastplan
       ON forecastplan.forecast_id = forecast.name
       AND calendarbucket.startdate = forecastplan.startdate
     WHERE calendarbucket.startdate >= '%s'
      AND calendarbucket.startdate < '%s'
     ORDER BY forecast.name, calendarbucket.startdate''' % (frepple.settings.current - timedelta(days=horizon_history), frepple.settings.current))
  first = True
  for rec in cursor.fetchall():
    fcst = frepple.demand(name=rec[0])
    if curfcst != fcst:
      # First demand bucket of a new forecast
      if curfcst:
        # Generate the forecast
        solver_fcst.timeseries(curfcst, data, thebuckets[fcst.calendar.name])
      curfcst = fcst
      data = []
      first = True
    elif not first or rec[2] > 0:
      data.append(rec[2])
      first = False
  if curfcst:
    # Generate the forecast
    solver_fcst.timeseries(curfcst, data, thebuckets[fcst.calendar.name])

  print("Exporting baseline forecast...")
  cursor.execute('''
    update forecastplan
    set forecastbaseline=0, forecastbaselinevalue=0, method=null
    where startdate>='%s'
      and (forecastbaseline<>0 or forecastbaselinevalue<>0 or method is not null)
    ''' % frepple.settings.current)
  cursor.executemany('''
    update forecastplan
    set forecastbaseline=%s, forecastbaselinevalue=%s, method=%s
    where forecast_id = %s and startdate=%s
    ''', [
      (
        round(i.total, settings.DECIMAL_PLACES),
        round(i.total * i.forecast.item.price, settings.DECIMAL_PLACES),
        i.forecast.method,
        i.forecast.name, str(i.startdate)
      )
      for i in frepple.demands()
      if isinstance(i, frepple.demand_forecastbucket) and i.forecast.methods != 0 and i.total != 0.0
    ])


def applyForecastAdjustments(cursor):
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  cursor.execute('''select
       forecast.name, calendarbucket.startdate, forecastplan.forecastadjustment
     from forecast
     inner join calendarbucket
       on calendarbucket.calendar_id = forecast.calendar_id
     left outer join forecastplan
       on forecastplan.forecast_id = forecast.name
       and calendarbucket.startdate = forecastplan.startdate
     where calendarbucket.enddate >= '%s'
       and calendarbucket.startdate < '%s'
       and forecastplan.forecastadjustment is not null
       and forecastplan.forecastadjustment > 0
     order by forecast.name, calendarbucket.startdate''' % (frepple.settings.current, frepple.settings.current + timedelta(days=horizon_future)))
  for fcstname, start, qty in cursor.fetchall():
    frepple.demand(name=fcstname).setQuantity(qty, start, start, False)


def loadForecastValues(cursor):
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  cursor.execute('''select
       forecast.name, calendarbucket.startdate, forecastplan.forecasttotal
     from forecast
     inner join calendarbucket
       on calendarbucket.calendar_id = forecast.calendar_id
     left outer join forecastplan
       on forecastplan.forecast_id = forecast.name
       and calendarbucket.startdate = forecastplan.startdate
     where calendarbucket.enddate >= '%s'
       and calendarbucket.startdate < '%s'
       and forecastplan.forecasttotal > 0
     order by forecast.name, calendarbucket.startdate''' % (frepple.settings.current, frepple.settings.current + timedelta(days=horizon_future)))
  for fcstname, start, qty in cursor.fetchall():
    frepple.demand(name=fcstname).setQuantity(qty, start, start, False)


def createSolver(cursor):
  # Initialize the solver
  kw = {'name': "Netting orders from forecast"}
  cursor.execute('''select name, value
     from common_parameter
     where name like 'forecast.%%'
     ''')
  for key, value in cursor.fetchall():
    if key in ('forecast.Horizon_future', 'forecast.Horizon_history'):
      continue
    elif key in ('forecast.DueAtEndOfBucket', 'forecast.Iterations', 'forecast.loglevel',
                 'forecast.MovingAverage_order', 'forecast.Net_CustomerThenItemHierarchy',
                 'forecast.Net_MatchUsingDeliveryOperation', 'forecast.Net_NetEarly',
                 'forecast.Net_NetLate', 'forecast.Skip'):
      kw[key[9:]] = int(value)
    else:
      kw[key[9:]] = float(value)
  return frepple.solver_forecast(**kw)


def exportForecastFull(cursor):
  def generator(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecastbucket) and (i.total != 0 or i.quantity != 0 or i.consumed != 0):
        yield i

  print("Exporting complete forecast...")
  starttime = time()
  cursor.execute('''update forecastplan
    set forecasttotal=0, forecastnet=0, forecastconsumed=0, ordersplanned=0, forecastplanned=0
    where startdate >= '%s'
      and (forecasttotal<>0 or forecastnet<>0 or forecastconsumed<>0 or ordersplanned <> 0 or forecastplanned <> 0)
    ''' % frepple.settings.current)
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany(
    '''update forecastplan
     set forecasttotal=%s, forecastnet=%s, forecastconsumed=%s,
       forecasttotalvalue=%s, forecastnetvalue=%s, forecastconsumedvalue=%s
     where forecast_id=%s and startdate=%s''', [
      (
        round(i.total, settings.DECIMAL_PLACES),
        round(i.quantity, settings.DECIMAL_PLACES),
        round(i.consumed, settings.DECIMAL_PLACES),
        round(i.total*i.item.price, settings.DECIMAL_PLACES),
        round(i.quantity*i.item.price, settings.DECIMAL_PLACES),
        round(i.consumed*i.item.price, settings.DECIMAL_PLACES),
        i.owner.name, str(i.startdate)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  cursor.execute('''
    update forecastplan
      set ordersplanned=coalesce(plannedquantities.planneddemand,0),
          forecastplanned=coalesce(plannedquantities.plannedforecast,0),
          ordersplannedvalue=coalesce(plannedquantities.planneddemandvalue,0),
          forecastplannedvalue=coalesce(plannedquantities.plannedforecastvalue,0)
      from (
        select
           forecast.name as forecast, calendarbucket.startdate as startdate,
           sum(case when demand.name is not null then planquantity else 0 end) as planneddemand,
           sum(case when demand.name is null then planquantity else 0 end) as plannedforecast,
           sum(case when demand.name is not null then (planquantity*item.price) else 0 end) as planneddemandvalue,
           sum(case when demand.name is null then (planquantity*item.price) else 0 end) as plannedforecastvalue
        from out_demand
        inner join item
          on out_demand.item = item.name
        inner join customer
          on out_demand.customer = customer.name
        left outer join demand
          on out_demand.demand = demand.name
        inner join item as fitem
          on item.lft between fitem.lft and fitem.rght
        inner join customer as fcustomer
          on customer.lft between fcustomer.lft and fcustomer.rght
        inner join forecast
          on fitem.name = forecast.item_id
          and fcustomer.name = forecast.customer_id
        inner join calendarbucket
          on calendarbucket.calendar_id = forecast.calendar_id
          and out_demand.plandate >= calendarbucket.startdate
          and out_demand.plandate < calendarbucket.enddate
        group by forecast.name, calendarbucket.startdate
        ) plannedquantities
      where forecastplan.forecast_id = plannedquantities.forecast
        and forecastplan.startdate = plannedquantities.startdate
        and (
          forecastplan.ordersplanned <> plannedquantities.planneddemand
          or forecastplan.forecastplanned <> plannedquantities.plannedforecast
          or forecastplan.ordersplannedvalue <> plannedquantities.planneddemandvalue
          or forecastplan.forecastplannedvalue <> plannedquantities.plannedforecastvalue
          )
    ''')
  transaction.commit(using=cursor.db.alias)
  print('Updated planned quantity fields in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.execute('''
    with aggfcst as (
      select
        forecastparent.name forecast_id, startdate,
        sum(forecasttotal) forecasttotal,
        sum(forecastbaseline) forecastbaseline,
        sum(forecastconsumed) forecastconsumed,
        sum(forecastnet) forecastnet,
        sum(ordersadjustment) ordersadjustment,
        sum(forecastadjustment) forecastadjustment,
        sum(forecasttotalvalue) forecasttotalvalue,
        sum(forecastbaselinevalue) forecastbaselinevalue,
        sum(forecastconsumedvalue) forecastconsumedvalue,
        sum(forecastnetvalue) forecastnetvalue,
        sum(ordersadjustmentvalue) ordersadjustmentvalue,
        sum(forecastadjustmentvalue) forecastadjustmentvalue
      from forecastplan
      inner join forecast
        on forecast_id = name
      inner join item
        on item.name = forecast.item_id
      inner join customer
        on customer.name = forecast.customer_id
      cross join forecast as forecastparent
      inner join item as itemparent
        on forecastparent.item_id = itemparent.name
        and item.lft >= itemparent.lft
        and item.lft < itemparent.rght
      inner join customer as customerparent
        on forecastparent.customer_id = customerparent.name
        and customer.lft >= customerparent.lft
        and customer.lft < customerparent.rght
      where forecast.planned = 't'
        and forecastparent.planned = 'f'
      group by forecastparent.name, startdate
      )
    update forecastplan
    set
      forecasttotal = aggfcst.forecasttotal,
      forecastbaseline = aggfcst.forecastbaseline,
      forecastconsumed = aggfcst.forecastconsumed,
      forecastnet = aggfcst.forecastnet,
      ordersadjustment = aggfcst.ordersadjustment,
      forecastadjustment = aggfcst.forecastadjustment,
      forecasttotalvalue = aggfcst.forecasttotalvalue,
      forecastbaselinevalue = aggfcst.forecastbaselinevalue,
      forecastconsumedvalue = aggfcst.forecastconsumedvalue,
      forecastnetvalue = aggfcst.forecastnetvalue,
      ordersadjustmentvalue = aggfcst.ordersadjustmentvalue,
      forecastadjustmentvalue = aggfcst.forecastadjustmentvalue
    from aggfcst
    where exists (
      select 1
      from forecast
      where forecast.name = forecastplan.forecast_id
        and forecast.planned = 'f'
      )
      and forecastplan.forecast_id = aggfcst.forecast_id
      and forecastplan.startdate = aggfcst.startdate
    ''')
  transaction.commit(using=cursor.db.alias)
  print('Updated aggregated values in %.2f seconds' % (time() - starttime))


def exportForecastPlanned(cursor):
  def generator(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecastbucket) and (i.quantity != 0 or i.consumed != 0):
        yield i

  print("Exporting forecast plan...")
  starttime = time()
  cursor.execute('''update forecastplan
    set forecastnet=0, forecastconsumed=0, ordersplanned = 0, forecastplanned = 0
    where startdate >= '%s'
      and (forecastnet<>0 or forecastconsumed<>0 or ordersplanned <> 0 or forecastplanned <> 0)
    ''' % frepple.settings.current)
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany(
    '''update forecastplan
     set forecastnet=%s, forecastconsumed=%s,
       forecastnetvalue=%s, forecastconsumedvalue=%s
     where forecast_id=%s and startdate=%s''', [
      (
        round(i.quantity, settings.DECIMAL_PLACES),
        round(i.consumed, settings.DECIMAL_PLACES),
        round(i.quantity*i.item.price, settings.DECIMAL_PLACES),
        round(i.consumed*i.item.price, settings.DECIMAL_PLACES),
        i.owner.name, str(i.startdate)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  cursor.execute('''
    update forecastplan
      set ordersplanned=coalesce(plannedquantities.planneddemand,0),
          forecastplanned=coalesce(plannedquantities.plannedforecast,0),
          ordersplannedvalue=coalesce(plannedquantities.planneddemandvalue,0),
          forecastplannedvalue=coalesce(plannedquantities.plannedforecastvalue,0)
      from (
        select
           forecast.name as forecast, calendarbucket.startdate as startdate,
           sum(case when demand.name is not null then planquantity else 0 end) as planneddemand,
           sum(case when demand.name is null then planquantity else 0 end) as plannedforecast,
           sum(case when demand.name is not null then (planquantity*item.price) else 0 end) as planneddemandvalue,
           sum(case when demand.name is null then (planquantity*item.price) else 0 end) as plannedforecastvalue
        from out_demand
        inner join item
          on out_demand.item = item.name
        inner join customer
          on out_demand.customer = customer.name
        left outer join demand
          on out_demand.demand = demand.name
        inner join item as fitem
          on item.lft between fitem.lft and fitem.rght
        inner join customer as fcustomer
          on customer.lft between fcustomer.lft and fcustomer.rght
        inner join forecast
          on fitem.name = forecast.item_id
          and fcustomer.name = forecast.customer_id
        inner join calendarbucket
          on calendarbucket.calendar_id = forecast.calendar_id
          and out_demand.plandate >= calendarbucket.startdate
          and out_demand.plandate < calendarbucket.enddate
        group by forecast.name, calendarbucket.startdate
        ) plannedquantities
      where forecastplan.forecast_id = plannedquantities.forecast
        and forecastplan.startdate = plannedquantities.startdate
        and (
          forecastplan.ordersplanned <> plannedquantities.planneddemand
          or forecastplan.forecastplanned <> plannedquantities.plannedforecast
          or forecastplan.ordersplannedvalue <> plannedquantities.planneddemandvalue
          or forecastplan.forecastplannedvalue <> plannedquantities.plannedforecastvalue
          )
    ''')
  transaction.commit(using=cursor.db.alias)
  print('Updated planned quantity fields in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.execute('''
    with aggfcst as (
      select
        forecastparent.name forecast_id, startdate,
        sum(forecastconsumed) forecastconsumed,
        sum(forecastnet) forecastnet,
        sum(forecastconsumedvalue) forecastconsumedvalue,
        sum(forecastnetvalue) forecastnetvalue
      from forecastplan
      inner join forecast
        on forecast_id = name and forecast.planned = 't'
      inner join item
        on forecast.item_id = item.name
      inner join customer
        on forecast.customer_id = customer.name
      cross join forecast as forecastparent
      inner join item as itemparent
        on forecastparent.item_id = item.name
        and item.lft >= itemparent.lft
        and item.lft < itemparent.rght
      inner join customer as customerparent
        on forecastparent.customer_id = customerparent.name
        and customer.lft >= customerparent.lft
        and customer.lft < customerparent.rght
      group by forecastparent.name, startdate
      )
    update forecastplan
    set
      forecastconsumed = aggfcst.forecastconsumed,
      forecastnet = aggfcst.forecastnet,
      forecastconsumedvalue = aggfcst.forecastconsumedvalue
    from aggfcst
    where exists (
      select 1
      from forecast
      where forecast.name = forecastplan.forecast_id
        and forecast.planned = 'f'
      )
      and forecastplan.forecast_id = aggfcst.forecast_id
      and forecastplan.startdate = aggfcst.startdate
    ''')
  transaction.commit(using=cursor.db.alias)
  print('Updated aggregated values in %.2f seconds' % (time() - starttime))


def exportForecastValues(cursor):
  def generator(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecastbucket) and i.total != 0.0:
        yield i

  print("Exporting forecast values...")
  starttime = time()
  cursor.execute('''update forecastplan
    set forecasttotal=0
    where startdate >= '%s'
      and forecasttotal<>0
    ''' % frepple.settings.current)
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany(
    '''update forecastplan
     set forecasttotal=%s,
       forecasttotalvalue=%s
     where forecast_id=%s and startdate=%s''', [
      (
        round(i.total, settings.DECIMAL_PLACES),
        round(i.total*i.item.price, settings.DECIMAL_PLACES),
        i.owner.name, str(i.startdate)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  print('Updated total values in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.execute('''
    with aggfcst as (
      select
        forecastparent.name forecast_id, startdate,
        sum(forecasttotal) forecasttotal,
        sum(forecasttotalvalue) forecasttotalvalue
      from forecastplan
      inner join forecast
        on forecast_id = name and forecast.planned = 't'
      inner join item
        on forecast.item_id = item.name
      inner join customer
        on forecast.customer_id = customer.name
      cross join forecast as forecastparent
      inner join item as itemparent
        on forecastparent.item_id = item.name
        and item.lft >= itemparent.lft
        and item.lft < itemparent.rght
      inner join customer as customerparent
        on forecastparent.customer_id = customerparent.name
        and customer.lft >= customerparent.lft
        and customer.lft < customerparent.rght
      group by forecastparent.name, startdate
      )
    update forecastplan
    set
      forecasttotal = aggfcst.forecasttotal,
      forecasttotalvalue = aggfcst.forecasttotalvalue
    from aggfcst
    where exists (
      select 1
      from forecast
      where forecast.name = forecastplan.forecast_id
        and forecast.planned = 'f'
      )
      and forecastplan.forecast_id = aggfcst.forecast_id
      and forecastplan.startdate = aggfcst.startdate
    ''')
  transaction.commit(using=cursor.db.alias)
  print('Updated aggregated values in %.2f seconds' % (time() - starttime))


def generate_plan():
  # Select database
  try:
    db = os.environ['FREPPLE_DATABASE'] or DEFAULT_DB_ALIAS
  except:
    db = DEFAULT_DB_ALIAS

  # Use the test database if we are running the test suite
  if 'FREPPLE_TEST' in os.environ:
    settings.DATABASES[db]['NAME'] = settings.DATABASES[db]['TEST']['NAME']

  # Make sure the debug flag is not set!
  # When it is set, the Django database wrapper collects a list of all sql
  # statements executed and their timings. This consumes plenty of memory
  # and cpu time.
  settings.DEBUG = False

  # Welcome message
  printWelcome(database=db)
  logProgress(1, db)

  from freppledb.execute.load import loadData
  frepple.printsize()
  if 'odoo_read' in os.environ:
    # Use input data from the frePPLe database and Odoo
    print("\nStart loading data from the database with filter \"source <> 'odoo'\" at", datetime.now().strftime("%H:%M:%S"))
    loadData(database=db, filter="source is null or source<>'odoo'").run()
    frepple.printsize()
    logProgress(10, db)
    print("\nStart loading data from odoo at", datetime.now().strftime("%H:%M:%S"))
    from freppledb.odoo.commands import odoo_read
    odoo_read(db)
    frepple.printsize()
  else:
    # Use input data from the frePPLe database
    print("\nStart loading data from the database at", datetime.now().strftime("%H:%M:%S"))
    loadData(database=db, filter=None).run()
    frepple.printsize()
  logProgress(33, db)

  # Create a database connection
  cursor = connections[db].cursor()
  if settings.DATABASES[db]['ENGINE'] == 'django.db.backends.sqlite3':
    cursor.execute('PRAGMA temp_store = MEMORY;')
    cursor.execute('PRAGMA synchronous = OFF')
    cursor.execute('PRAGMA cache_size = 8000')
  elif settings.DATABASES[db]['ENGINE'] == 'oracle':
    cursor.execute("ALTER SESSION SET COMMIT_WRITE='BATCH,NOWAIT'")

  # Detect whether the forecast module is available
  with_forecasting = 'demand_forecast' in [ a[0] for a in inspect.getmembers(frepple) ]
  if settings.DATABASES[db]['ENGINE'] != 'django.db.backends.postgresql_psycopg2':
    print("Warning: forecast module is only supported when using a PostgreSQL database")
    with_forecasting = False

  if with_forecasting:
    print("\nStart loading forecast data from the database at", datetime.now().strftime("%H:%M:%S"))
    try:
      loadForecast(cursor)
    except Exception as e:
      print(e)

    solver_fcst = createSolver(cursor)
    # Assure the hierarchies are up to date
    from freppledb.forecast.models import Forecast
    print("\nStart building hierarchies at", datetime.now().strftime("%H:%M:%S"))
    Item.rebuildHierarchy(database=db)
    Customer.rebuildHierarchy(database=db)
    logProgress(33, db)

    print("\nStart aggregating demand at", datetime.now().strftime("%H:%M:%S"))
    aggregateDemand(cursor)
    logProgress(50, db)

    if 'noforecast' in os.environ:
      # Option A: Just load the forecast values previously computed
      if not 'noproduction' in os.environ:
        print("\nLoading total forecast values at", datetime.now().strftime("%H:%M:%S"))
        loadForecastValues(cursor)
        logProgress(66, db)
    else:
      # Option B: Recompute the statistical forecast
      print("\nStart processing forecastdemand records at", datetime.now().strftime("%H:%M:%S"))
      processForecastDemand(cursor)
      logProgress(58, db)

      print("\nStart generation of baseline forecast at", datetime.now().strftime("%H:%M:%S"))
      generateBaseline(solver_fcst, cursor)
      logProgress(66, db)

      print("\nStart applying forecast adjustments at", datetime.now().strftime("%H:%M:%S"))
      applyForecastAdjustments(cursor)
      logProgress(75, db)

    if not 'noproduction' in os.environ:
      print("\nStart forecast netting at", datetime.now().strftime("%H:%M:%S"))
      solver_fcst.solve()
      frepple.printsize()
      logProgress(83, db)

  if not 'noproduction' in os.environ:
    print("\nStart plan generation at", datetime.now().strftime("%H:%M:%S"))
    createPlan(db)
    frepple.printsize()
    logProgress(94, db)

  if 'odoo_read' in os.environ:
    print("\nStart exporting static model to the database with filter \"source = 'odoo'\" at", datetime.now().strftime("%H:%M:%S"))
    from freppledb.execute.export_database_static import exportStaticModel
    exportStaticModel(database=db, source='odoo').run()
  from freppledb.execute.export_database_static import exportStaticModel
  exportStaticModel(database=db).run()

  if not 'noproduction' in os.environ:
    print("\nStart exporting plan to the database at", datetime.now().strftime("%H:%M:%S"))
    exportPlan(db)
  if with_forecasting:
    if 'noproduction' in os.environ:
      if not 'noforecast' in os.environ:
        # Export only the base forecast
        exportForecastValues(cursor)
      #else:
        # Export nothing to the fcst table
    else:
      if 'noforecast' in os.environ:
        # Export only planned quantities to the forecast plan table
        exportForecastPlanned(cursor)
      else:
        # Export both the computed forecast and the planned quantities
        exportForecastFull(cursor)

  if 'odoo_write' in os.environ:
    from freppledb.odoo.commands import odoo_write
    print("\nStart exporting plan to odoo at", datetime.now().strftime("%H:%M:%S"))
    odoo_write(db)

  #if settings.DATABASES[db]['ENGINE'] == 'django.db.backends.postgresql_psycopg2':
  #  from freppledb.execute.export_database_plan_postgresql import exportfrepple as export_plan_to_database
  #else:
  #  from freppledb.execute.export_database_plan import exportfrepple as export_plan_to_database
  #export_plan_to_database()

  #print("\nStart saving the plan to flat files at", datetime.now().strftime("%H:%M:%S"))
  #from freppledb.execute.export_file_plan import exportfrepple as export_plan_to_file
  #export_plan_to_file()

  #print("\nStart saving the plan to an XML file at", datetime.now().strftime("%H:%M:%S"))
  #frepple.saveXMLfile("output.1.xml","PLANDETAIL")
  #frepple.saveXMLfile("output.2.xml","PLAN")
  #frepple.saveXMLfile("output.3.xml","STANDARD")

  #print("Start deleting model data at", datetime.now().strftime("%H:%M:%S"))
  #frepple.erase(True)
  #frepple.printsize()

  print("\nFinished planning at", datetime.now().strftime("%H:%M:%S"))
  logProgress(100, db)


if __name__ == "__main__":
  try:
    generate_plan()
  except Exception as e:
    print("Error during planning: ", e)
    raise
