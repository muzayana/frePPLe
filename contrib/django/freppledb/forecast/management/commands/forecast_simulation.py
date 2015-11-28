#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime
from optparse import make_option

from django.core import management
from django.core.management.base import BaseCommand, CommandError
from django.db import DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.common.models import User, Parameter
from freppledb.execute.models import Task
from freppledb.input.models import CalendarBucket


class Command(BaseCommand):
  option_list = BaseCommand.option_list + (
    make_option(
      '--user', dest='user', type='string',
      help='User running the command'
      ),
    make_option(
      '--history', dest='history', type='int', default='12',
      help='Number of periods in the past to step back'
      ),
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS,
      help='Nominates a specific database to load data from and export results into'
      ),
    make_option(
      '--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'
      ),
  )
  help = '''
  Runs a simulation to measure the historical forecast accuracy.

  The simulation sets back to "current date - N periods" ago. It computes the
  forecast, and compares the actual orders and forecast in period "current
  date - N + 1".
  The process is then repeated for period "current date - N + 1" period to
  estimate the forecast error in period "current date - N + 2".
  The final output is thus an estimation of the forecast accuracy over time.

  Warning: During the simulation, the current_date parameter is changing.
  No other calculation processes and user interface actions should be run
  during the simulation, as they would incorrectly use the manipulated
  current_date value.
  '''

  requires_system_checks = False

  def handle(self, **options):
    # Pick up the options
    if 'database' in options:
      database = options['database'] or DEFAULT_DB_ALIAS
    else:
      database = DEFAULT_DB_ALIAS
    if database not in settings.DATABASES:
      raise CommandError("No database settings known for '%s'" % database )
    if 'user' in options and options['user']:
      try:
        user = User.objects.all().using(database).get(username=options['user'])
      except:
        raise CommandError("User '%s' not found" % options['user'] )
    else:
      user = None

    now = datetime.now()
    task = None
    currentdate = None
    param = None
    created = False
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try:
          task = Task.objects.all().using(database).get(pk=options['task'])
        except:
          raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'forecast simulation':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='forecast simulation', submitted=now, started=now, status='0%', user=user)

      # Validate options
      if 'history' in options:
        history = int(options['history'])
        if history < 0:
          raise ValueError("Invalid history: %s" % options['history'])
        task.arguments = "--history=%d" % history
      else:
        history = 12

      # Log task
      task.save(using=database)

      # Get current date
      param, created = Parameter.objects.all().using(database).get_or_create(name='currentdate')
      currentdate = param.value
      try:
        curdate = datetime.strptime(param.value, "%Y-%m-%d %H:%M:%S")
      except:
        curdate = datetime.now().replace(microsecond=0)

      # Loop over all buckets in the simulation horizon
      cal = Parameter.getValue('forecast.calendar', database)
      bckt_list = [ i for i in CalendarBucket.objects.all().using(database).filter(calendar__name=cal, startdate__lt=curdate).order_by('startdate') ]
      bckt_list = bckt_list[-history:]
      idx = 0
      for bckt in bckt_list:
        # Start message
        task.status = "%.0f%%" % (100.0 * idx / history)
        task.message = 'Simulating period %s' % bckt.startdate.date()
        task.save(using=database)
        idx += 1

        # Update currentdate parameter
        param.value = bckt.startdate.date().strftime("%Y-%m-%d %H:%M:%S")
        param.save(using=database)

        # Run simulation
        management.call_command('frepple_run', database=database, env="noproduction,noinventory,noevaluation")

      # Task update
      task.status = 'Done'
      task.message = "Simulated from %s till %s" % (bckt_list[0].startdate.date(), bckt_list[-1].startdate.date())
      task.finished = datetime.now()

    except Exception as e:
      if task:
        task.status = 'Failed'
        task.message = '%s' % e
        task.finished = datetime.now()
      raise e

    finally:
      # Restore original current date parameter
      if created:
        Parameter.objects.all().using(database).get(name='currentdate').delete()
      else:
        param.value = currentdate
        param.save(using=database)

      # Final task status
      if task:
        task.save(using=database)
