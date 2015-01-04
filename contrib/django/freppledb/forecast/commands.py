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
    category,subcategory
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
      fcst.discrete = False
    if i[8] is not None:
      fcst.maxlateness = i[8]
  print('Loaded %d forecasts in %.2f seconds' % (cnt, time() - starttime))


def aggregateDemand(cursor):
  # Aggregate demand history
  starttime = time()
  cursor.execute('update forecastplan set orderstotal = 0, ordersopen = 0')
  transaction.commit(using=cursor.db.alias)
  print('Aggregate - reset records in %.2f seconds' % (time() - starttime))

  # Create a temp table with the aggregated demand
  starttime = time()
  cursor.execute('''
     create temp table demand_history
     on commit preserve rows
     as
      select forecast.name as forecast, calendarbucket.startdate as startdate,
        fcustomer.lft as customer, fitem.lft as item,
        sum(demand.quantity) as orderstotal,
        sum(case when demand.status is null or demand.status = 'open' then demand.quantity else 0 end) as ordersopen
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
      group by forecast.name, fcustomer.lft, fitem.lft, calendarbucket.startdate
     ''')
  cursor.execute('''CREATE UNIQUE INDEX demand_history_idx ON demand_history (forecast, startdate)''')
  print('Aggregate - temp table in %.2f seconds' % (time() - starttime))

  # Create all active history pairs
  starttime = time()
  cursor.execute('''
    insert into forecastplan (
      forecast_id, customerlvl, itemlvl, startdate, orderstotal, ordersopen,
      forecastbaseline, forecastadjustment, forecasttotal, forecastnet, forecastconsumed,
      ordersadjustment, ordersplanned, forecastplanned
      )
    select demand_history.forecast, demand_history.customer, demand_history.item,
       demand_history.startdate, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
    set orderstotal = demand_history.orderstotal , ordersopen = demand_history.ordersopen
    from demand_history
    where forecastplan.forecast_id = demand_history.forecast
      and forecastplan.startdate = demand_history.startdate
    ''')
  transaction.commit(using=cursor.db.alias)
  cursor.execute("drop table demand_history")
  print('Aggregate - update order records in %.2f seconds' % (time() - starttime))

  # Initialize all buckets in the past and future
  starttime = time()
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  cursor.execute('''
    insert into forecastplan (
        forecast_id, customerlvl, itemlvl, startdate, orderstotal, ordersopen,
        forecastbaseline, forecastadjustment, forecasttotal, forecastnet, forecastconsumed,
        ordersadjustment, ordersplanned, forecastplanned
        )
      select forecast.name, customer.lft, item.lft, calendarbucket.startdate, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
      group by forecast.name, customer.lft, item.lft, calendarbucket.startdate
    ''' % (frepple.settings.current, frepple.settings.current + timedelta(days=horizon_future))
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
    set forecastbaseline = 0
    where startdate > '%s'
    ''' % frepple.settings.current)
  cursor.executemany('''
    update forecastplan
    set forecastbaseline=%s
    where forecast_id = %s and startdate=%s
    ''', [
      (
        round(i.total, settings.DECIMAL_PLACES),
        i.owner.name, str(i.startdate)
      )
      for i in frepple.demands()
      if isinstance(i, frepple.demand_forecastbucket)
    ])


def applyForecastAdjustments(cursor):
  horizon_future = int(Parameter.getValue('forecast.Horizon_future', cursor.db.alias, 365))
  cursor.execute('''select forecast.name, calendarbucket.startdate,
       forecastplan.forecastadjustment
     from forecast
     inner join calendarbucket
       on calendarbucket.calendar_id = forecast.calendar_id
     left outer join forecastplan
       on forecastplan.forecast_id = forecast.name
       and calendarbucket.startdate = forecastplan.startdate
     where calendarbucket.enddate >= '%s'
       and calendarbucket.startdate < '%s'
       and forecastplan.forecastadjustment > 0
     order by forecast.name, calendarbucket.startdate''' % (frepple.settings.current, frepple.settings.current + timedelta(days=horizon_future)))
  for fcstname, start, qty in cursor.fetchall():
    frepple.demand(name=fcstname).setQuantity(qty, start, start, True)


def createSolver(cursor):
  # Initialize the solver
  kw = {'name': "Netting orders from forecast"}
  cursor.execute('''
     select name, value
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


def exportForecast(cursor):
  def generator(cursor):
    for i in frepple.demands():
      if isinstance(i, frepple.demand_forecastbucket):
        yield i

  print("Exporting forecast...")
  starttime = time()
  cursor.execute('''update forecastplan
    set forecasttotal=0, forecastnet=0, forecastconsumed=0
    where startdate > '%s'
    ''' % frepple.settings.current)
  print('Export set to 0 in %.2f seconds' % (time() - starttime))
  starttime = time()
  cursor.executemany(
    '''update forecastplan
     set forecasttotal=%s, forecastnet=%s, forecastconsumed=%s
     where forecast_id = %s and startdate=%s''', [
      (
        round(i.total, settings.DECIMAL_PLACES),
        round(i.quantity, settings.DECIMAL_PLACES),
        round(i.consumed, settings.DECIMAL_PLACES),
        i.owner.name, str(i.startdate)
      )
      for i in generator(cursor)
    ])
  transaction.commit(using=cursor.db.alias)
  print('Exported forecast in %.2f seconds' % (time() - starttime))  # TODO use fast export for forecast
  cursor.execute('update forecastplan set ordersplanned = 0, forecastplanned = 0')
  transaction.commit(using=cursor.db.alias)
  cursor.execute('''
    update forecastplan
      set ordersplanned = plannedquantities.planneddemand,
          forecastplanned = plannedquantities.plannedforecast
      from (
        select
           forecast.name as forecast, calendarbucket.startdate as startdate,
           sum(case when demand.name is not null then planquantity else 0 end) as planneddemand,
           sum(case when demand.name is null then planquantity else 0 end) as plannedforecast
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
    ''')
  transaction.commit(using=cursor.db.alias)
  print('Updated planned quantity fields in %.2f seconds' % (time() - starttime))


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
    # Load forecast data
    print("\nStart loading forecast data from the database at", datetime.now().strftime("%H:%M:%S"))
    loadForecast(cursor)

    # Intialize the solver
    solver_fcst = createSolver(cursor)

    # Assure the hierarchies are up to date
    print("\nStart building hierarchies at", datetime.now().strftime("%H:%M:%S"))
    Item.rebuildHierarchy(database=db)
    Customer.rebuildHierarchy(database=db)
    logProgress(33, db)

    print("\nStart aggregating demand at", datetime.now().strftime("%H:%M:%S"))
    aggregateDemand(cursor)
    logProgress(50, db)

    print("\nStart processing forecastdemand records at", datetime.now().strftime("%H:%M:%S"))
    processForecastDemand(cursor)
    logProgress(58, db)

    print("\nStart generation of baseline forecast at", datetime.now().strftime("%H:%M:%S"))
    generateBaseline(solver_fcst, cursor)
    logProgress(66, db)

    print("\nStart applying forecast adjustments at", datetime.now().strftime("%H:%M:%S"))
    applyForecastAdjustments(cursor)
    logProgress(75, db)

    print("\nStart forecast netting at", datetime.now().strftime("%H:%M:%S"))
    solver_fcst.solve()
    frepple.printsize()
    logProgress(83, db)

  print("\nStart plan generation at", datetime.now().strftime("%H:%M:%S"))
  createPlan(db)
  logProgress(94, db)

  if 'odoo_read' in os.environ:
    print("\nStart exporting static model to the database with filter \"source = 'odoo'\" at", datetime.now().strftime("%H:%M:%S"))
    from freppledb.execute.export_database_static import exportStaticModel
    exportStaticModel(database=db, source='odoo').run()

  print("\nStart exporting plan to the database at", datetime.now().strftime("%H:%M:%S"))
  exportPlan(db)
  if with_forecasting:
    exportForecast(cursor)

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
  generate_plan()
