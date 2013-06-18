import os, inspect
from datetime import datetime, timedelta
from time import time

from django.db import connections, transaction, DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.common.models import Parameter
from freppledb.input.models import Item, Customer
from freppledb.execute.commands import printWelcome, logProgress, createPlan, exportPlan


def loadForecast(cursor):
  print 'Importing forecast...'
  cnt = 0
  starttime = time()
  cursor.execute('''SELECT name, customer_id, item_id, priority,
    operation_id, minshipment, calendar_id, discrete, maxlateness,
    category,subcategory
    FROM forecast''')
  for i,j,k,l,m,n,o,p,q,r,s in cursor.fetchall():
    cnt += 1
    fcst = frepple.demand_forecast(name=i, priority=l, category=r, subcategory=s)
    if j: fcst.customer = frepple.customer(name=j)
    if k: fcst.item = frepple.item(name=k)
    if m: fcst.operation = frepple.operation(name=m)
    if n: fcst.minshipment = n
    if o: fcst.calendar = frepple.calendar(name=o)
    if not p: fcst.discrete = False
    if q != None: fcst.maxlateness = q
  print 'Loaded %d forecasts in %.2f seconds' % (cnt, time() - starttime)


def loadForecastdemand(cursor):
  return 
  # Detect whether the forecast module is available
  if not 'demand_forecast' in [ a[0] for a in inspect.getmembers(frepple) ]:
    return

  print 'Importing forecast demand...'
  cnt = 0
  starttime = time()
  cursor.execute("SELECT forecast_id, quantity, startdate, enddate FROM forecastdemand")
  for i, j, k, l in cursor.fetchall():
    cnt += 1
    frepple.demand_forecast(name=i).setQuantity(j,k,l)
  print 'Loaded %d forecast demands in %.2f seconds' % (cnt, time() - starttime)


def aggregateDemand(cursor):
  # Aggregate demand history
  cursor.execute('update forecastplan set orderstotal = 0, ordersopen = 0')
  transaction.commit(using=db)
  
  # Create all active history pairs 
  cursor.execute('''
    insert into forecastplan (
        forecast_id, customerlvl, itemlvl, startdate, orderstotal, ordersopen, 
        forecastbaseline, forecastadjustment, forecasttotal, forecastnet, forecastconsumed
        ) 
      select forecast.name, fcustomer.lft, fitem.lft, common_bucketdetail.startdate, 0, 0, 0, 0, 0, 0, 0 
      from demand
      inner join item as ditem on demand.item_id = ditem.name
      inner join customer as dcustomer on demand.customer_id = dcustomer.name 
      inner join item as fitem on ditem.lft between fitem.lft and fitem.rght
      inner join customer as fcustomer on dcustomer.lft between fcustomer.lft and fcustomer.rght
      inner join forecast on fitem.name = forecast.item_id and fcustomer.name = forecast.customer_id
      inner join common_bucketdetail 
        on forecast.calendar_id = common_bucketdetail.bucket_id
        and common_bucketdetail.startdate <= demand.due
        and common_bucketdetail.enddate > demand.due  
      left outer join forecastplan 
        on common_bucketdetail.startdate = forecastplan.startdate  
        and forecastplan.forecast_id = forecast.name  
      where forecastplan.forecast_id is null
      group by forecast.name, fcustomer.lft, fitem.lft, common_bucketdetail.startdate'''
  )
  transaction.commit(using=db)
  
  # Create aggregate demand history
  cursor.execute('''select forecast.name as forecast, common_bucketdetail.startdate, 
         sum(demand.quantity) as orderstotal,
         sum(case when demand.status is null or demand.status = 'open' then demand.quantity else 0 end) as ordersopen 
      from demand
      inner join item as ditem on demand.item_id = ditem.name
      inner join customer as dcustomer on demand.customer_id = dcustomer.name 
      inner join item as fitem on ditem.lft between fitem.lft and fitem.rght
      inner join customer as fcustomer on dcustomer.lft between fcustomer.lft and fcustomer.rght
      inner join forecast on fitem.name = forecast.item_id and fcustomer.name = forecast.customer_id
      inner join common_bucketdetail 
        on forecast.calendar_id = common_bucketdetail.bucket_id
        and common_bucketdetail.startdate <= demand.due
        and common_bucketdetail.enddate > demand.due  
      group by forecast.name, common_bucketdetail.startdate''')
  results = [ (i[2],i[3], i[0], str(i[1])) for i in cursor.fetchall() ]
  cursor.executemany(
      "update forecastplan set orderstotal = %s , ordersopen = %s \
      where forecast_id = %s and startdate = %s", results )
  transaction.commit(using=db)

  # Initialize future buckets 
  cursor.execute('''
    insert into forecastplan (
        forecast_id, customerlvl, itemlvl, startdate, orderstotal, ordersopen, 
        forecastbaseline, forecastadjustment, forecasttotal, forecastnet, forecastconsumed
        ) 
      select forecast.name, customer.lft, item.lft, calendarbucket.startdate, 0, 0, 0, 0, 0, 0, 0 
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
      group by forecast.name, customer.lft, item.lft, calendarbucket.startdate''' % (frepple.settings.current, frepple.settings.current + timedelta(days=365*2))
  )
  transaction.commit(using=db)
  

