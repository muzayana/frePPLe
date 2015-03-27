from __future__ import print_function

import os
import time
from datetime import datetime

from django.db import DEFAULT_DB_ALIAS
from django.conf import settings
from django.core import management

from freppledb.execute.commands import logMessage


if __name__ == "__main__":
  # Select database
  try:
    db = os.environ['FREPPLE_DATABASE'] or DEFAULT_DB_ALIAS
  except:
    db = DEFAULT_DB_ALIAS

  # Use the test database if we are running the test suite
  if 'FREPPLE_TEST' in os.environ:
    settings.DATABASES[db]['NAME'] = settings.DATABASES[db]['TEST_NAME']
    if 'TEST_CHARSET' in os.environ:
      settings.DATABASES[db]['CHARSET'] = settings.DATABASES[db]['TEST_CHARSET']
    if 'TEST_COLLATION' in os.environ:
      settings.DATABASES[db]['COLLATION'] = settings.DATABASES[db]['TEST_COLLATION']
    if 'TEST_USER' in os.environ:
      settings.DATABASES[db]['USER'] = settings.DATABASES[db]['TEST_USER']

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
    from freppledb.quoting.service import runWebService
    print("\nOrder quoting service starting at", datetime.now().strftime("%H:%M:%S"))
    logMessage("Order quoting service active", database=db)
    runWebService(database=db)
    logMessage(None, status='Done', database=db)
    print("\nOrder quoting service finishing at", datetime.now().strftime("%H:%M:%S"))
  elif ok:
    logMessage(None, status='Done', database=db)
