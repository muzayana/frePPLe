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
                ('source', models.CharField(null=True, blank=True, max_length=20, db_index=True, verbose_name='source')),
                ('lastmodified', models.DateTimeField(verbose_name='last modified', editable=False, db_index=True, default=django.utils.timezone.now)),
                ('buffer', models.OneToOneField(serialize=False, primary_key=True, to='input.Buffer')),
                ('roq_min_qty', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='ROQ minimum quantity')),
                ('roq_max_qty', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='ROQ maximum quantity')),
                ('roq_multiple_qty', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='ROQ multiple quantity')),
                ('roq_min_poc', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='ROQ minimum period of cover')),
                ('roq_max_poc', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='ROQ maximum period of cover')),
                ('leadtime_deviation', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='lead time deviation')),
                ('demand_deviation', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='demand deviation')),
                ('demand_distribution', models.CharField(null=True, choices=[('automatic', 'Automatic'), ('normal', 'Normal'), ('poisson', 'Poisson'), ('negative binomial', 'Negative binomial')], verbose_name='demand distribution', max_length=20, blank=True)),
                ('service_level', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='service level')),
                ('ss_min_qty', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='safety stock minimum quantity')),
                ('ss_max_qty', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='safety stock maximum quantity')),
                ('ss_multiple_qty', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='safety stock multiple quantity')),
                ('ss_min_poc', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='safety stock minimum period of cover')),
                ('ss_max_poc', models.DecimalField(null=True, blank=True, decimal_places=4, max_digits=15, verbose_name='safety stock maximum period of cover')),
                ('nostock', models.BooleanField(default=False, verbose_name='Do not stock')),
            ],
            options={
                'verbose_name': 'inventory planning parameter',
                'verbose_name_plural': 'inventory planning parameters',
                'ordering': ['buffer'],
                'abstract': False,
                'db_table': 'inventoryplanning',
            },
        ),
        migrations.CreateModel(
            name='InventoryPlanningOutput',
            fields=[
                ('buffer', models.OneToOneField(serialize=False, primary_key=True, to='input.Buffer')),
                ('leadtime', models.DurationField(null=True, verbose_name='lead time', db_index=True)),
                ('servicelevel', models.DecimalField(null=True, verbose_name='service level', decimal_places=4, max_digits=15)),
                ('localforecast', models.DecimalField(null=True, verbose_name='local forecast', decimal_places=4, max_digits=15)),
                ('localorders', models.DecimalField(null=True, verbose_name='local orders', decimal_places=4, max_digits=15)),
                ('localbackorders', models.DecimalField(null=True, verbose_name='local backorders', decimal_places=4, max_digits=15)),
                ('dependentforecast', models.DecimalField(null=True, verbose_name='dependent forecast', decimal_places=4, max_digits=15)),
                ('totaldemand', models.DecimalField(null=True, verbose_name='total demand', decimal_places=4, max_digits=15)),
                ('safetystock', models.DecimalField(null=True, verbose_name='safety stock', decimal_places=4, max_digits=15)),
                ('reorderquantity', models.DecimalField(null=True, verbose_name='reorder quantity', decimal_places=4, max_digits=15)),
                ('proposedpurchases', models.DecimalField(null=True, verbose_name='proposed purchases', decimal_places=4, max_digits=15)),
                ('proposedtransfers', models.DecimalField(null=True, verbose_name='proposed transfers', decimal_places=4, max_digits=15)),
                ('localforecastvalue', models.DecimalField(null=True, verbose_name='local forecast value', decimal_places=4, max_digits=15)),
                ('localordersvalue', models.DecimalField(null=True, verbose_name='local orders value', decimal_places=4, max_digits=15)),
                ('localbackordersvalue', models.DecimalField(null=True, verbose_name='local backorders value', decimal_places=4, max_digits=15)),
                ('dependentforecastvalue', models.DecimalField(null=True, verbose_name='dependent forecast value', decimal_places=4, max_digits=15)),
                ('totaldemandvalue', models.DecimalField(null=True, verbose_name='total demand value', decimal_places=4, max_digits=15)),
                ('safetystockvalue', models.DecimalField(null=True, verbose_name='safety stock value', decimal_places=4, max_digits=15)),
                ('reorderquantityvalue', models.DecimalField(null=True, verbose_name='reorder quantity value', decimal_places=4, max_digits=15)),
                ('proposedpurchasesvalue', models.DecimalField(null=True, verbose_name='proposed purchases value', decimal_places=4, max_digits=15)),
                ('proposedtransfersvalue', models.DecimalField(null=True, verbose_name='proposed transfers value', decimal_places=4, max_digits=15)),
            ],
            options={
                'ordering': ['buffer'],
                'db_table': 'out_inventoryplanning',
            },
        ),
      migrations.RunPython(loadParameters),
    ]
