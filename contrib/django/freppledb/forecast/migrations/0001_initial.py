#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from django.conf import settings
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
                ('source', models.CharField(max_length=settings.CATEGORYSIZE, verbose_name='source', null=True, db_index=True, blank=True)),
                ('lastmodified', models.DateTimeField(verbose_name='last modified', editable=False, db_index=True, default=django.utils.timezone.now)),
                ('name', models.CharField(max_length=settings.NAMESIZE, primary_key=True, verbose_name='name', serialize=False)),
                ('description', models.CharField(max_length=settings.DESCRIPTIONSIZE, null=True, verbose_name='description', blank=True)),
                ('category', models.CharField(max_length=settings.CATEGORYSIZE, verbose_name='category', null=True, db_index=True, blank=True)),
                ('subcategory', models.CharField(max_length=settings.CATEGORYSIZE, verbose_name='subcategory', null=True, db_index=True, blank=True)),
                ('method', models.CharField(choices=[('automatic', 'Automatic'), ('constant', 'Constant'), ('trend', 'Trend'), ('seasonal', 'Seasonal'), ('intermittent', 'Intermittent'), ('moving average', 'Moving average'), ('manual', 'Manual')], help_text='Method used to generate a base forecast', verbose_name='Forecast method', max_length=20, null=True, default='automatic', blank=True)),
                ('priority', models.PositiveIntegerField(choices=[(1, '1'), (2, '2'), (3, '3'), (4, '4'), (5, '5'), (6, '6'), (7, '7'), (8, '8'), (9, '9'), (10, '10'), (11, '11'), (12, '12'), (13, '13'), (14, '14'), (15, '15'), (16, '16'), (17, '17'), (18, '18'), (19, '19'), (20, '20')], help_text='Priority of the demand (lower numbers indicate more important demands)', default=10, verbose_name='priority')),
                ('minshipment', models.DecimalField(decimal_places=settings.DECIMAL_PLACES, max_digits=settings.MAX_DIGITS, verbose_name='minimum shipment', blank=True, help_text='Minimum shipment quantity when planning this demand', null=True)),
                ('maxlateness', models.DecimalField(decimal_places=settings.DECIMAL_PLACES, max_digits=settings.MAX_DIGITS, verbose_name='maximum lateness', blank=True, help_text='Maximum lateness allowed when planning this demand', null=True)),
                ('discrete', models.BooleanField(help_text='Round forecast numbers to integers', verbose_name='discrete', default=True)),
                ('planned', models.BooleanField(help_text='Use this forecast for planning', verbose_name='planned', default=True)),
                ('calendar', models.ForeignKey(to='input.Calendar', verbose_name='calendar')),
                ('customer', models.ForeignKey(to='input.Customer', verbose_name='customer')),
                ('item', models.ForeignKey(to='input.Item', verbose_name='item')),
                ('operation', models.ForeignKey(related_name='used_forecast', verbose_name='delivery operation', to='input.Operation', blank=True, help_text='Operation used to satisfy this demand', null=True)),
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
                ('source', models.CharField(max_length=settings.CATEGORYSIZE, verbose_name='source', null=True, db_index=True, blank=True)),
                ('lastmodified', models.DateTimeField(verbose_name='last modified', editable=False, db_index=True, default=django.utils.timezone.now)),
                ('id', models.AutoField(primary_key=True, serialize=False, verbose_name='identifier')),
                ('startdate', models.DateField(verbose_name='start date')),
                ('enddate', models.DateField(verbose_name='end date')),
                ('quantity', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default=0, verbose_name='quantity')),
                ('forecast', models.ForeignKey(related_name='entries', to='forecast.Forecast', verbose_name='forecast')),
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
                ('id', models.AutoField(primary_key=True, serialize=False, verbose_name='identifier')),
                ('customerlvl', models.PositiveIntegerField(null=True, editable=False, blank=True)),
                ('itemlvl', models.PositiveIntegerField(null=True, editable=False, blank=True)),
                ('startdate', models.DateTimeField(db_index=True, verbose_name='start date')),
                ('enddate', models.DateTimeField(db_index=True, verbose_name='end date')),
                ('orderstotal', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='total orders')),
                ('ordersadjustment', models.DecimalField(max_digits=settings.MAX_DIGITS, null=True, decimal_places=settings.DECIMAL_PLACES, verbose_name='orders adjustment', blank=True)),
                ('ordersopen', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='open orders')),
                ('ordersplanned', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='planned orders')),
                ('forecastbaseline', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast baseline')),
                ('forecastadjustment', models.DecimalField(max_digits=settings.MAX_DIGITS, null=True, decimal_places=settings.DECIMAL_PLACES, verbose_name='forecast adjustment', blank=True)),
                ('forecasttotal', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast total')),
                ('forecastnet', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast net')),
                ('forecastconsumed', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast consumed')),
                ('forecastplanned', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='planned forecast')),
                ('orderstotalvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='total orders')),
                ('ordersadjustmentvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, null=True, decimal_places=settings.DECIMAL_PLACES, verbose_name='orders adjustment', blank=True)),
                ('ordersopenvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='open orders')),
                ('ordersplannedvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='planned orders')),
                ('forecastbaselinevalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast baseline')),
                ('forecastadjustmentvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, null=True, decimal_places=settings.DECIMAL_PLACES, verbose_name='forecast adjustment', blank=True)),
                ('forecasttotalvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast total')),
                ('forecastnetvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast net')),
                ('forecastconsumedvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='forecast consumed')),
                ('forecastplannedvalue', models.DecimalField(max_digits=settings.MAX_DIGITS, decimal_places=settings.DECIMAL_PLACES, default='0.00', verbose_name='planned forecast')),
                ('method', models.CharField(max_length=settings.NAMESIZE, null=True, editable=False, db_index=True, verbose_name='method')),
                ('forecast', models.ForeignKey(related_name='plans', to='forecast.Forecast', verbose_name='forecast')),
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
