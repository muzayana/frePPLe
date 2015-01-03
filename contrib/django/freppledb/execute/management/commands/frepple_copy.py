#
# Copyright (C) 2010-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os
import shutil
from optparse import make_option
from datetime import datetime

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings

from freppledb.execute.models import Task, Scenario
from freppledb.common.models import User
from freppledb import VERSION


class Command(BaseCommand):
  help = '''
  This command copies the contents of a database into another.
  The original data in the destination database are lost.

  To use this command the following prerequisites need to be met:
    * PostgreSQL:
       - pg_dump and psql need to be in the path
       - The passwords need to be specified upfront in a file ~/.pgpass
    * SQLite:
       - none
  '''
  option_list = BaseCommand.option_list + (
    make_option(
      '--user', dest='user', type='string',
      help='User running the command'
      ),
    make_option(
      '--force', action="store_true", dest='force',
      default=False, help='Overwrite scenarios already in use'
      ),
    make_option(
      '--description', dest='description', type='string',
      help='Description of the destination scenario'
      ),
    make_option(
      '--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'
      ),
    )
  args = 'source_database destination_database'

  requires_system_checks = False

  def get_version(self):
    return VERSION


  def handle(self, *args, **options):
    # Make sure the debug flag is not set!
    # When it is set, the django database wrapper collects a list of all sql
    # statements executed and their timings. This consumes plenty of memory
    # and cpu time.
    tmp_debug = settings.DEBUG
    settings.DEBUG = False

    # Pick up options
    if 'force' in options:
      force = options['force']
    else:
      force = False
    test = 'FREPPLE_TEST' in os.environ
    if 'user' in options and options['user']:
      try:
        user = User.objects.all().get(username=options['user'])
      except:
        raise CommandError("User '%s' not found" % options['user'] )
    else:
      user = None

    # Initialize the task
    now = datetime.now()
    task = None
    if 'task' in options and options['task']:
      try:
        task = Task.objects.all().get(pk=options['task'])
      except:
        raise CommandError("Task identifier not found")
      if task.started or task.finished or task.status != "Waiting" or task.name != 'copy scenario':
        raise CommandError("Invalid task identifier")
      task.status = '0%'
      task.started = now
    else:
      task = Task(name='copy scenario', submitted=now, started=now, status='0%', user=user)
    task.save()

    # Synchronize the scenario table with the settings
    Scenario.syncWithSettings()

    # Validate the arguments
    destinationscenario = None
    try:
      if len(args) != 2:
        raise CommandError("Command takes exactly 2 arguments.")
      task.arguments = "%s %s" % (args[0], args[1])
      task.save()
      source = args[0]
      try:
        sourcescenario = Scenario.objects.get(pk=source)
      except:
        raise CommandError("No source database defined with name '%s'" % source)
      destination = args[1]
      try:
        destinationscenario = Scenario.objects.get(pk=destination)
      except:
        raise CommandError("No destination database defined with name '%s'" % destination)
      if source == destination:
        raise CommandError("Can't copy a schema on itself")
      if settings.DATABASES[source]['ENGINE'] != settings.DATABASES[destination]['ENGINE']:
        raise CommandError("Source and destination scenarios have a different engine")
      if sourcescenario.status != 'In use':
        raise CommandError("Source scenario is not in use")
      if destinationscenario.status != 'Free' and not force:
        raise CommandError("Destination scenario is not free")

      # Logging message - always logging in the default database
      destinationscenario.status = 'Busy'
      destinationscenario.save()

      # Copying the data
      if settings.DATABASES[source]['ENGINE'] == 'django.db.backends.postgresql_psycopg2':
        # Commenting the next line is a little more secure, but requires you to create a .pgpass file.
        os.environ['PGPASSWORD'] = settings.DATABASES[source]['PASSWORD']
        ret = os.system("pg_dump -c -U%s -Fp %s%s%s | psql -U%s %s%s%s" % (
          settings.DATABASES[source]['USER'],
          settings.DATABASES[source]['HOST'] and ("-h %s " % settings.DATABASES[source]['HOST']) or '',
          settings.DATABASES[source]['PORT'] and ("-p %s " % settings.DATABASES[source]['PORT']) or '',
          test and settings.DATABASES[source]['TEST']['NAME'] or settings.DATABASES[source]['NAME'],
          settings.DATABASES[destination]['USER'],
          settings.DATABASES[destination]['HOST'] and ("-h %s " % settings.DATABASES[destination]['HOST']) or '',
          settings.DATABASES[destination]['PORT'] and ("-p %s " % settings.DATABASES[destination]['PORT']) or '',
          test and settings.DATABASES[destination]['TEST']['NAME'] or settings.DATABASES[destination]['NAME'],
          ))
        if ret:
          raise Exception('Exit code of the database copy command is %d' % ret)
      elif settings.DATABASES[source]['ENGINE'] == 'django.db.backends.sqlite3':
        # A plain copy of the database file
        if test:
          shutil.copy2(settings.DATABASES[source]['TEST']['NAME'], settings.DATABASES[destination]['TEST']['NAME'])
        else:
          shutil.copy2(settings.DATABASES[source]['NAME'], settings.DATABASES[destination]['NAME'])
      else:
        raise Exception('Copy command not supported for database engine %s' % settings.DATABASES[source]['ENGINE'])

      # Update the scenario table
      destinationscenario.status = 'In use'
      destinationscenario.lastrefresh = datetime.today()
      if 'description' in options:
        destinationscenario.description = options['description']
      else:
        destinationscenario.description = "Copied from scenario '%s'" % source
      destinationscenario.save()

      # Logging message
      task.status = 'Done'
      task.finished = datetime.now()

    except Exception as e:
      if task:
        task.status = 'Failed'
        task.message = '%s' % e
        task.finished = datetime.now()
      if destinationscenario and destinationscenario.status == 'Busy':
        destinationscenario.status = 'Free'
        destinationscenario.save()
      raise e

    finally:
      if task:
        task.save()
      settings.DEBUG = tmp_debug
