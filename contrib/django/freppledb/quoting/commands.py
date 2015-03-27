import os
import time
from datetime import datetime

from django.db import DEFAULT_DB_ALIAS
from django.conf import settings
from django.core import management

from freppledb.execute.commands import logMessage


def getConnection(dbname):
  '''
  Returns a PostgreSQL connection string used by the core engine to
  connect to the database.
  http://www.postgresql.org/docs/9.3/static/libpq-connect.html#LIBPQ-PARAMKEYWORDS
  '''
  res = [
    "client_encoding=utf-8",
    "connect_timeout=10"
    ]
  if settings.DATABASES[db]['NAME']:
    res.append("dbname=%s" % settings.DATABASES[db]['NAME'])
  if settings.DATABASES[db]['USER']:
    res.append("user=%s" % settings.DATABASES[db]['USER'])
  if settings.DATABASES[db]['PASSWORD']:
    res.append("password=%s" % settings.DATABASES[db]['PASSWORD'])
  if settings.DATABASES[db]['HOST']:
    res.append("host=%s" % settings.DATABASES[db]['HOST'])
  if settings.DATABASES[db]['PORT']:
    res.append("port=%s" % settings.DATABASES[db]['PORT'])
  return " ".join(res)


if __name__ == "__main__":
  # Select database
  try:
    db = os.environ['FREPPLE_DATABASE'] or DEFAULT_DB_ALIAS
  except:
    db = DEFAULT_DB_ALIAS

  # Use the test database if we are running the test suite
  if 'FREPPLE_TEST' in os.environ:
    settings.DATABASES[db]['NAME'] = settings.DATABASES[db]['TEST']['NAME']

  # Generate plan as usual
  ok = True
  try:
    from freppledb.forecast.commands import generate_plan
    generate_plan()
  except Exception as e:
    logMessage(str(e), status='Failed', database=db)
    ok = False
    raise

  if 'webservice' in os.environ and ok:
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
    print("\nOrder quoting service starting at", datetime.now().strftime("%H:%M:%S"))
    logMessage("Order quoting service active", database=db)
    # TODO Uncomment the next section to use the OLD cherrypy web service
    #from freppledb.quoting.service import runWebService
    #runWebService(database=db)
    frepple.runWebServer(
      document_root = ".",
      listening_ports = "8001",
      num_threads = "10",
      enable_directory_listing = "no",
      request_timeout_ms = "7200000", # 2 hours timeout
      access_log_file = "server_access.log",
      error_log_file = "server_error.log",
      max_websocket_clients = "20",
      secret_key = settings.SECRET_KEY,
      database_connection=getConnection(db)
      )
    logMessage(None, status='Done', database=db)
    print("\nOrder quoting service finishing at", datetime.now().strftime("%H:%M:%S"))
  elif ok:
    logMessage(None, status='Done', database=db)
