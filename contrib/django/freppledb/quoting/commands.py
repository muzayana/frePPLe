from __future__ import print_function

import os, inspect, time
from datetime import datetime

from django.db import connections, DEFAULT_DB_ALIAS
from django.conf import settings
from django.core import management

from freppledb.execute.commands import printWelcome, logProgress, createPlan, exportPlan, logMessage


if __name__ == "__main__":
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

  # Make sure the debug flag is not set!
  # When it is set, the Django database wrapper collects a list of all sql
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

  # Initialization
  printWelcome(database=db)
  logProgress(1, db)

  # Detect whether the forecast module is available
  with_forecasting = 'demand_forecast' in [ a[0] for a in inspect.getmembers(frepple) ]
  if settings.DATABASES[db]['ENGINE'] != 'django.db.backends.postgresql_psycopg2':
    print("Warning: forecast module is only supported when using a PostgreSQL database")
    with_forecasting = False

  print("\nStart loading data from the database at", datetime.now().strftime("%H:%M:%S"))
  frepple.printsize()
  from freppledb.execute.load import loadfrepple
  loadfrepple(db)
  if with_forecasting:
    from freppledb.forecast.commands import loadForecast
    loadForecast(cursor)
  frepple.printsize()
  logProgress(33, db)

  if with_forecasting:
    from freppledb.forecast.commands import aggregateDemand, generateBaseline, processForecastDemand, createSolver, applyForecastAdjustments
    from freppledb.input.models import Item, Customer

    # Initialize the solver
    solver_fcst = createSolver(cursor)

    # Assure the hierarchies are up to date
    print("\nStart building hierarchies at", datetime.now().strftime("%H:%M:%S"))
    Item.rebuildHierarchy(database=db)
    Customer.rebuildHierarchy(database=db)
    logProgress(35, db)

    print("\nStart aggregating demand at", datetime.now().strftime("%H:%M:%S"))
    aggregateDemand(cursor)
    logProgress(43, db)

    print("\nStart processing forecastdemand records at", datetime.now().strftime("%H:%M:%S"))
    processForecastDemand(cursor)
    logProgress(48, db)

    print("\nStart generation of baseline forecast at", datetime.now().strftime("%H:%M:%S"))
    generateBaseline(solver_fcst, cursor)
    logProgress(53, db)

    print("\nStart applying forecast adjustments at", datetime.now().strftime("%H:%M:%S"))
    applyForecastAdjustments(cursor)
    logProgress(58, db)

    print("\nStart forecast netting at", datetime.now().strftime("%H:%M:%S"))
    solver_fcst.solve()
    frepple.printsize()
    logProgress(63, db)

  print("\nStart plan generation at", datetime.now().strftime("%H:%M:%S"))
  createPlan(db)
  frepple.printsize()
  logProgress(66, db)

  print("\nStart exporting plan to the database at", datetime.now().strftime("%H:%M:%S"))
  exportPlan(db)
  if with_forecasting:
    from freppledb.forecast.commands import exportForecast
    exportForecast(cursor)

  print("\nFinished planning at", datetime.now().strftime("%H:%M:%S"))
  logProgress(100, db)

  if 'webservice' in os.environ:
    # Shut down the previous quoting server
    # The previous order quoting service is only shut it down when the new plan
    # is ready to take over.
    #  +: Order quoting can continue while new plan is generated.
    #  +: If the new plan fails for some reason the old plan is still available.
    #  -: During the creation of the plan we have 2 processes both writing to the same log file.
    #  -: Double memory consumption.
    print("\nPrevious order quoting service shutting down at", datetime.now().strftime("%H:%M:%S"))
    # Need a hard stop to avoid messing up the log file
    management.call_command('frepple_stop_web_service', force=True, database=db)
    time.sleep(2)

    # Start the quoting service
    from freppledb.quoting.service import runWebService
    print("\nOrder quoting service starting at", datetime.now().strftime("%H:%M:%S"))
    logMessage("Order quoting service active", database=db)
    runWebService(database=db)
    logMessage(None, database=db)
    print("\nOrder quoting service finishing at", datetime.now().strftime("%H:%M:%S"))
