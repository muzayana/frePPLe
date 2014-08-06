#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
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
from django.utils.importlib import import_module
from django.conf import settings

from freppledb.common.models import User
from freppledb.execute.models import Task


class Command(BaseCommand):
  option_list = BaseCommand.option_list + (
    make_option('--user', dest='user', type='string',
      help='User running the command'),
    make_option('--constraint', dest='constraint', type='choice',
      choices=['0','1','2','3','4','5','6','7','8','9','10','11','12','13','14','15'], default='15',
      help='Constraints to be considered: 1=lead time, 2=material, 4=capacity, 8=release fence'),
    make_option('--plantype', dest='plantype', type='choice', choices=['1','2'],
      default='1', help='Plan type: 1=constrained, 2=unconstrained'),
    make_option('--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates a specific database to load data from and export results into'),
    make_option('--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'),
    make_option('--env', dest='env', type='string',
      help='A comma separated list of extra settings passed as environment variables to the engine'),
  )
  help = "Runs frePPLe to generate a plan"

  requires_model_validation = False

  def handle(self, **options):
    # Pick up the options
    if 'database' in options:
      database = options['database'] or DEFAULT_DB_ALIAS
    else:
      database = DEFAULT_DB_ALIAS
    if not database in settings.DATABASES:
      raise CommandError("No database settings known for '%s'" % database )
    if 'user' in options and options['user']:
      try:
        user = User.objects.all().using(database).get(username=options['user'])
      except:
        raise CommandError("User '%s' not found" % options['user'] )
    else:
      user = None

    now = datetime.now()
    transaction.enter_transaction_management(using=database)
    task = None
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try:
          task = Task.objects.all().using(database).get(pk=options['task'])
        except:
          raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'generate plan':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='generate plan', submitted=now, started=now, status='0%', user=user)

      # Validate options
      if 'constraint' in options:
        constraint = int(options['constraint'])
        if constraint < 0 or constraint > 15:
          raise ValueError("Invalid constraint: %s" % options['constraint'])
      else:
        constraint = 15
      if 'plantype' in options:
        plantype = int(options['plantype'])
        if plantype < 1 or plantype > 2:
          raise ValueError("Invalid plan type: %s" % options['plantype'])
      else:
        plantype = 1
      if options['env']:
        task.arguments = "--constraint=%d --plantype=%d --env=%s" % (constraint, plantype, options['env'])
        for i in options['env'].split(','):
          j = i.split('=')
          if len(j) == 1:
            os.environ[j[0]] = '1'
          else:
            os.environ[j[0]] = j[1]
      else:
        task.arguments = "--constraint=%d --plantype=%d" % (constraint, plantype)

      # Log task
      task.save(using=database)
      transaction.commit(using=database)

      # Locate commands.py
      cmd = None
      for app in settings.INSTALLED_APPS:
        mod = import_module(app)
        if os.path.exists(os.path.join(os.path.dirname(mod.__file__),'commands.py')):
          cmd = os.path.join(os.path.dirname(mod.__file__),'commands.py')
          break
      if not cmd:
        raise Exception("Can't locate commands.py")

      # Execute
      os.environ['FREPPLE_PLANTYPE'] = str(plantype)
      os.environ['FREPPLE_CONSTRAINT'] = str(constraint)
      os.environ['FREPPLE_TASKID'] = str(task.id)
      os.environ['FREPPLE_DATABASE'] = database
      os.environ['PATH'] = settings.FREPPLE_HOME + os.pathsep + os.environ['PATH'] + os.pathsep + settings.FREPPLE_APP
      if os.path.isfile(os.path.join(settings.FREPPLE_HOME,'libfrepple.so')):
        os.environ['LD_LIBRARY_PATH'] = settings.FREPPLE_HOME
      if 'DJANGO_SETTINGS_MODULE' not in os.environ:
        os.environ['DJANGO_SETTINGS_MODULE'] = 'freppledb.settings'
      if os.path.exists(os.path.join(settings.FREPPLE_HOME,'python27.zip')):
        # For the py2exe executable
        os.environ['PYTHONPATH'] = os.path.join(settings.FREPPLE_HOME,'python27.zip') + os.pathsep + os.path.normpath(settings.FREPPLE_APP)
      else:
        # Other executables
        os.environ['PYTHONPATH'] = os.path.normpath(settings.FREPPLE_APP)
      ret = os.system('frepple "%s"' % cmd.replace('\\','\\\\'))
      if ret != 0 and ret != 2:
        # Return code 0 is a successful run
        # Return code is 2 is a run cancelled by a user. That's shown in the status field.
        raise Exception('Failed with exit code %d' % ret)

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
      if task:
        task.save(using=database)
      try:
        transaction.commit(using=database)
      except:
        pass
      transaction.leave_transaction_management(using=database)
