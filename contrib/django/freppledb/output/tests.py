#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.test import TestCase
from django.conf import settings


class OutputTest(TestCase):

  def setUp(self):
    # Login
    if not 'django.contrib.sessions' in settings.INSTALLED_APPS:
      settings.INSTALLED_APPS += ('django.contrib.sessions',)
    self.client.login(username='frepple', password='frepple')

  # Buffer
  def test_output_buffer(self):
    response = self.client.get('/buffer/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":8,')

  def test_output_buffer_csvtable(self):
    response = self.client.get('/buffer/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  def test_output_buffer_csvlist(self):
    response = self.client.get('/buffer/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  # Resource
  def test_output_resource(self):
    response = self.client.get('/resource/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":3,')

  def test_output_resource_csvtable(self):
    response = self.client.get('/resource/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  # Demand
  def test_output_demand(self):
    response = self.client.get('/demand/')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, 'Demand report')

  def test_output_demand_csvlist(self):
    response = self.client.get('/demand/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  # Operation
  def test_output_operation(self):
    response = self.client.get('/operation/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":14,')

  def test_output_operation_csvtable(self):
    response = self.client.get('/operation/?format=csvtable')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  # Problem
  def test_output_problem(self):
    response = self.client.get('/problem/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":0,')

  def test_output_problem_csvlist(self):
    response = self.client.get('/problem/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  # Constraint
  def test_output_constraint(self):
    response = self.client.get('/constraint/?format=json')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, '"records":0,')

  def test_output_constraint_csvlist(self):
    response = self.client.get('/constraint/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))

  # KPI
  def test_output_kpi(self):
    response = self.client.get('/kpi/')
    self.assertEqual(response.status_code, 200)
    self.assertContains(response, 'Performance Indicators')

  def test_output_kpi_csvlist(self):
    response = self.client.get('/kpi/?format=csvlist')
    self.assertEqual(response.status_code, 200)
    self.assertTrue(response.__getitem__('Content-Type').startswith('text/csv; charset='))
