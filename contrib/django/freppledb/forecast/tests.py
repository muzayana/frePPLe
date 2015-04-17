#
# Copyright (C) 2012-2014 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import date
from decimal import Decimal
import os

from django.conf import settings
from django.core import management
from django.test import TestCase

from freppledb.forecast.models import Forecast, ForecastPlan


class ForecastTest(TestCase):

  fixtures = ["demo"]

  def setUp(self):
    # Login
    if not 'django.contrib.sessions' in settings.INSTALLED_APPS:
      settings.INSTALLED_APPS += ('django.contrib.sessions',)
    self.client.login(username='admin', password='admin')
    os.environ['FREPPLE_TEST'] = "YES"

  def tearDown(self):
    del os.environ['FREPPLE_TEST']

  def test_input_forecast(self):
    response = self.client.get('/forecast/')
    self.assertEqual(response.status_code, 200)
    response = self.client.get('/forecast/?format=json')
    self.assertContains(response, '"records":9,')
    self.assertEqual(response.status_code, 200)
    response = self.client.get('/forecast/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  def test_edit_forecast(self):
    # Generate the forecast
    #self.assertEqual(ForecastPlan.objects.all().count(), 0)
    #management.call_command('frepple_run', env="noproduction")
    #recs = ForecastPlan.objects.all().count()
    #self.assertGreater(recs, 0)

    # Generate the forecast and the production plan
    #management.call_command('frepple_run', plantype=1, constraint=15)
    #self.assertEqual(ForecastPlan.objects.all().count(), recs)

    # Create a quantity override at an agggregate level
    bcktst = date(2014, 1, 1)
    bcktnd = date(2014, 2, 1)
    Forecast.objects.get(name="All products - All regions").updatePlan(
      bcktst, bcktnd, Decimal(100), None, True
      )
    fcstpln1 = ForecastPlan.filter(forecast__name="product - region 1", startdate=bcktst)
    self.assertEqual(fcstpln1.forecastadjustment, 50)
    self.assertEqual(fcstpln1.forecastadjustmentvalue, 5000)
    self.assertEqual(fcstpln1.forecasttotal, 50)
    self.assertEqual(fcstpln1.forecasttotalvalue, 5000)
    fcstpln2 = ForecastPlan.filter(forecast__name="product - All regions", startdate=bcktst)
    self.assertEqual(fcstpln2.forecastadjustment, 100)
    self.assertEqual(fcstpln2.forecastadjustmentvalue, 10000)
    self.assertEqual(fcstpln2.forecasttotal, 100)
    self.assertEqual(fcstpln2.forecasttotalvalue, 10000)
    fcstpln3 = ForecastPlan.filter(forecast__name="All products - All regions", startdate=bcktst)
    self.assertEqual(fcstpln3.forecastadjustment, 100)
    self.assertEqual(fcstpln3.forecastadjustmentvalue, 10000)
    self.assertEqual(fcstpln3.forecasttotal, 100)
    self.assertEqual(fcstpln3.forecasttotalvalue, 10000)

    # Update a quantity override at the lower level
    bcktst = date(2014, 1, 1)
    bcktnd = date(2014, 2, 1)
    Forecast.objects.get(name="product - region 1").updatePlan(
      bcktst, bcktnd, Decimal(100), None, True
      )
    fcstpln1 = ForecastPlan.filter(forecast__name="product - region 1", startdate=bcktst)
    self.assertEqual(fcstpln1.forecastadjustment, 100)
    self.assertEqual(fcstpln1.forecastadjustmentvalue, 10000)
    self.assertEqual(fcstpln1.forecasttotal, 100)
    self.assertEqual(fcstpln1.forecasttotalvalue, 10000)
    fcstpln2 = ForecastPlan.filter(forecast__name="product - All regions", startdate=bcktst)
    self.assertEqual(fcstpln2.forecastadjustment, 150)
    self.assertEqual(fcstpln2.forecastadjustmentvalue, 15000)
    self.assertEqual(fcstpln2.forecasttotal, 150)
    self.assertEqual(fcstpln2.forecasttotalvalue, 15000)
    fcstpln3 = ForecastPlan.filter(forecast__name="All products - All regions", startdate=bcktst)
    self.assertEqual(fcstpln3.forecastadjustment, 150)
    self.assertEqual(fcstpln3.forecastadjustmentvalue, 15000)
    self.assertEqual(fcstpln3.forecasttotal, 150)
    self.assertEqual(fcstpln3.forecasttotalvalue, 15000)

    # Create a quantity override at the lower level
    bcktst = date(2014, 2, 1)
    bcktnd = date(2014, 3, 1)
    Forecast.objects.get(name="product2 - region 1").updatePlan(
      bcktst, bcktnd, Decimal(100), None, True
      )
    fcstpln1 = ForecastPlan.filter(forecast__name="product2 - region 1", startdate=bcktst)
    self.assertEqual(fcstpln1.forecastadjustment, 100)
    self.assertEqual(fcstpln1.forecastadjustmentvalue, 5000)
    self.assertEqual(fcstpln1.forecasttotal, 100)
    self.assertEqual(fcstpln1.forecasttotalvalue, 5000)
    fcstpln2 = ForecastPlan.filter(forecast__name="product2 - All regions", startdate=bcktst)
    self.assertEqual(fcstpln2.forecastadjustment, 100)
    self.assertEqual(fcstpln2.forecastadjustmentvalue, 5000)
    self.assertEqual(fcstpln2.forecasttotal, 100)
    self.assertEqual(fcstpln2.forecasttotalvalue, 5000)
    fcstpln3 = ForecastPlan.filter(forecast__name="All products - All regions", startdate=bcktst)
    self.assertEqual(fcstpln3.forecastadjustment, 100)
    self.assertEqual(fcstpln3.forecastadjustmentvalue, 5000)
    self.assertEqual(fcstpln3.forecasttotal, 308)
    self.assertEqual(fcstpln3.forecasttotalvalue, 25800)

    # Update a partial quantity override at aggregate level

    # Create a value override at an agggregate level

    # Update a value override at the lower level

    # Create a value override at the lower level

    # Update a partial value override at aggregate level

    # Test export in cvs and spreadsheet format
    response = self.client.get('/forecast/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/forecast/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/forecast/?format=spreadsheettable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))
