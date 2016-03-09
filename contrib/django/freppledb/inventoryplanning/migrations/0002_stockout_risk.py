#
# Copyright (C) 2016 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db import models, migrations


class Migration(migrations.Migration):

  dependencies = [
    ('inventoryplanning', '0001_initial'),
  ]

  operations = [
    migrations.AddField(
        model_name='inventoryplanningoutput',
        name='stockoutrisk',
        field=models.DecimalField(max_digits=15, verbose_name='stockout risk', null=True, decimal_places=4),
    ),
  ]
