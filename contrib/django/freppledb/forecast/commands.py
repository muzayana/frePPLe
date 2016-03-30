import inspect
import os
from datetime import datetime, timedelta
from time import time

from django.db import connections, transaction, DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.common.models import Parameter
from freppledb.input.models import Item, Customer, Location
from freppledb.execute.commands import printWelcome, logProgress, createPlan, exportPlan

import frepple


def loadForecast(cursor):
  print('Importing forecast...')
  cnt = 0
  starttime = time()
  cursor.execute('''SELECT name, customer_id, item_id, priority,
    operation_id, minshipment, discrete, maxlateness,
    category, subcategory, method, planned, location_id
    FROM forecast''')
  for i in cursor.fetchall():
    cnt += 1
    fcst = frepple.demand_forecast(name=i[0], priority=i[3], category=i[8], subcategory=i[9])
    if i[1]:
      fcst.customer = frepple.customer(name=i[1])
    if i[2]:
      fcst.item = frepple.item(name=i[2])
    if i[4]:
      fcst.operation = frepple.operation(name=i[4])
    if i[5]:
      fcst.minshipment = i[5]
    if not i[6]:
      fcst.discrete = False  # null value -> False
    if i[7] is not None:
      fcst.maxlateness = i[7]
    if i[10]:
      if i[10] == 'constant':
        fcst.methods = 1
      elif i[10] == 'trend':
        fcst.methods = 2
      elif i[10] == 'seasonal':
        fcst.methods = 4
      elif i[10] == 'intermittent':
        fcst.methods = 8
      elif i[10] == 'moving average':
        fcst.methods = 16
      elif i[10] == 'manual':
        fcst.methods = 0
    if i[11] is not None and not i[11]:
      fcst.planned = False  # null value -> True
    if i[12]:
      fcst.location = frepple.location(name=i[12])
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
  # TODO only the demand history within the window specificied in the parameter forecast.Horizon_history should be aggregated
  #      currently all periods in calendarbucket are used...
  starttime = time()
  cursor.execute('''
    create temporary table parent_child_price on commit preserve rows as
    select product.name parent, item.name child, 0.00 price
    from item product, item
    where item.lft between product.lft and product.rght
    and item.rght between product.lft and product.rght
    and exists (select 1 from forecast where item_id = product.name)
    ''')
  cursor.execute('''
    update parent_child_price
    set price = (
      select sum(coalesce(child.price,0))
      from item parent, item child
      where child.lft between parent.lft and parent.rght
        and parent.name = parent_child_price.child
      )''')
  cursor.execute('''
    create temporary table customers_customer on commit preserve rows as
    select customers.name customers, customer.name customer
    from customer customers, customer
    where customer.lft between customers.lft and customers.rght
    and exists (select 1 from forecast where customer_id = customers.name)
    ''')
  cursor.execute('''
    create temporary table locations_location on commit preserve rows as
    select locations.name locations, location.name as location
    from location locations, location
    where location.lft between locations.lft and locations.rght
    and exists (select 1 from forecast where location_id = locations.name)
    ''')
  cursor.execute('''
    create temporary table demand_history on commit preserve rows as
    select forecast.name as forecast,
    calendarbucket.startdate as startdate,
    calendarbucket.enddate as enddate,
    sum(demand.quantity) as orderstotal,
    sum(case when demand.status = 'open' then demand.quantity else 0 end) as ordersopen,
    coalesce(sum(demand.quantity*parent_child_price.price), 0) as orderstotalvalue,
    coalesce(sum(case when demand.status = 'open' then (demand.quantity*parent_child_price.price) else 0 end), 0) as ordersopenvalue
    from forecast
    left outer join locations_location on forecast.location_id = locations_location.locations
    left outer join customers_customer on forecast.customer_id = customers_customer.customers
    inner join parent_child_price on parent_child_price.parent = forecast.item_id
    inner join demand on demand.item_id = parent_child_price.child and demand.status in ('open','closed')
      and demand.location_id = coalesce(locations_location.location,demand.location_id)
      and demand.customer_id = coalesce(customers_customer.customer,demand.customer_id)
    inner join common_parameter on common_parameter.name = 'forecast.calendar'
    inner join calendarbucket on common_parameter.value = calendarbucket.calendar_id
      and calendarbucket.startdate <= demand.due
      and calendarbucket.enddate > demand.due
    group by forecast.name, calendarbucket.startdate, calendarbucket.enddate
    ''')
  cursor.execute('create unique index demand_history_idx ON demand_history (forecast, startdate)')
  cursor.execute('drop table locations_location')
  cursor.execute('drop table customers_customer')
  cursor.execute('drop table parent_child_price')
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
  fcst_calendar = Parameter.getValue('forecast.calendar', cursor.db.alias, None)
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
      inner join calendarbucket
        on calendarbucket.calendar_id = %s
        and calendarbucket.startdate >= %s
        and calendarbucket.startdate < %s
      left outer join forecastplan
        on forecastplan.startdate = calendarbucket.startdate
        and forecastplan.forecast_id = forecast.name
      where forecastplan.forecast_id is null
      group by forecast.name, calendarbucket.startdate, calendarbucket.enddate
    ''', (
      fcst_calendar,
      frepple.settings.current - timedelta(days=horizon_history),
      frepple.settings.current + timedelta(days=horizon_future)
      )
  )
  cursor.execute('vacuum analyze forecastplan')
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
  cursor.execute('vacuum analyze forecastdemand')


def generateBaseline(solver_fcst, cursor):

  def generatorFcst(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecast):
        yield i

  data = []
  curfcst = None

  # Build bucket lists
  horizon_history = int(Parameter.getValue('forecast.Horizon_history', cursor.db.alias, 10000))
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  fcst_calendar = Parameter.getValue('forecast.calendar', cursor.db.alias, None)
  if not fcst_calendar:
    raise Exception("Parameter forecast.calendar not set")
  # TODO Do validation checks on the forecast calendar: no overlapping entries allowed, no gaps between entries
  cursor.execute('''select distinct startdate
     from calendarbucket
     where calendarbucket.calendar_id = %s
       and enddate > %s and startdate < %s
     order by startdate
     ''', (
    fcst_calendar, frepple.settings.current,
    frepple.settings.current + timedelta(days=horizon_future)
    ))
  thebuckets = [ i[0] for i in cursor.fetchall() ]
  if not thebuckets:
    raise Exception("No calendar buckets found in forecast.calendar")

  # Read history data and generate forecast
  cursor.execute('''SELECT forecast.name, calendarbucket.startdate,
     coalesce(forecastplan.orderstotal, 0) + coalesce(forecastplan.ordersadjustment, 0) as r
     from forecast
     inner join calendarbucket
       on calendarbucket.calendar_id = %s
     left outer join forecastplan
       on forecastplan.forecast_id = forecast.name
       and calendarbucket.startdate = forecastplan.startdate
     where calendarbucket.startdate >= %s
      and calendarbucket.enddate <= %s
      and forecast.planned = true
     order by forecast.name, calendarbucket.startdate
     ''', (
       fcst_calendar, frepple.settings.current - timedelta(days=horizon_history),
       frepple.settings.current
     ))
  first = True
  for rec in cursor.fetchall():
    fcst = frepple.demand(name=rec[0])
    if curfcst != fcst:
      # First demand bucket of a new forecast
      if curfcst:
        # Generate the forecast
        solver_fcst.timeseries(curfcst, data, thebuckets)
      curfcst = fcst
      data = []
      first = True
    elif not first or rec[2] > 0:
      data.append(rec[2])
      first = False
  if curfcst:
    # Generate the forecast
    solver_fcst.timeseries(curfcst, data, thebuckets)

  print("Exporting baseline forecast...")
  cursor.execute('''
    update forecastplan
    set forecastbaseline=0, forecastbaselinevalue=0
    where forecastbaseline<>0 or forecastbaselinevalue<>0
    ''')
  cursor.execute('vacuum analyze forecastplan')
  cursor.executemany('''
    update forecastplan
    set forecastbaseline=%s, forecastbaselinevalue=%s
    where forecast_id = %s and startdate=%s
    ''', [
      (
        round(i.total, 4),
        round(i.total * i.forecast.item.price, 4),
        i.forecast.name, str(i.start)
      )
      for i in frepple.demands()
      if isinstance(i, frepple.demand_forecastbucket) and i.total != 0.0
    ])
  cursor.execute('vacuum analyze forecastplan')

  print("Exporting forecast metrics")
  cursor.execute('update forecast set out_smape=null, out_method=null, out_deviation=null')
  cursor.executemany('''
    update forecast
    set out_smape=%s, out_method=%s, out_deviation=%s
    where name = %s
    ''',
    [ (i.smape_error * 100, i.method, i.deviation, i.name) for i in generatorFcst(cursor) ]
    )
  cursor.execute('vacuum analyze forecast')


def applyForecastAdjustments(cursor):
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  fcst_calendar = Parameter.getValue('forecast.calendar', cursor.db.alias, None)
  cursor.execute('''select
       forecast.name, calendarbucket.startdate, forecastplan.forecastadjustment
     from forecast
     inner join calendarbucket
       on calendarbucket.calendar_id = %s
     left outer join forecastplan
       on forecastplan.forecast_id = forecast.name
       and calendarbucket.startdate = forecastplan.startdate
     where calendarbucket.enddate > %s
       and calendarbucket.startdate < %s
       and forecastplan.forecastadjustment is not null
       and forecastplan.forecastadjustment > 0
     order by forecast.name, calendarbucket.startdate''', (
    fcst_calendar, frepple.settings.current,
    frepple.settings.current + timedelta(days=horizon_future)
    ))
  for fcstname, start, qty in cursor.fetchall():
    frepple.demand(name=fcstname).setQuantity(qty, start, start, False)


def loadForecastValues(cursor):
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  fcst_calendar = Parameter.getValue('forecast.calendar', cursor.db.alias, None)
  cursor.execute('''select
       forecast.name, calendarbucket.startdate, forecastplan.forecasttotal
     from forecast
     inner join calendarbucket
       on calendarbucket.calendar_id = %s
     left outer join forecastplan
       on forecastplan.forecast_id = forecast.name
       and calendarbucket.startdate = forecastplan.startdate
     where calendarbucket.enddate > %s
       and calendarbucket.startdate < %s
       and forecastplan.forecasttotal > 0
       and forecast.planned = 't'
     order by forecast.name, calendarbucket.startdate''', (
    fcst_calendar, frepple.settings.current,
    frepple.settings.current + timedelta(days=horizon_future)
    ))
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
    elif key in ('forecast.DueWithinBucket',):
      kw[key[9:]] = value
    elif key == 'forecast.calendar':
      try:
        kw[key[9:]] = frepple.calendar(name=value, action="C")
      except:
        print("Warning: Parameter forecast.calendar not configured.")
        print("Warning: All forecast related calculations will be skipped.")
        print("")
        return None
    elif key in ('forecast.Iterations', 'forecast.loglevel', 'forecast.Skip'
                 'forecast.MovingAverage_order', 'forecast.Net_CustomerThenItemHierarchy',
                 'forecast.Net_MatchUsingDeliveryOperation', 'forecast.Net_NetEarly',
                 'forecast.Net_NetLate', ):
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
  # Reset all forecast fields in the future.
  cursor.execute('''update forecastplan
    set forecasttotal=0, forecastnet=0, forecastconsumed=0, ordersplanned=0, forecastplanned=0,
      forecasttotalvalue=0, forecastnetvalue=0, forecastconsumedvalue=0, ordersplannedvalue=0, forecastplannedvalue=0
    where enddate > %s
      and (forecasttotal<>0 or forecastnet<>0 or forecastconsumed<>0 or ordersplanned <> 0 or forecastplanned <> 0
        or forecasttotalvalue<>0 or forecastnetvalue<>0 or forecastconsumedvalue<>0 or ordersplannedvalue <> 0 or forecastplannedvalue <> 0)
    ''', (frepple.settings.current,))
  # Reset forecast fields in the past.
  # Note that the total forecast field is not reset in the past. This allows
  # us to track the historical forecast accuracy.
  cursor.execute('''update forecastplan
    set forecastnet=0, forecastconsumed=0, ordersplanned=0, forecastplanned=0,
      forecastbaselinevalue=0, forecastnetvalue=0, forecastconsumedvalue=0, ordersplannedvalue=0, forecastplannedvalue=0
    where enddate <= %s
      and (forecastnet<>0 or forecastconsumed<>0 or ordersplanned <> 0 or forecastplanned <> 0
        or forecastnetvalue<>0 or forecastconsumedvalue<>0 or ordersplannedvalue <> 0 or forecastplannedvalue <> 0)
    ''', (frepple.settings.current,))
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany('''update forecastplan
     set forecasttotal=%s, forecastnet=%s, forecastconsumed=%s,
       forecasttotalvalue=%s, forecastnetvalue=%s, forecastconsumedvalue=%s
     where forecast_id=%s and startdate=%s''', [
      (
        round(i.total, 4),
        round(i.quantity, 4),
        round(i.consumed, 4),
        round(i.total*i.item.price, 4),
        round(i.quantity*i.item.price, 4),
        round(i.consumed*i.item.price, 4),
        i.owner.name, str(i.start)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  fcst_calendar = Parameter.getValue('forecast.calendar', cursor.db.alias, None)
  cursor.execute('''
    update forecastplan
      set ordersplanned=coalesce(plannedquantities.planneddemand,0),
          forecastplanned=coalesce(plannedquantities.plannedforecast,0),
          ordersplannedvalue=coalesce(plannedquantities.planneddemandvalue,0),
          forecastplannedvalue=coalesce(plannedquantities.plannedforecastvalue,0)
      from (
        select
           forecast.name as forecast, calendarbucket.startdate as startdate,
           sum(case when demand.name is not null and location.lft between flocation.lft and flocation.rght then planquantity else 0 end) as planneddemand,
           sum(case when demand.name is null and out_demand.demand like forecast.name || ' - %%' then planquantity else 0 end) as plannedforecast,
           sum(case when demand.name is not null and location.lft between flocation.lft and flocation.rght then (planquantity*item.price) else 0 end) as planneddemandvalue,
           sum(case when demand.name is null and out_demand.demand like forecast.name || ' - %%' then (planquantity*item.price) else 0 end) as plannedforecastvalue
        from out_demand
        inner join item
          on out_demand.item = item.name
        left outer join customer
          on out_demand.customer = customer.name
        left outer join demand
          on out_demand.demand = demand.name
        left outer join location
          on location.name = demand.location_id
        inner join item as fitem
          on item.lft between fitem.lft and fitem.rght
        left outer join customer as fcustomer
          on customer.lft between fcustomer.lft and fcustomer.rght
        inner join forecast
          on fitem.name = forecast.item_id
          and fcustomer.name = forecast.customer_id
        inner join location as flocation
          on flocation.name = forecast.location_id
        inner join calendarbucket
          on calendarbucket.calendar_id = %s
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
    ''', (fcst_calendar,))
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
      left outer join customer
        on customer.name = forecast.customer_id
      left outer join location
        on location.name = forecast.location_id
      cross join forecast as forecastparent
      inner join item as itemparent
        on forecastparent.item_id = itemparent.name
        and item.lft >= itemparent.lft
        and item.lft < itemparent.rght
      left outer join customer as customerparent
        on forecastparent.customer_id = customerparent.name
        and customer.lft >= customerparent.lft
        and customer.lft < customerparent.rght
      left outer join location as locationparent
        on forecastparent.location_id = locationparent.name
        and location.lft >= locationparent.lft
        and location.lft < locationparent.rght
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
  cursor.execute('vacuum analyze forecastplan')
  print('Updated aggregated values in %.2f seconds' % (time() - starttime))


def exportForecastPlanned(cursor):
  def generator(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecastbucket) and (i.quantity != 0 or i.consumed != 0):
        yield i

  print("Exporting forecast plan...")
  starttime = time()
  cursor.execute('''update forecastplan
    set forecastnet=0, forecastconsumed=0, ordersplanned=0, forecastplanned=0,
      forecastnetvalue=0, forecastconsumedvalue=0, ordersplannedvalue=0, forecastplannedvalue=0
    where forecastnet<>0 or forecastconsumed<>0 or ordersplanned <> 0 or forecastplanned <> 0
      or forecastnetvalue<>0 or forecastconsumedvalue<>0 or ordersplannedvalue<>0 or forecastplannedvalue=0
    ''')
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany(
    '''update forecastplan
     set forecastnet=%s, forecastconsumed=%s,
       forecastnetvalue=%s, forecastconsumedvalue=%s
     where forecast_id=%s and startdate=%s''', [
      (
        round(i.quantity, 4),
        round(i.consumed, 4),
        round(i.quantity*i.item.price, 4),
        round(i.consumed*i.item.price, 4),
        i.owner.name, str(i.start)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  fcst_calendar = Parameter.getValue('forecast.calendar', cursor.db.alias, None)
  cursor.execute('''
    update forecastplan
      set ordersplanned=coalesce(plannedquantities.planneddemand,0),
          forecastplanned=coalesce(plannedquantities.plannedforecast,0),
          ordersplannedvalue=coalesce(plannedquantities.planneddemandvalue,0),
          forecastplannedvalue=coalesce(plannedquantities.plannedforecastvalue,0)
      from (
        select
           forecast.name as forecast, calendarbucket.startdate as startdate,
           sum(case when demand.name is not null and location.lft between flocation.lft and flocation.rght then planquantity else 0 end) as planneddemand,
           sum(case when demand.name is null and out_demand.demand like forecast.name || ' - %%' then planquantity else 0 end) as plannedforecast,
           sum(case when demand.name is not null and location.lft between flocation.lft and flocation.rght then (planquantity*item.price) else 0 end) as planneddemandvalue,
           sum(case when demand.name is null and out_demand.demand like forecast.name || ' - %%' then (planquantity*item.price) else 0 end) as plannedforecastvalue
        from out_demand
        inner join item
          on out_demand.item = item.name
        inner join customer
          on out_demand.customer = customer.name
        left outer join demand
          on out_demand.demand = demand.name
        left outer join location
          on demand.location_id = location.name
        inner join item as fitem
          on item.lft between fitem.lft and fitem.rght
        inner join customer as fcustomer
          on customer.lft between fcustomer.lft and fcustomer.rght
        inner join forecast
          on fitem.name = forecast.item_id
          and fcustomer.name = forecast.customer_id
        inner join location as flocation
          on flocation.name = forecast.location_id
        inner join calendarbucket
          on calendarbucket.calendar_id = %s
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
    ''', (fcst_calendar,))
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
      forecastnetvalue = aggfcst.forecastnetvalue,
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
  cursor.execute('vacuum analyze forecastplan')
  print('Updated aggregated values in %.2f seconds' % (time() - starttime))


def exportForecastValues(cursor):
  def generator(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecastbucket) and i.total != 0.0:
        yield i

  print("Exporting forecast values...")
  starttime = time()
  cursor.execute('''update forecastplan
    set forecasttotal=0, forecasttotalvalue=0
    where startdate >= %s
      and (forecasttotal<>0 or forecasttotalvalue<>0)
    ''', (frepple.settings.current,))
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany(
    '''update forecastplan
     set forecasttotal=%s,
       forecasttotalvalue=%s
     where forecast_id=%s and startdate=%s''', [
      (
        round(i.total, 4),
        round(i.total*i.item.price, 4),
        i.owner.name, str(i.start)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  cursor.execute('vacuum analyze forecastplan')
  print('Updated total values in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.execute('''
    with aggfcst as (
      select
        forecastparent.name forecast_id, startdate,
        sum(forecasttotal) forecasttotal,
        sum(forecastbaseline) forecastbaseline,
        sum(forecasttotalvalue) forecasttotalvalue,
        sum(forecastbaselinevalue) forecastbaselinevalue
      from forecastplan
      inner join forecast
        on forecast_id = name
      inner join item
        on item.name = forecast.item_id
      left outer join customer
        on customer.name = forecast.customer_id
      left outer join location
        on location.name = forecast.location_id
      cross join forecast as forecastparent
      inner join item as itemparent
        on forecastparent.item_id = itemparent.name
        and item.lft >= itemparent.lft
        and item.lft < itemparent.rght
      left outer join customer as customerparent
        on forecastparent.customer_id = customerparent.name
        and customer.lft >= customerparent.lft
        and customer.lft < customerparent.rght
      left outer join location as locationparent
        on forecastparent.location_id = locationparent.name
        and location.lft >= locationparent.lft
        and location.lft < locationparent.rght
      where forecast.planned = 't'
        and forecastparent.planned = 'f'
      group by forecastparent.name, startdate
      )
    update forecastplan
    set
      forecasttotal = aggfcst.forecasttotal,
      forecastbaseline = aggfcst.forecastbaseline,
      forecasttotalvalue = aggfcst.forecasttotalvalue,
      forecastbaselinevalue = aggfcst.forecastbaselinevalue
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
  cursor.execute('vacuum analyze forecastplan')
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

  # Detect whether the forecast module is available
  with_forecasting = 'demand_forecast' in [ a[0] for a in inspect.getmembers(frepple) ]
  if with_forecasting and not Parameter.getValue('forecast.calendar', db, None):
    with_forecasting = False
    print("Warning: parameter forecast.calendar not set. No forecast will be calculated.")

  # Detect whether the inventory planning module is available
  with_inventoryplanning = 'solver_inventoryplanning' in [ a[0] for a in inspect.getmembers(frepple) ]

  if with_forecasting:
    solver_fcst = createSolver(cursor)
    if not solver_fcst:
      with_forecasting = False

  if with_forecasting:
    print("\nStart loading forecast data from the database at", datetime.now().strftime("%H:%M:%S"))
    try:
      loadForecast(cursor)
    except Exception as e:
      print(e)

    # Assure the hierarchies are up to date
    print("\nStart building hierarchies at", datetime.now().strftime("%H:%M:%S"))
    Item.rebuildHierarchy(database=db)
    Location.rebuildHierarchy(database=db)
    Customer.rebuildHierarchy(database=db)
    logProgress(33, db)

    # Note: demand aggrgation is run, even when we choose not to run the
    # forecast calculation. We want to assure that the forecast report shows
    # the latest order information.
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

      if solver_fcst:
        print("\nStart generation of baseline forecast at", datetime.now().strftime("%H:%M:%S"))
        generateBaseline(solver_fcst, cursor)
      logProgress(66, db)

      print("\nStart applying forecast adjustments at", datetime.now().strftime("%H:%M:%S"))
      applyForecastAdjustments(cursor)
      logProgress(75, db)

    if not 'noproduction' in os.environ and solver_fcst:
      print("\nStart forecast netting at", datetime.now().strftime("%H:%M:%S"))
      solver_fcst.solve()
      frepple.printsize()
      logProgress(83, db)

  if not 'noinventory' in os.environ and with_inventoryplanning:
    from freppledb.inventoryplanning.commands import createInventoryPlan
    createInventoryPlan(database=db)

  if not 'noproduction' in os.environ:
    # Use the solver that solves constraints in a single sweeping pass.
    # Suitable for lightly constrained plans.
    # frepple.solver_moveout(loglevel=3).solve()
    # Use the solver which solves demand per demand.
    # Suitable for plans with complex constraints, alternates and
    # prioritization.
    if not 'noinventory' in os.environ and with_inventoryplanning:
      # Remove the unconstrained inventory plan
      frepple.erase(False)
    print("\nStart plan generation at", datetime.now().strftime("%H:%M:%S"))
    createPlan(db)
    frepple.printsize()
    logProgress(94, db)

  if 'odoo_read' in os.environ:
    print("\nStart exporting static model to the database with filter \"source = 'odoo'\" at", datetime.now().strftime("%H:%M:%S"))
    from freppledb.execute.export_database_static import exportStaticModel
    exportStaticModel(database=db, source='odoo').run()

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

  if not 'noevaluation' in os.environ and with_inventoryplanning:
    from freppledb.inventoryplanning.commands import computeStockoutProbability
    computeStockoutProbability(database=db)

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
