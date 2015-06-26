#
# Copyright (C) 2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the
# software is used within your company.
# You are not allowed to distribute the software, either in the form of
# source code or in the form of compiled binaries.
#

import http.client
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS

from freppledb import VERSION
from freppledb.common.models import Parameter

database = DEFAULT_DB_ALIAS


class Command(BaseCommand):
  help = '''
  This command stops the frePPLe web service if it is running.
  '''
  option_list = BaseCommand.option_list + (
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates a specific database to backup'
      ),
    make_option(
      '--force', action="store_true", dest='force',
      default=False, help='Force an immediate shutdown, rather than a graceful stop'
      ),
    )

  requires_model_validation = False

  def get_version(self):
    return VERSION

  def handle(self, **options):

    # Pick up the options
    if 'force' in options:
      force = options['force']
    else:
      force = False
    if 'database' in options:
      global database
      database = options['database'] or DEFAULT_DB_ALIAS
    if not database in settings.DATABASES:
      raise CommandError("No database settings known for '%s'" % database )

    # Connect to the url "/stop/"
    url = Parameter.getValue('quoting.service_location', database=database, default="localhost:8001")
    try:
      conn = http.client.HTTPConnection(url)
      if force:
        conn.request("GET", '/stop/?hard=1')
      else:
        conn.request("GET", '/stop/')
    except Exception:
      # The service wasn't up
      print("Web service for database '%s' wasn't running" % database )
