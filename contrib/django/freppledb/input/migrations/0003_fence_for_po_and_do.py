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
import freppledb.common.fields

class Migration(migrations.Migration):

  dependencies = [
    ('input', '0002_resource_for_po_and_do'),
    ]

  operations = [
    migrations.AddField(
      model_name='itemdistribution',
      name='fence',
      field=freppledb.common.fields.DurationField(help_text='Frozen fence for creating new shipments', null=True, blank=True, verbose_name='fence', decimal_places=4, max_digits=15),
    ),
    migrations.AddField(
      model_name='itemsupplier',
      name='fence',
      field=freppledb.common.fields.DurationField(help_text='Frozen fence for creating new procurements', null=True, blank=True, verbose_name='fence', decimal_places=4, max_digits=15),
    ),
  ]
