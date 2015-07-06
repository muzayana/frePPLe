#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from django.core.management import call_command
from django.db import migrations


def loadParameters(apps, schema_editor):
  call_command('loaddata', "parameters.json", app_label="quoting", verbosity=0)


class Migration(migrations.Migration):

  dependencies = [
    ('common', '0001_initial'),
  ]

  operations = [
    migrations.RunPython(loadParameters),
  ]
