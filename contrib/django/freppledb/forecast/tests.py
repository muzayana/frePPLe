#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.  
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code 
# or in the form of compiled binaries.
#

from django.test import TestCase

class OutputTest(TestCase):

  def setUp(self):
    # Login
    self.client.login(username='frepple', password='frepple')
    
  # Forecast
  def test_output_forecast(self):
    response = self.client.get('/forecast/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":2,')

  def test_output_forecast_csvlist(self):
    response = self.client.get('/forecast/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
