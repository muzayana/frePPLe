#
# Copyright (C) 2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db import DEFAULT_DB_ALIAS
from django.db.models import get_model, signals
from django.contrib.auth.models import Permission

from freppledb.output import models as output_app

def removeDefaultPermissions(app, created_models, verbosity, db=DEFAULT_DB_ALIAS, **kwargs):
  # Delete the default permissions that were created for the models in the output app
  Permission.objects.all().filter(content_type__app_label="output", codename__startswith="change").delete()
  Permission.objects.all().filter(content_type__app_label="output", codename__startswith="add").delete()
  Permission.objects.all().filter(content_type__app_label="output", codename__startswith="delete").delete()

signals.post_syncdb.connect(removeDefaultPermissions, output_app)
