#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db.models import signals
from django.db import DEFAULT_DB_ALIAS


def createReportPermissions(app, created_models, verbosity, db=DEFAULT_DB_ALIAS, **kwargs):
  # Create the report permissions for the single menu instance we know about.
  if db == DEFAULT_DB_ALIAS:
    appname = app.__name__.replace(".models","")
    from freppledb.menu import menu
    menu.createReportPermissions(appname)
    from freppledb.common.dashboard import Dashboard
    Dashboard.createWidgetPermissions(appname)


signals.post_syncdb.connect(createReportPermissions)
