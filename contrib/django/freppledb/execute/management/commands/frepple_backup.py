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
import re
import subprocess
import shutil
from datetime import datetime
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS, transaction

from freppledb.execute.models import Task
from freppledb.common.models import User
from freppledb import VERSION


class Command(BaseCommand):
  help = '''
  This command creates a database dump of the frePPLe database.

  It also removes dumps older than a month to limit the disk space usage.
  If you want to keep dumps for a longer period of time, you'll need to
  copy the dumps to a different location.

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
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS,
      help='Nominates a specific database to backup'
      ),
    make_option(
      '--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'
      ),
    )

  requires_system_checks = False

  def get_version(self):
    return VERSION

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
        if task.started or task.finished or task.status != "Waiting" or task.name != 'backup database':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='backup database', submitted=now, started=now, status='0%', user=user)

      # Choose the backup file name
      backupfile = now.strftime("database.%s.%%Y%%m%%d.%%H%%M%%S.dump" % database)
      task.message = 'Backup to file %s' % backupfile
      task.save(using=database)
      transaction.commit(using=database)

      # Run the backup command
      if settings.DATABASES[database]['ENGINE'] == 'django.db.backends.sqlite3':
        # SQLITE
        shutil.copy2(settings.DATABASES[database]['NAME'], os.path.abspath(os.path.join(settings.FREPPLE_LOGDIR, backupfile)))
      elif settings.DATABASES[database]['ENGINE'] == 'django.db.backends.postgresql_psycopg2':
        # POSTGRESQL
        # Commenting the next line is a little more secure, but requires you to create a .pgpass file.
        os.environ['PGPASSWORD'] = settings.DATABASES[database]['PASSWORD']
        args = [
          "pg_dump",
          "-b", "-w",
          '--username=%s' % settings.DATABASES[database]['USER'],
          '--file=%s' % os.path.abspath(os.path.join(settings.FREPPLE_LOGDIR, backupfile))
          ]
        if settings.DATABASES[database]['HOST']:
          args.append("--host=%s" % settings.DATABASES[database]['HOST'])
        if settings.DATABASES[database]['PORT']:
          args.append("--port=%s " % settings.DATABASES[database]['PORT'])
        args.append(settings.DATABASES[database]['NAME'])
        ret = subprocess.call(args)
        if ret:
          raise Exception("Run of run pg_dump failed")
      else:
        raise Exception('Databasebackup command not supported for database engine %s' % settings.DATABASES[database]['ENGINE'])

      # Task update
      task.status = '99%'
      task.save(using=database)
      transaction.commit(using=database)

      # Delete backups older than a month
      pattern = re.compile("database.*.*.*.dump")
      for f in os.listdir(settings.FREPPLE_LOGDIR):
        if os.path.isfile(os.path.join(settings.FREPPLE_LOGDIR, f)):
          # Note this is NOT 100% correct on UNIX. st_ctime is not alawys the creation date...
          created = datetime.fromtimestamp(os.stat(os.path.join(settings.FREPPLE_LOGDIR, f)).st_ctime)
          if pattern.match(f) and (now - created).days > 31:
            try:
              os.remove(os.path.join(settings.FREPPLE_LOGDIR, f))
            except:
              pass

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
