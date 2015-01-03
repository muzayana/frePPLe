#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
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
class OutputTest(TestCase):

  fixtures = ["demo"]

  def setUp(self):
    # Login
    login = self.client.login(username='admin', password='admin')

  # Buffer
  def test_output_buffer(self):
    response = self.client.get('/buffer/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":8,')
    response = self.client.get('/buffer/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/buffer/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/buffer/?format=spreadsheettable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))


  # Resource
  def test_output_resource(self):
    response = self.client.get('/resource/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":3,')
    response = self.client.get('/resource/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/resource/?format=spreadsheetlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))

  # Demand
  def test_output_demand(self):
    response = self.client.get('/demand/')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, 'Demand report')
    response = self.client.get('/demand/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/demand/?format=spreadsheettable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))

  # Operation
  def test_output_operation(self):
    response = self.client.get('/operation/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":14,')
    response = self.client.get('/operation/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/operation/?format=spreadsheetlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))

  # Problem
  def test_output_problem(self):
    response = self.client.get('/problem/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":0,')
    response = self.client.get('/problem/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/problem/?format=spreadsheetlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))

  # Constraint
  def test_output_constraint(self):
    response = self.client.get('/constraint/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":0,')
    response = self.client.get('/constraint/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/constraint/?format=spreadsheetlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))

  # KPI
  def test_output_kpi(self):
    response = self.client.get('/kpi/')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, 'Performance Indicators')
    response = self.client.get('/kpi/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
    response = self.client.get('/kpi/?format=spreadsheetlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'))
