#
# Copyright (C) 2012-2014 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf import settings
from django.test import TestCase
from django.test.utils import override_settings


@override_settings(INSTALLED_APPS=settings.INSTALLED_APPS + ('django.contrib.sessions',))
class ForecastTest(TestCase):

  fixtures = ["demo"]

  def setUp(self):
    # Login
    self.client.login(username='admin', password='admin')

  def test_input_forecast(self):
    response = self.client.get('/forecast/')
    self.assertEqual(response.status_code, 200)
    response = self.client.get('/forecast/?format=json')
    self.assertContains(response, '"records":2,')
    self.assertEqual(response.status_code, 200)
    response = self.client.get('/forecast/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
