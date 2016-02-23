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
    ('input', '0001_initial'),
  ]

  operations = [
    migrations.AddField(
      model_name='itemdistribution',
      name='resource',
      field=models.ForeignKey(verbose_name='resource', help_text='Resource to model the distribution capacity', null=True, related_name='itemdistributions', to='input.Resource', blank=True),
    ),
    migrations.AddField(
      model_name='itemdistribution',
      name='resource_qty',
      field=models.DecimalField(verbose_name='resource quantity', help_text='Resource capacity consumed per distributed unit', null=True, decimal_places=4, blank=True, max_digits=15, default='1.0'),
    ),
    migrations.AddField(
      model_name='itemsupplier',
      name='resource',
      field=models.ForeignKey(verbose_name='resource', help_text='Resource to model the supplier capacity', null=True, related_name='itemsuppliers', to='input.Resource', blank=True),
    ),
    migrations.AddField(
      model_name='itemsupplier',
      name='resource_qty',
      field=models.DecimalField(verbose_name='resource quantity', help_text='Resource capacity consumed per purchased unit', null=True, decimal_places=4, blank=True, max_digits=15, default='1.0'),
    ),
  ]
