#
# Copyright (C) 2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the
# software is used within your company.
# You are not allowed to distribute the software, either in the form of
# source code or in the form of compiled binaries.
#
# file : $URL: file:///C:/Users/Johan/Dropbox/SVNrepository/frepple/addon/contrib/django/freppledb_extra/management/commands/databasebackup.py $
# revision : $LastChangedRevision: 493 $  $LastChangedBy: Johan $
# date : $LastChangedDate: 2013-05-01 17:50:02 +0200 (Wed, 01 May 2013) $

import os, re, subprocess
from datetime import datetime
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS

from freppledb.execute.models import log
from freppledb import VERSION

database = DEFAULT_DB_ALIAS


class Command(BaseCommand):
  help = '''
  This command creates a database dump of the frePPLe database.
  TODO backup only supports postgresql now.  Need to add SQLITE, MYSQL and ORACLE.
  '''
  option_list = BaseCommand.option_list + (
      make_option('--user', dest='user', type='string',
        help='User running the command'),
      make_option('--database', action='store', dest='database',
        default=DEFAULT_DB_ALIAS, help='Nominates a specific database to backup'),
      make_option('--nonfatal', action="store_true", dest='nonfatal',
        default=False, help='Dont abort the execution upon an error'),
      )

  requires_model_validation = False

  def get_version(self):
    return VERSION

  def handle(self, **options):

    # Pick up the options
    if 'user' in options: user = options['user']
    else: user = ''
    if 'database' in options:
      global database
      database = options['database'] or DEFAULT_DB_ALIAS
    if not database in settings.DATABASES.keys():
      raise CommandError("No database settings known for '%s'" % database )
    if 'nonfatal' in options: nonfatal = options['nonfatal']
    else: nonfatal = False

    try:
      # Run a PG_DUMP process
      now = datetime.now()
      args = ["pg_dump", "-b", "-w", '--username=%s' % settings.DATABASES[database]['USER']]
      if settings.DATABASES[database]['HOST']:
        args.append("--host=%s" % settings.DATABASES[database]['HOST'])
      if settings.DATABASES[database]['PORT']:
        args.append("--port=%s " % settings.DATABASES[database]['PORT'])
      args.append('--file=%s' % os.path.abspath(os.path.join(settings.FREPPLE_LOGDIR,now.strftime("database.%Y%m%d.%H%M%S.dump"))))
      args.append(settings.DATABASES[database]['NAME'])
      ret = subprocess.call(args)
      if ret: raise Exception("Couldn't create a database dump")

      # Delete backups older than a month
      pattern = re.compile("database.*.*.dump")
      for f in os.listdir(settings.FREPPLE_LOGDIR):
        if os.path.isfile(os.path.join(settings.FREPPLE_LOGDIR,f)):
          # Note this is NOT 100% correct on UNIX. st_ctime is not alawys the creation date...
          created = datetime.fromtimestamp(os.stat(os.path.join(settings.FREPPLE_LOGDIR,f)).st_ctime)
          if pattern.match(f) and  (now - created).days > 31:
            try: os.remove(os.path.join(settings.FREPPLE_LOGDIR,f))
            except: pass

      # Logging message
      log(category='BACKUP', theuser=user,
        message="Successfully created a database backup").save(using=database)

    except Exception as e:
      try:
        log(category='BACKUP', theuser=user,
          message="Failure creating a database backup").save(using=database)
      except: pass
      if nonfatal: raise e
      else: raise CommandError(e)
