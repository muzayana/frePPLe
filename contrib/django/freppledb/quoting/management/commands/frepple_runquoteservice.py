#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.db import DEFAULT_DB_ALIAS
from django.conf import settings

import freppledb.quoting


class Command(BaseCommand):
  option_list = BaseCommand.option_list + (
    make_option('--user', dest='user', type='string',
      help='User running the command'),
    make_option('--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates a specific database to load data from'),
  )
  help = "Runs the frePPLe order quoting server"

  requires_model_validation = False

  def handle(self, **options):
    # Pick up the options
    if 'nonfatal' in options: nonfatal = options['nonfatal']
    else: nonfatal = False
    if 'database' in options: database = options['database'] or DEFAULT_DB_ALIAS
    else: database = DEFAULT_DB_ALIAS
    if not database in settings.DATABASES.keys():
      raise CommandError("No database settings known for '%s'" % database )

    try:
      # Execute
      os.environ['FREPPLE_HOME'] = settings.FREPPLE_HOME.replace('\\','\\\\')
      os.environ['FREPPLE_APP'] = settings.FREPPLE_APP
      os.environ['FREPPLE_DATABASE'] = database
      os.environ['PATH'] = settings.FREPPLE_HOME + os.pathsep + os.environ['PATH'] + os.pathsep + settings.FREPPLE_APP
      os.environ['LD_LIBRARY_PATH'] = settings.FREPPLE_HOME
      if 'DJANGO_SETTINGS_MODULE' not in os.environ.keys():
        os.environ['DJANGO_SETTINGS_MODULE'] = 'freppledb.settings'
      if os.path.exists(os.path.join(os.environ['FREPPLE_HOME'],'python27.zip')):
        # For the py2exe executable
        os.environ['PYTHONPATH'] = os.path.join(os.environ['FREPPLE_HOME'],'python27.zip') + os.pathsep + os.path.normpath(os.environ['FREPPLE_APP'])
      else:
        # Other executables
        os.environ['PYTHONPATH'] = os.path.normpath(os.environ['FREPPLE_APP'])
      ret = os.system('frepple "%s"' % os.path.join(os.path.dirname(freppledb.quoting.__file__),'commands.py').replace('\\','\\\\'))
      if ret:
        raise Exception('Exit code of the batch run is %d' % ret)

    except Exception as e:
      if nonfatal: raise e
      else: raise CommandError(e)
