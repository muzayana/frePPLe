#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db.models import signals
from django.db import DEFAULT_DB_ALIAS

from freppledb.menu import menu

def createReportPermissions(app, created_models, verbosity, db=DEFAULT_DB_ALIAS, **kwargs):
  # Create the report permissions for the single menu instance we know about.
  if db == DEFAULT_DB_ALIAS:
    menu.createReportPermissions(app.__name__.replace(".models",""))

signals.post_syncdb.connect(createReportPermissions)
