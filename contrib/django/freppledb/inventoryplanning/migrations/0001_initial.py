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
from django.db import models, migrations
import django.utils.timezone


def loadParameters(apps, schema_editor):
  call_command('loaddata', "parameters.json", app_label="inventoryplanning", verbosity=0)


class Migration(migrations.Migration):

    dependencies = [
        ('input', '0001_initial'),
    ]

    operations = [
        migrations.CreateModel(
            name='InventoryPlanning',
            fields=[
                ('source', models.CharField(null=True, blank=True, verbose_name='source', db_index=True, max_length=300)),
                ('lastmodified', models.DateTimeField(editable=False, db_index=True, verbose_name='last modified', default=django.utils.timezone.now)),
                ('buffer', models.OneToOneField(serialize=False, primary_key=True, to='input.Buffer')),
                ('roq_min_qty', models.DecimalField(null=True, max_digits=15, verbose_name='ROQ minimum quantity', blank=True, decimal_places=4)),
                ('roq_max_qty', models.DecimalField(null=True, max_digits=15, verbose_name='ROQ maximum quantity', blank=True, decimal_places=4)),
                ('roq_multiple_qty', models.DecimalField(null=True, max_digits=15, verbose_name='ROQ multiple quantity', blank=True, decimal_places=4)),
                ('roq_min_poc', models.DecimalField(null=True, max_digits=15, verbose_name='ROQ minimum period of cover', blank=True, decimal_places=4)),
                ('roq_max_poc', models.DecimalField(null=True, max_digits=15, verbose_name='ROQ maximum period of cover', blank=True, decimal_places=4)),
                ('leadtime_deviation', models.DecimalField(null=True, max_digits=15, verbose_name='lead time deviation', blank=True, decimal_places=4)),
                ('demand_deviation', models.DecimalField(null=True, max_digits=15, verbose_name='demand deviation', blank=True, decimal_places=4)),
                ('demand_distribution', models.CharField(null=True, choices=[('automatic', 'Automatic'), ('normal', 'Normal'), ('poisson', 'Poisson'), ('negative binomial', 'Negative binomial')], verbose_name='demand distribution', blank=True, max_length=20)),
                ('service_level', models.DecimalField(null=True, max_digits=15, verbose_name='service level', blank=True, decimal_places=4)),
                ('ss_min_qty', models.DecimalField(null=True, max_digits=15, verbose_name='safety stock minimum quantity', blank=True, decimal_places=4)),
                ('ss_max_qty', models.DecimalField(null=True, max_digits=15, verbose_name='safety stock maximum quantity', blank=True, decimal_places=4)),
                ('ss_multiple_qty', models.DecimalField(null=True, max_digits=15, verbose_name='safety stock multiple quantity', blank=True, decimal_places=4)),
                ('ss_min_poc', models.DecimalField(null=True, max_digits=15, verbose_name='safety stock minimum period of cover', blank=True, decimal_places=4)),
                ('ss_max_poc', models.DecimalField(null=True, max_digits=15, verbose_name='safety stock maximum period of cover', blank=True, decimal_places=4)),
                ('nostock', models.BooleanField(verbose_name='Do not stock', default=False)),
            ],
            options={
                'db_table': 'inventoryplanning',
                'ordering': ['buffer'],
                'verbose_name_plural': 'inventory planning parameters',
                'verbose_name': 'inventory planning parameter',
                'abstract': False,
            },
        ),
        migrations.CreateModel(
            name='InventoryPlanningOutput',
            fields=[
                ('buffer', models.OneToOneField(serialize=False, primary_key=True, to='input.Buffer')),
                ('leadtime', models.DurationField(null=True, verbose_name='lead time', db_index=True)),
                ('servicelevel', models.DecimalField(null=True, verbose_name='service level', max_digits=15, decimal_places=4)),
                ('localforecast', models.DecimalField(null=True, verbose_name='local forecast', max_digits=15, decimal_places=4)),
                ('localorders', models.DecimalField(null=True, verbose_name='local orders', max_digits=15, decimal_places=4)),
                ('localbackorders', models.DecimalField(null=True, verbose_name='local backorders', max_digits=15, decimal_places=4)),
                ('dependentforecast', models.DecimalField(null=True, verbose_name='dependent forecast', max_digits=15, decimal_places=4)),
                ('totaldemand', models.DecimalField(null=True, verbose_name='total demand', max_digits=15, decimal_places=4)),
                ('safetystock', models.DecimalField(null=True, verbose_name='safety stock', max_digits=15, decimal_places=4)),
                ('reorderquantity', models.DecimalField(null=True, verbose_name='reorder quantity', max_digits=15, decimal_places=4)),
                ('proposedpurchases', models.DecimalField(null=True, verbose_name='proposed purchases', max_digits=15, decimal_places=4)),
                ('proposedtransfers', models.DecimalField(null=True, verbose_name='proposed transfers', max_digits=15, decimal_places=4)),
                ('localforecastvalue', models.DecimalField(null=True, verbose_name='local forecast value', max_digits=15, decimal_places=4)),
                ('localordersvalue', models.DecimalField(null=True, verbose_name='local orders value', max_digits=15, decimal_places=4)),
                ('localbackordersvalue', models.DecimalField(null=True, verbose_name='local backorders value', max_digits=15, decimal_places=4)),
                ('dependentforecastvalue', models.DecimalField(null=True, verbose_name='dependent forecast value', max_digits=15, decimal_places=4)),
                ('totaldemandvalue', models.DecimalField(null=True, verbose_name='total demand value', max_digits=15, decimal_places=4)),
                ('safetystockvalue', models.DecimalField(null=True, verbose_name='safety stock value', max_digits=15, decimal_places=4)),
                ('reorderquantityvalue', models.DecimalField(null=True, verbose_name='reorder quantity value', max_digits=15, decimal_places=4)),
                ('proposedpurchasesvalue', models.DecimalField(null=True, verbose_name='proposed purchases value', max_digits=15, decimal_places=4)),
                ('proposedtransfersvalue', models.DecimalField(null=True, verbose_name='proposed transfers value', max_digits=15, decimal_places=4)),
            ],
            options={
                'db_table': 'out_inventoryplanning',
                'ordering': ['buffer'],
            },
        ),
      migrations.RunPython(loadParameters),
    ]
