#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db.models import signals, get_models
from django.db import DEFAULT_DB_ALIAS
from django.contrib.auth.models import Permission
from django.contrib.contenttypes.models import ContentType
from django.template.base import add_to_builtins

from freppledb.common import models as common_models


# Make our tags built-in, so we don't have to load them any more in our
# templates with a 'load' tag.
add_to_builtins('freppledb.common.templatetags.base_utils')


def removeDefaultPermissions(app, created_models, verbosity, db=DEFAULT_DB_ALIAS, **kwargs):
  if db != DEFAULT_DB_ALIAS:
    return
  appname = app.__name__.replace(".models", "")
  # Delete the default permissions that were created for the models
  Permission.objects.all().filter(content_type__app_label=appname, codename__startswith="change").delete()
  Permission.objects.all().filter(content_type__app_label=appname, codename__startswith="add").delete()
  Permission.objects.all().filter(content_type__app_label=appname, codename__startswith="delete").delete()


def createViewPermissions(app, created_models, verbosity, db=DEFAULT_DB_ALIAS, **kwargs):
  if db != DEFAULT_DB_ALIAS:
    return
  # Create model read permissions
  for m in get_models(app):
    p = Permission.objects.get_or_create(
          codename='view_%s' % m._meta.model_name,
          content_type=ContentType.objects.db_manager(db).get_for_model(m)
          )[0]
    p.name = 'Can view %s' % m._meta.verbose_name_raw
    p.save()


def createExtraPermissions(app, created_models, verbosity, db=DEFAULT_DB_ALIAS, **kwargs):
  if db != DEFAULT_DB_ALIAS:
    return
  # Create the report permissions for the single menu instance we know about.
  appname = app.__name__.replace(".models", "")
  from freppledb.menu import menu
  menu.createReportPermissions(appname)
  # Create widget permissions
  from freppledb.common.dashboard import Dashboard
  Dashboard.createWidgetPermissions(appname)


signals.post_syncdb.connect(createExtraPermissions)
signals.post_syncdb.connect(createViewPermissions, common_models)