def generateBaseline(cursor):
  data = []
  curfcst = None  
  cursor.execute('''select startdate 
     from common_bucketdetail 
     where bucket_id='month' and startdate >= '%s' and startdate < '%s'
     ''' % (frepple.settings.current, frepple.settings.current + timedelta(days=365*2)))
  thebuckets = [ i for i in cursor.fetchall() ]
  cursor.execute('''SELECT forecast.name, calendarbucket.startdate, 
     coalesce(forecastplan.orderstotal, 0) as r 
     FROM forecast
     INNER JOIN calendarbucket 
       ON calendarbucket.calendar_id = forecast.calendar_id        
     LEFT OUTER JOIN forecastplan 
       ON forecastplan.forecast_id = forecast.name       
       AND calendarbucket.startdate = forecastplan.startdate     
     WHERE calendarbucket.startdate >= '%s' 
      AND calendarbucket.startdate < '%s' 
     ORDER BY forecast.name, calendarbucket.startdate''' % (frepple.settings.current - timedelta(days=365*3), frepple.settings.current)) 
  for rec in cursor.fetchall():
    fcst = frepple.demand(name=rec[0])  
    if curfcst != fcst and rec[2] > 0:
      # First demand bucket of a new forecast
      if curfcst:
        # Generate the forecast
        thebuckets = [ i.start for i in fcst.calendar.buckets if i.start >= frepple.settings.current and i.start < frepple.settings.current + timedelta(days=365*2) ]
        curfcst.timeseries(data, thebuckets)
      curfcst = fcst
      data = []
    data.append(rec[2])
  if curfcst:
    # Generate the forecast
    thebuckets = [ i.start for i in fcst.calendar.buckets if i.start >= frepple.settings.current and i.start < frepple.settings.current + timedelta(days=365*2) ]
    curfcst.timeseries(data, thebuckets)

    
def exportForecast(cursor):
  cnt = 0
  for i in frepple.demands():
    if not isinstance(i, frepple.demand_forecastbucket):
      continue
    print i.name, i.due, i.total, i.quantity
    cursor.executemany(   # NOT EFFICIENT
      "update forecastplan set\
       forecastbaseline=%s, forecasttotal=%s, forecastnet=%s, forecastconsumed=%s \
       where forecast_id = %s and startdate=%s",
      [(
         round(i.total,settings.DECIMAL_PLACES), round(i.total,settings.DECIMAL_PLACES),
         round(i.quantity,settings.DECIMAL_PLACES),
         round(i.consumed,settings.DECIMAL_PLACES),
         i.owner.name, str(i.startdate)         
       )
      ])
    cnt += 1
    if cnt % 1000 == 0: transaction.commit(using=db)
  transaction.commit(using=db)


