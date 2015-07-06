#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from datetime import datetime

from django.db import migrations


def createAdminUser(apps, schema_editor):
  from django.contrib.auth import get_user_model
  User = get_user_model()
  usr = User.objects.create_superuser('admin', 'your@company.com', 'admin')
  usr.first_name = 'admin'
  usr.last_name = 'admin'
  usr.date_joined = datetime(2000, 1, 1)
  usr.horizontype = True
  usr.horizonlength = 6
  usr.horizonunit = "month"
  usr.language = "auto"
  usr.save()


class Migration(migrations.Migration):
  dependencies = [
      ('common', '0001_initial'),
      ('execute', '0001_initial'),
  ]

  operations = [
      migrations.RunPython(createAdminUser),
  ]
