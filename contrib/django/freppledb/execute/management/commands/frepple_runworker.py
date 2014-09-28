#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import logging
import time
from datetime import datetime, timedelta
from threading import Thread
from optparse import make_option

from django.db import DEFAULT_DB_ALIAS
from django.core import management
from django.core.management.base import BaseCommand, CommandError
from django.conf import settings

from freppledb import VERSION
from freppledb.common.models import Parameter
from freppledb.execute.models import Task


logger = logging.getLogger(__name__)


class WorkerAlive(Thread):
  def __init__(self, database=DEFAULT_DB_ALIAS):
    self.database = database
    Thread.__init__(self)
    self.daemon = True

  def run(self):
    while True:
      p = Parameter.objects.all().using(self.database).get_or_create(pk='Worker alive')[0]
      p.value = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
      p.save(update_fields=['value'])
      time.sleep(5)


def checkActive(database=DEFAULT_DB_ALIAS):
    try:
      p = Parameter.objects.all().using(database).get(pk='Worker alive')
      return datetime.now() - datetime.strptime(p.value, "%Y-%m-%d %H:%M:%S") <= timedelta(0, 5)
    except:
      return False


class Command(BaseCommand):
  help = '''Processes the job queue of a database.
    The command is intended only to be used internally by frePPLe, not by an API or user.
    '''
  option_list = BaseCommand.option_list + (
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates a specific database to load data from and export results into'
      ),
    make_option(
      '--continuous', action="store_true", dest='continuous',
      default=False, help='Keep the worker alive after the queue is empty'
      ),
  )
  requires_model_validation = False

  def get_version(self):
    return VERSION


  def handle(self, *args, **options):
    # Pick up the options
    if 'database' in options:
      database = options['database'] or DEFAULT_DB_ALIAS
    else:
      database = DEFAULT_DB_ALIAS
    if not database in settings.DATABASES:
      raise CommandError("No database settings known for '%s'" % database )
    if 'continuous' in options:
      continuous = options['continuous']
    else:
      continuous = False

    # Check if a worker already exists
    if checkActive(database):
      logger.info("Worker process already active")
      return

    # Spawn a worker-alive thread
    WorkerAlive(database).start()

    # Process the queue
    logger.info("Worker starting to process jobs in the queue")
    while True:
      try:
        task = Task.objects.all().using(database).filter(status='Waiting').order_by('id')[0]
      except:
        # No more tasks found
        if continuous:
          time.sleep(5)
          continue
        else:
          break
      try:
        logger.info("starting task %d at %s" % (task.id, datetime.now()))
        background = False
        # A
        if task.name == 'generate plan':
          kwargs = {}
          for i in task.arguments.split():
            j = i.split('=')
            if len(j) > 1:
              kwargs[j[0][2:]] = j[1]
            else:
              kwargs[j[0][2:]] = True
          if 'background' in kwargs:
            background = True
          management.call_command('frepple_run', database=database, task=task.id, **kwargs)
        # B
        elif task.name == 'generate model':
          args = {}
          for i in task.arguments.split():
            key, val = i.split('=')
            args[key[2:]] = val
          management.call_command('frepple_flush', database=database)
          management.call_command('frepple_createmodel', database=database, task=task.id, verbosity=0, **args)
        # C
        elif task.name == 'empty database':
          # Erase the database contents
          args = {}
          if task.arguments:
            for i in task.arguments.split():
              key, val = i.split('=')
              args[key[2:]] = val
          management.call_command('frepple_flush', database=database, task=task.id, **args)
        # D
        elif task.name == 'load dataset':
          args = task.arguments.split()
          management.call_command('loaddata', *args, verbosity=0, database=database, task=task.id)
        # E
        elif task.name == 'copy scenario':
          args = task.arguments.split()
          management.call_command('frepple_copy', args[0], args[1], force=True, task=task.id)
        # F
        elif task.name == 'backup database':
          management.call_command('frepple_backup', database=database, task=task.id)
        # G
        elif task.name == 'generate buckets':
          management.call_command('frepple_createbuckets', database=database, task=task.id)
        # J
        elif task.name == 'Openbravo import' and 'freppledb.openbravo' in settings.INSTALLED_APPS:
          args = {}
          for i in task.arguments.split():
            key, val = i.split('=')
            args[key[2:]] = val
          management.call_command('openbravo_import', database=database, task=task.id, verbosity=0, **args)
        # K
        elif task.name == 'Openbravo export' and 'freppledb.openbravo' in settings.INSTALLED_APPS:
          management.call_command('openbravo_export', database=database, task=task.id, verbosity=0)
        else:
          logger.error('Task %s not recognized' % task.name)
        # Read the task again from the database and update.
        task = Task.objects.all().using(database).get(pk=task.id)
        if task.status not in ('Done', 'Failed') or not task.finished or not task.started:
          now = datetime.now()
          if not task.started:
            task.started = now
          if not background:
            if not task.finished:
              task.finished = now
            if task.status not in ('Done', 'Failed'):
              task.status = 'Done'
          task.save(using=database)
        logger.info("finished task %d at %s: success" % (task.id, datetime.now()))
      except Exception as e:
        task.status = 'Failed'
        now = datetime.now()
        if not task.started:
          task.started = now
        task.finished = now
        task.message = str(e)
        task.save(using=database)
        logger.info("finished task %d at %s: failed" % (task.id, datetime.now()))
    # Remove the parameter again
    try:
      Parameter.objects.all().using(self.database).get(pk='Worker alive').delete()
    except:
      pass
    # Exit
    logger.info("Worker finished all jobs in the queue and exits")