# Select database 
try: db = os.environ['FREPPLE_DATABASE'] or DEFAULT_DB_ALIAS
except: db = DEFAULT_DB_ALIAS
        
# Use the test database if we are running the test suite
if 'FREPPLE_TEST' in os.environ:
  settings.DATABASES[db]['NAME'] = settings.DATABASES[db]['TEST_NAME']
  if 'TEST_CHARSET' in os.environ:
    settings.DATABASES[db]['CHARSET'] = settings.DATABASES[db]['TEST_CHARSET']
  if 'TEST_COLLATION' in os.environ:
    settings.DATABASES[db]['COLLATION'] = settings.DATABASES[db]['TEST_COLLATION']
  if 'TEST_USER' in os.environ:
    settings.DATABASES[db]['USER'] = settings.DATABASES[db]['TEST_USER']
  
if __name__ == "__main__":
  # Welcome message
  printWelcome(db)
  logProgress(1, db)
  
  # Make sure the debug flag is not set!
  # When it is set, the django database wrapper collects a list of all sql
  # statements executed and their timings. This consumes plenty of memory
  # and cpu time.
  settings.DEBUG = False
  
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
  
  print "\nStart loading data from the database at", datetime.now().strftime("%H:%M:%S")
  frepple.printsize()
  from freppledb.execute.load import loadfrepple
  loadfrepple()
  if with_forecasting:
    loadForecast(cursor)
    loadForecastdemand(cursor)
  frepple.printsize()
  logProgress(16, db)
  
  if with_forecasting:
    # Initialize the solver
    kw = {'name': "Netting orders from forecast"}
    # TODO READ PARAMETERS
    # cursor.execute("select name, value from common_parameter where name like 'forecast.%%'")
    # for key, value in cursor.fetchall():
    #  kw[key[9:]] = float(value)
    solver_fcst = frepple.solver_forecast(**kw)
    
    # Assure the hierarchies are up to date
    print "\nStart building hierarchies at", datetime.now().strftime("%H:%M:%S")
    Item.rebuildHierarchy(database=db)
    Customer.rebuildHierarchy(database=db)
    logProgress(33, db)
    
    print "\nStart aggregating demand at", datetime.now().strftime("%H:%M:%S")
    aggregateDemand(cursor)
    logProgress(50, db)
    
    print "\nStart generation of baseline forecast at", datetime.now().strftime("%H:%M:%S")
    generateBaseline(cursor)
    logProgress(66, db)
  
    print "\nStart forecast netting at", datetime.now().strftime("%H:%M:%S")
    solver_fcst.solve()
    frepple.printsize()
    logProgress(83, db)
  
  print "\nStart plan generation at", datetime.now().strftime("%H:%M:%S")
  createPlan()
  logProgress(94, db)
  
  print "\nStart exporting plan to the database at", datetime.now().strftime("%H:%M:%S")
  exportPlan(db)
  exportForecast(cursor)
  
  #if settings.DATABASES[db]['ENGINE'] == 'django.db.backends.postgresql_psycopg2':
  #  from freppledb.execute.export_database_plan_postgresql import exportfrepple as export_plan_to_database
  #else:
  #  from freppledb.execute.export_database_plan import exportfrepple as export_plan_to_database
  #export_plan_to_database()
  
  #print "\nStart saving the plan to flat files at", datetime.now().strftime("%H:%M:%S")
  #from freppledb.execute.export_file_plan import exportfrepple as export_plan_to_file
  #export_plan_to_file()
  
  #print "\nStart saving the plan to an XML file at", datetime.now().strftime("%H:%M:%S")
  #frepple.saveXMLfile("output.1.xml","PLANDETAIL")
  #frepple.saveXMLfile("output.2.xml","PLAN")
  #frepple.saveXMLfile("output.3.xml","STANDARD")
  
  #print "Start deleting model data at", datetime.now().strftime("%H:%M:%S")
  #frepple.erase(True)
  #frepple.printsize()
  
  print "\nFinished planning at", datetime.now().strftime("%H:%M:%S")
  logProgress(100, db)
