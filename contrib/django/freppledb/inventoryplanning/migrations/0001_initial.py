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
                ('source', models.CharField(max_length=300, blank=True, null=True, verbose_name='source', db_index=True)),
                ('lastmodified', models.DateTimeField(default=django.utils.timezone.now, editable=False, db_index=True, verbose_name='last modified')),
                ('buffer', models.OneToOneField(primary_key=True, to='input.Buffer', serialize=False)),
                ('roq_type', models.CharField(choices=[('calculated', 'calculated'), ('quantity', 'quantity'), ('periodofcover', 'period of cover')], max_length=20, blank=True, null=True, verbose_name='ROQ type')),
                ('roq_min_qty', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='ROQ minimum quantity', max_digits=15)),
                ('roq_max_qty', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='ROQ maximum quantity', max_digits=15)),
                ('roq_multiple_qty', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='ROQ multiple quantity', max_digits=15)),
                ('roq_min_poc', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='ROQ minimum period of cover', max_digits=15)),
                ('roq_max_poc', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='ROQ maximum period of cover', max_digits=15)),
                ('leadtime_deviation', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='lead time deviation', max_digits=15)),
                ('demand_deviation', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='demand deviation', max_digits=15)),
                ('demand_distribution', models.CharField(choices=[('Automatic', 'Automatic'), ('Normal', 'Normal'), ('Poisson', 'Poisson'), ('Negative Binomial', 'Negative Binomial')], max_length=20, blank=True, null=True, verbose_name='demand distribution')),
                ('ss_type', models.CharField(choices=[('calculated', 'calculated'), ('quantity', 'quantity'), ('periodofcover', 'period of cover')], max_length=20, blank=True, null=True, verbose_name='Safety stock type')),
                ('service_level', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='service level', max_digits=15)),
                ('ss_min_qty', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='safety stock minimum quantity', max_digits=15)),
                ('ss_max_qty', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='safety stock maximum quantity', max_digits=15)),
                ('ss_multiple_qty', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='safety stock multiple quantity', max_digits=15)),
                ('ss_min_poc', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='safety stock minimum period of cover', max_digits=15)),
                ('ss_max_poc', models.DecimalField(decimal_places=4, blank=True, null=True, verbose_name='safety stock maximum period of cover', max_digits=15)),
                ('nostock', models.BooleanField(default=False, verbose_name='Do not stock')),
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
                ('buffer', models.OneToOneField(primary_key=True, to='input.Buffer', serialize=False)),
                ('leadtime', models.DurationField(null=True, verbose_name='lead time', db_index=True)),
                ('calculatedreorderquantity', models.DecimalField(decimal_places=4, null=True, verbose_name='calculated reorder quantity', max_digits=15)),
                ('calculatedsafetystock', models.DecimalField(decimal_places=4, null=True, verbose_name='calculated safety stock', max_digits=15)),
                ('safetystock', models.DecimalField(decimal_places=4, null=True, verbose_name='safety stock', max_digits=15)),
                ('reorderquantity', models.DecimalField(decimal_places=4, null=True, verbose_name='reorder quantity', max_digits=15)),
                ('safetystockvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='safety stock value', max_digits=15)),
                ('reorderquantityvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='reorder quantity value', max_digits=15)),
                ('onhand', models.DecimalField(decimal_places=4, null=True, verbose_name='onhand', max_digits=15)),
                ('overduesalesorders', models.DecimalField(decimal_places=4, null=True, verbose_name='overdue sales orders', max_digits=15)),
                ('opensalesorders', models.DecimalField(decimal_places=4, null=True, verbose_name='open sales orders', max_digits=15)),
                ('proposedpurchases', models.DecimalField(decimal_places=4, null=True, verbose_name='proposed purchases', max_digits=15)),
                ('proposedtransfers', models.DecimalField(decimal_places=4, null=True, verbose_name='proposed transfers', max_digits=15)),
                ('openpurchases', models.DecimalField(decimal_places=4, null=True, verbose_name='open purchases', max_digits=15)),
                ('opentransfers', models.DecimalField(decimal_places=4, null=True, verbose_name='open transfers', max_digits=15)),
                ('onhandvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='onhand value', max_digits=15)),
                ('overduesalesordersvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='overdue sales orders value', max_digits=15)),
                ('opensalesordersvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='open sales orders value', max_digits=15)),
                ('proposedpurchasesvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='proposed purchases value', max_digits=15)),
                ('proposedtransfersvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='proposed transfers value', max_digits=15)),
                ('openpurchasesvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='open purchases value', max_digits=15)),
                ('opentransfersvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='open transfers value', max_digits=15)),
                ('localforecast', models.DecimalField(decimal_places=4, null=True, verbose_name='local forecast', max_digits=15)),
                ('dependentforecast', models.DecimalField(decimal_places=4, null=True, verbose_name='dependent forecast', max_digits=15)),
                ('totaldemand', models.DecimalField(decimal_places=4, null=True, verbose_name='total demand', max_digits=15)),
                ('localforecastvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='local forecast value', max_digits=15)),
                ('dependentforecastvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='dependent forecast value', max_digits=15)),
                ('totaldemandvalue', models.DecimalField(decimal_places=4, null=True, verbose_name='total demand value', max_digits=15)),
            ],
            options={
                'db_table': 'out_inventoryplanning',
                'ordering': ['buffer'],
            },
        ),
      migrations.RunPython(loadParameters),
    ]
