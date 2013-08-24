#
# Copyright (C) 2011-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os
from datetime import datetime
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.db import transaction, DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.execute.models import Task
from freppledb.common.models import User
from freppledb import VERSION


class Command(BaseCommand):
  help = "Loads an XML file into the frePPLe database"
  option_list = BaseCommand.option_list + (
    make_option('--user', dest='user', type='string',
      help='User running the command'),
    make_option('--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates a specific database to load data from and export results into'),
    make_option('--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'),
  )
  args = 'XMLfile(s)'

  requires_model_validation = False

  def get_version(self):
    return VERSION

  def handle(self, *args, **options):
    # Pick up the options
    if 'database' in options: database = options['database'] or DEFAULT_DB_ALIAS
    else: database = DEFAULT_DB_ALIAS
    if not database in settings.DATABASES.keys():
      raise CommandError("No database settings known for '%s'" % database )
    if 'user' in options and options['user']:
      try: user = User.objects.all().using(database).get(username=options['user'])
      except: raise CommandError("User '%s' not found" % options['user'] )
    else:
      user = None

    now = datetime.now()
    transaction.enter_transaction_management(using=database)
    transaction.managed(True, using=database)
    task = None
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try: task = Task.objects.all().using(database).get(pk=options['task'])
        except: raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'load XML file':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='load XML file', submitted=now, started=now, status='0%', user=user)
      task.arguments = ' '.join(['"%s"' % i for i in args])
      task.save(using=database)
      transaction.commit(using=database)

      if not args:
        raise CommandError("No XML input file given")

      # Execute
      # TODO: if frePPLe is available as a module, we don't really need to spawn another process.
      os.environ['FREPPLE_HOME'] = settings.FREPPLE_HOME.replace('\\','\\\\')
      os.environ['FREPPLE_APP'] = settings.FREPPLE_APP
      os.environ['FREPPLE_DATABASE'] = database
      os.environ['PATH'] = settings.FREPPLE_HOME + os.pathsep + os.environ['PATH'] + os.pathsep + settings.FREPPLE_APP
      os.environ['LD_LIBRARY_PATH'] = settings.FREPPLE_HOME
      if 'DJANGO_SETTINGS_MODULE' not in os.environ.keys():
        os.environ['DJANGO_SETTINGS_MODULE'] = 'freppledb.settings'
      if os.path.exists(os.path.join(os.environ['FREPPLE_HOME'],'python27.zip')):
        # For the py2exe executable
        os.environ['PYTHONPATH'] = os.path.join(os.environ['FREPPLE_HOME'],'python27.zip') + ';' + os.path.normpath(os.environ['FREPPLE_APP'])
      else:
        # Other executables
        os.environ['PYTHONPATH'] = os.path.normpath(os.environ['FREPPLE_APP'])
      cmdline = [ '"%s"' % i for i in args ]
      cmdline.insert(0, 'frepple')
      cmdline.append( '"%s"' % os.path.join(settings.FREPPLE_APP,'freppledb','execute','loadxml.py') )
      ret = os.system(' '.join(cmdline))
      if ret: raise Exception('Exit code of the batch run is %d' % ret)

      # Task update
      task.status = 'Done'
      task.finished = datetime.now()

    except Exception as e:
      if task:
        task.status = 'Failed'
        task.message = '%s' % e
        task.finished = datetime.now()
      raise e

    finally:
      if task: task.save(using=database)
      try: transaction.commit(using=database)
      except: pass
      transaction.leave_transaction_management(using=database)
