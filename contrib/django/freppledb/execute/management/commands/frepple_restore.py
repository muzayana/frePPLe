#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os.path
import subprocess
from datetime import datetime
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS

from freppledb.execute.models import Task
from freppledb.common.models import User
from freppledb import VERSION


class Command(BaseCommand):
  help = '''
  This command creates a database dump of the frePPLe database.

  The psql command needs to be in the path, otherwise this command
  will fail.
  '''
  option_list = BaseCommand.option_list + (
    make_option(
      '--user', dest='user', type='string',
      help='User running the command'
      ),
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates a specific database to backup'
      ),
    make_option(
      '--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'
      ),
    )

  requires_system_checks = False

  def get_version(self):
    return VERSION

  def handle(self, *args, **options):

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
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try:
          task = Task.objects.all().using(database).get(pk=options['task'])
        except:
          raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'restore database':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='restore database', submitted=now, started=now, status='0%', user=user)
      task.arguments = args and args[0] or None
      task.save(using=database)

      # Validate options
      if not args:
        raise CommandError("No dump file specified")
      if not os.path.isfile(os.path.join(settings.FREPPLE_LOGDIR, args[0])):
        raise CommandError("Dump file not found")

      # Run the restore command
      # Commenting the next line is a little more secure, but requires you to create a .pgpass file.
      os.environ['PGPASSWORD'] = settings.DATABASES[database]['PASSWORD']
      cmd = [ "psql", '--username=%s' % settings.DATABASES[database]['USER'] ]
      if settings.DATABASES[database]['HOST']:
        cmd.append("--host=%s" % settings.DATABASES[database]['HOST'])
      if settings.DATABASES[database]['PORT']:
        cmd.append("--port=%s " % settings.DATABASES[database]['PORT'])
      cmd.append(settings.DATABASES[database]['NAME'])
      cmd.append('<%s' % os.path.abspath(os.path.join(settings.FREPPLE_LOGDIR, args[0])))
      ret = subprocess.call(cmd, shell=True)  # Shell needs to be True in order to interpret the < character
      if ret:
        raise Exception("Run of run psql failed")

      # Task update
      # We need to recreate a new task record, since the previous one is lost during the restoration.
      task = Task(
        name='restore database', submitted=task.submitted, started=task.started,
        arguments=task.arguments, status='Done', finished=datetime.now(),
        user=task.user
        )

    except Exception as e:
      if task:
        task.status = 'Failed'
        task.message = '%s' % e
        task.finished = datetime.now()
      raise e

    finally:
      # Commit it all, even in case of exceptions
      if task:
        task.save(using=database)
