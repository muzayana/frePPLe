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
  call_command('loaddata', "parameters.json", app_label="forecast", verbosity=0)


class Migration(migrations.Migration):

    dependencies = [
        ('input', '0001_initial'),
    ]

    operations = [
        migrations.CreateModel(
            name='Forecast',
            fields=[
                ('source', models.CharField(db_index=True, blank=True, max_length=300, verbose_name='source', null=True)),
                ('lastmodified', models.DateTimeField(db_index=True, default=django.utils.timezone.now, editable=False, verbose_name='last modified')),
                ('name', models.CharField(primary_key=True, max_length=300, verbose_name='name', serialize=False)),
                ('description', models.CharField(blank=True, max_length=500, verbose_name='description', null=True)),
                ('category', models.CharField(db_index=True, blank=True, max_length=300, verbose_name='category', null=True)),
                ('subcategory', models.CharField(db_index=True, blank=True, max_length=300, verbose_name='subcategory', null=True)),
                ('method', models.CharField(blank=True, max_length=20, null=True, default='automatic', choices=[('automatic', 'Automatic'), ('constant', 'Constant'), ('trend', 'Trend'), ('seasonal', 'Seasonal'), ('intermittent', 'Intermittent'), ('moving average', 'Moving average'), ('manual', 'Manual')], help_text='Method used to generate a base forecast', verbose_name='Forecast method')),
                ('priority', models.PositiveIntegerField(default=10, choices=[(1, '1'), (2, '2'), (3, '3'), (4, '4'), (5, '5'), (6, '6'), (7, '7'), (8, '8'), (9, '9'), (10, '10'), (11, '11'), (12, '12'), (13, '13'), (14, '14'), (15, '15'), (16, '16'), (17, '17'), (18, '18'), (19, '19'), (20, '20')], help_text='Priority of the demand (lower numbers indicate more important demands)', verbose_name='priority')),
                ('minshipment', models.DecimalField(blank=True, decimal_places=4, null=True, help_text='Minimum shipment quantity when planning this demand', verbose_name='minimum shipment', max_digits=15)),
                ('maxlateness', models.DecimalField(blank=True, decimal_places=4, null=True, help_text='Maximum lateness allowed when planning this demand', verbose_name='maximum lateness', max_digits=15)),
                ('discrete', models.BooleanField(help_text='Round forecast numbers to integers', verbose_name='discrete', default=True)),
                ('planned', models.BooleanField(help_text='Use this forecast for planning', verbose_name='planned', default=True)),
                ('out_smape', models.DecimalField(blank=True, max_digits=15, decimal_places=4, verbose_name='calculated SMAPE forecast error', null=True)),
                ('out_method', models.CharField(blank=True, max_length=20, verbose_name='calculated forecast method', null=True)),
                ('out_deviation', models.DecimalField(blank=True, max_digits=15, decimal_places=4, verbose_name='calculated standard deviation', null=True)),
                ('calendar', models.ForeignKey(to='input.Calendar', verbose_name='calendar')),
                ('customer', models.ForeignKey(blank=True, to='input.Customer', null=True, verbose_name='customer')),
                ('item', models.ForeignKey(to='input.Item', verbose_name='item')),
                ('location', models.ForeignKey(blank=True, to='input.Location', null=True, verbose_name='location')),
                ('operation', models.ForeignKey(blank=True, related_name='used_forecast', to='input.Operation', null=True, help_text='Operation used to satisfy this demand', verbose_name='delivery operation')),
            ],
            options={
                'ordering': ['name'],
                'verbose_name_plural': 'forecasts',
                'abstract': False,
                'verbose_name': 'forecast',
                'db_table': 'forecast',
            },
        ),
        migrations.CreateModel(
            name='ForecastDemand',
            fields=[
                ('source', models.CharField(null=True, blank=True, verbose_name='source', db_index=True, max_length=300)),
                ('lastmodified', models.DateTimeField(editable=False, db_index=True, verbose_name='last modified', default=django.utils.timezone.now)),
                ('id', models.AutoField(primary_key=True, verbose_name='identifier', serialize=False)),
                ('startdate', models.DateField(verbose_name='start date')),
                ('enddate', models.DateField(verbose_name='end date')),
                ('quantity', models.DecimalField(max_digits=15, verbose_name='quantity', default=0, decimal_places=4)),
                ('forecast', models.ForeignKey(related_name='entries', verbose_name='forecast', to='forecast.Forecast')),
            ],
            options={
                'verbose_name_plural': 'forecast demands',
                'abstract': False,
                'verbose_name': 'forecast demand',
                'db_table': 'forecastdemand',
            },
        ),
        migrations.CreateModel(
            name='ForecastPlan',
            fields=[
                ('id', models.AutoField(primary_key=True, verbose_name='identifier', serialize=False)),
                ('startdate', models.DateTimeField(verbose_name='start date', db_index=True)),
                ('enddate', models.DateTimeField(verbose_name='end date', db_index=True)),
                ('orderstotal', models.DecimalField(max_digits=15, verbose_name='total orders', default='0.00', decimal_places=4)),
                ('ordersadjustment', models.DecimalField(null=True, max_digits=15, verbose_name='orders adjustment', blank=True, decimal_places=4)),
                ('ordersopen', models.DecimalField(max_digits=15, verbose_name='open orders', default='0.00', decimal_places=4)),
                ('ordersplanned', models.DecimalField(max_digits=15, verbose_name='planned orders', default='0.00', decimal_places=4)),
                ('forecastbaseline', models.DecimalField(max_digits=15, verbose_name='forecast baseline', default='0.00', decimal_places=4)),
                ('forecastadjustment', models.DecimalField(null=True, max_digits=15, verbose_name='forecast adjustment', blank=True, decimal_places=4)),
                ('forecasttotal', models.DecimalField(max_digits=15, verbose_name='forecast total', default='0.00', decimal_places=4)),
                ('forecastnet', models.DecimalField(max_digits=15, verbose_name='forecast net', default='0.00', decimal_places=4)),
                ('forecastconsumed', models.DecimalField(max_digits=15, verbose_name='forecast consumed', default='0.00', decimal_places=4)),
                ('forecastplanned', models.DecimalField(max_digits=15, verbose_name='planned forecast', default='0.00', decimal_places=4)),
                ('orderstotalvalue', models.DecimalField(max_digits=15, verbose_name='total orders', default='0.00', decimal_places=4)),
                ('ordersadjustmentvalue', models.DecimalField(null=True, max_digits=15, verbose_name='orders adjustment', blank=True, decimal_places=4)),
                ('ordersopenvalue', models.DecimalField(max_digits=15, verbose_name='open orders', default='0.00', decimal_places=4)),
                ('ordersplannedvalue', models.DecimalField(max_digits=15, verbose_name='planned orders', default='0.00', decimal_places=4)),
                ('forecastbaselinevalue', models.DecimalField(max_digits=15, verbose_name='forecast baseline', default='0.00', decimal_places=4)),
                ('forecastadjustmentvalue', models.DecimalField(null=True, max_digits=15, verbose_name='forecast adjustment', blank=True, decimal_places=4)),
                ('forecasttotalvalue', models.DecimalField(max_digits=15, verbose_name='forecast total', default='0.00', decimal_places=4)),
                ('forecastnetvalue', models.DecimalField(max_digits=15, verbose_name='forecast net', default='0.00', decimal_places=4)),
                ('forecastconsumedvalue', models.DecimalField(max_digits=15, verbose_name='forecast consumed', default='0.00', decimal_places=4)),
                ('forecastplannedvalue', models.DecimalField(max_digits=15, verbose_name='planned forecast', default='0.00', decimal_places=4)),
                ('forecast', models.ForeignKey(related_name='plans', verbose_name='forecast', to='forecast.Forecast')),
            ],
            options={
                'ordering': ['id'],
                'verbose_name_plural': 'forecast plans',
                'verbose_name': 'forecast plan',
                'db_table': 'forecastplan',
            },
        ),
        migrations.AlterIndexTogether(
            name='forecastplan',
            index_together=set([('forecast', 'startdate')]),
        ),
        migrations.RunPython(loadParameters),
    ]
