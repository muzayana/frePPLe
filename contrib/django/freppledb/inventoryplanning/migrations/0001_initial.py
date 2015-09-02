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
  call_command('loaddata', "parameters.json", app_label="inventoryplanning", verbosity=0)


class Migration(migrations.Migration):

  dependencies = [
    ('common', '0001_initial'),
    ('input', '0001_initial'),
  ]

  operations = [
      migrations.CreateModel(
          name='InventoryPlanning',
          fields=[
              ('source', models.CharField(max_length=20, verbose_name='source', db_index=True, blank=True, null=True)),
              ('lastmodified', models.DateTimeField(default=django.utils.timezone.now, editable=False, verbose_name='last modified', db_index=True)),
              ('buffer', models.OneToOneField(serialize=False, primary_key=True, to='input.Buffer')),
              ('roq_min_qty', models.DecimalField(max_digits=15, blank=True, verbose_name='ROQ minimum quantity', decimal_places=4, null=True)),
              ('roq_max_qty', models.DecimalField(max_digits=15, blank=True, verbose_name='ROQ maximum quantity', decimal_places=4, null=True)),
              ('roq_multiple_qty', models.DecimalField(max_digits=15, blank=True, verbose_name='ROQ multiple quantity', decimal_places=4, null=True)),
              ('roq_min_poc', models.DecimalField(max_digits=15, blank=True, verbose_name='ROQ minimum period of cover', decimal_places=4, null=True)),
              ('roq_max_poc', models.DecimalField(max_digits=15, blank=True, verbose_name='ROQ maximum period of cover', decimal_places=4, null=True)),
              ('leadtime_deviation', models.DecimalField(max_digits=15, blank=True, verbose_name='lead time deviation', decimal_places=4, null=True)),
              ('demand_deviation', models.DecimalField(max_digits=15, blank=True, verbose_name='demand deviation', decimal_places=4, null=True)),
              ('demand_distribution', models.CharField(verbose_name='demand distribution', max_length=20, blank=True, choices=[('automatic', 'Automatic'), ('normal', 'Normal'), ('poisson', 'Poisson'), ('negative binomial', 'Negative binomial')], null=True)),
              ('service_level', models.DecimalField(max_digits=15, blank=True, verbose_name='service level', decimal_places=4, null=True)),
              ('ss_min_qty', models.DecimalField(max_digits=15, blank=True, verbose_name='safety stock minimum quantity', decimal_places=4, null=True)),
              ('ss_max_qty', models.DecimalField(max_digits=15, blank=True, verbose_name='safety stock maximum quantity', decimal_places=4, null=True)),
              ('ss_multiple_qty', models.DecimalField(max_digits=15, blank=True, verbose_name='safety stock multiple quantity', decimal_places=4, null=True)),
              ('ss_min_poc', models.DecimalField(max_digits=15, blank=True, verbose_name='safety stock minimum period of cover', decimal_places=4, null=True)),
              ('ss_max_poc', models.DecimalField(max_digits=15, blank=True, verbose_name='safety stock maximum period of cover', decimal_places=4, null=True)),
              ('nostock', models.BooleanField(default=False, verbose_name='Do not stock')),
          ],
          options={
              'ordering': ['buffer'],
              'verbose_name_plural': 'inventory planning parameters',
              'db_table': 'inventory_planning',
              'verbose_name': 'inventory planning parameter',
              'abstract': False,
          },
      ),
      migrations.RunPython(loadParameters),
  ]
