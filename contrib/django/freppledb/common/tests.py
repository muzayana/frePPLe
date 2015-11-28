#
# Copyright (C) 2009-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os
import os.path

from django.conf import settings
from django.core import management
from django.http.response import StreamingHttpResponse
from django.test import TestCase, TransactionTestCase
from django.test.utils import override_settings

from freppledb.common.models import User
import freppledb.common as common
import freppledb.input as input

from rest_framework.test import APIClient, APITestCase, APIRequestFactory
from rest_framework import status
from django.core.urlresolvers import reverse

@override_settings(INSTALLED_APPS=settings.INSTALLED_APPS + ('django.contrib.sessions',))
class DataLoadTest(TestCase):

  def setUp(self):
    # Login
    self.client.login(username='admin', password='admin')

  def test_common_parameter(self):
    response = self.client.get('/data/common/parameter/?format=json')
    if not isinstance(response, StreamingHttpResponse):
      raise Exception("expected a streaming response")
    for i in response.streaming_content:
      if b'"records":60,' in i:   # Different between Enterprise Edition and Community Edition
        return
    self.fail("Didn't find expected number of parameters")


@override_settings(INSTALLED_APPS=settings.INSTALLED_APPS + ('django.contrib.sessions',))
class UserPreferenceTest(TestCase):

  def test_get_set_preferences(self):
    user = User.objects.all().get(username='admin')
    before = user.getPreference('test')
    self.assertIsNone(before)
    user.setPreference('test', {'a': 1, 'b': 'c'})
    after = user.getPreference('test')
    self.assertEqual(after, {'a': 1, 'b': 'c'})


@override_settings(INSTALLED_APPS=settings.INSTALLED_APPS + ('django.contrib.sessions',))
class ExcelTest(TransactionTestCase):

  fixtures = ['demo']

  def setUp(self):
    # Login
    User.objects.create_superuser('admin', 'your@company.com', 'admin')
    self.client.login(username='admin', password='admin')

  def tearDown(self):
    if os.path.exists("workbook.xlsx"):
      os.remove("workbook.xlsx")

  def run_workbook(self, language):
    # Change the language preference
    self.client.post('/preferences/', {'pagesize': 100, 'language': language, 'theme': 'sunny'})

    # Initial size
    countBuffer = input.models.Buffer.objects.count()
    countCalendarBucket = input.models.CalendarBucket.objects.count()
    countCalendar = input.models.Calendar.objects.count()
    countCustomer = input.models.Customer.objects.count()
    countDemand = input.models.Demand.objects.count()
    countFlow = input.models.Flow.objects.count()
    countItem = input.models.Item.objects.count()
    countItemSupplier = input.models.ItemSupplier.objects.count()
    countItemDistribution = input.models.ItemDistribution.objects.count()
    countLoad = input.models.Load.objects.count()
    countLocation = input.models.Location.objects.count()
    countOperationPlan = input.models.OperationPlan.objects.count()
    countOperation = input.models.Operation.objects.count()
    countResourceSkill = input.models.ResourceSkill.objects.count()
    countResource = input.models.Resource.objects.count()
    countSetupMatrix = input.models.SetupMatrix.objects.count()
    countSkill = input.models.Skill.objects.count()
    countSubOperation = input.models.SubOperation.objects.count()
    countSupplier = input.models.Supplier.objects.count()
    countBucket = common.models.Bucket.objects.count()
    countBucketDetail = common.models.BucketDetail.objects.count()
    countParameter = common.models.Parameter.objects.count()
    self.assertTrue(countDemand > 0)

    # Export workbook
    response = self.client.post('/execute/launch/exportworkbook/', {
       'entities': [
          'input.buffer', 'input.calendarbucket', 'input.calendar',
          'input.customer', 'input.demand', 'input.flow', 'input.item',
          'input.itemsupplier', 'input.itemdistribution',
          'input.load', 'input.location', 'input.operationplan',
          'input.operation', 'input.resourceskill', 'input.resource',
          'input.skill', 'input.supplier', 'input.suboperation',
          'common.bucket', 'common.bucketdetail', 'common.parameter',
          ]
       })
    with open("workbook.xlsx", 'wb') as f:
      f.write(response.content)

    # Erase the database
    management.call_command('frepple_flush')
    self.assertEqual(input.models.Buffer.objects.count(), 0)
    self.assertEqual(input.models.CalendarBucket.objects.count(), 0)
    self.assertEqual(input.models.Calendar.objects.count(), 0)
    self.assertEqual(input.models.Customer.objects.count(), 0)
    self.assertEqual(input.models.Demand.objects.count(), 0)
    self.assertEqual(input.models.Flow.objects.count(), 0)
    self.assertEqual(input.models.Item.objects.count(), 0)
    self.assertEqual(input.models.ItemDistribution.objects.count(), 0)
    self.assertEqual(input.models.ItemSupplier.objects.count(), 0)
    self.assertEqual(input.models.Load.objects.count(), 0)
    self.assertEqual(input.models.Location.objects.count(), 0)
    self.assertEqual(input.models.OperationPlan.objects.count(), 0)
    self.assertEqual(input.models.Operation.objects.count(), 0)
    self.assertEqual(input.models.ResourceSkill.objects.count(), 0)
    self.assertEqual(input.models.Resource.objects.count(), 0)
    self.assertEqual(input.models.SetupMatrix.objects.count(), 0)
    self.assertEqual(input.models.Skill.objects.count(), 0)
    self.assertEqual(input.models.SubOperation.objects.count(), 0)
    self.assertEqual(input.models.Supplier.objects.count(), 0)
    self.assertEqual(common.models.Bucket.objects.count(), 0)
    self.assertEqual(common.models.BucketDetail.objects.count(), 0)
    self.assertEqual(common.models.Parameter.objects.count(), 0)

    # Import the same workbook again
    with open("workbook.xlsx", 'rb') as f:
      response = self.client.post('/execute/launch/importworkbook/', {'spreadsheet': f})
      if not isinstance(response, StreamingHttpResponse):
        raise Exception("expected a streaming response")
      for rec in response.streaming_content:
        rec
      self.assertEqual(response.status_code, 200)

    # Verify the new content is identical
    self.assertEqual(input.models.Buffer.objects.count(), countBuffer)
    self.assertEqual(input.models.CalendarBucket.objects.count(), countCalendarBucket)
    self.assertEqual(input.models.Calendar.objects.count(), countCalendar)
    self.assertEqual(input.models.Customer.objects.count(), countCustomer)
    self.assertEqual(input.models.Demand.objects.count(), countDemand)
    self.assertEqual(input.models.Flow.objects.count(), countFlow)
    self.assertEqual(input.models.Item.objects.count(), countItem)
    self.assertEqual(input.models.ItemDistribution.objects.count(), countItemDistribution)
    self.assertEqual(input.models.ItemSupplier.objects.count(), countItemSupplier)
    self.assertEqual(input.models.Load.objects.count(), countLoad)
    self.assertEqual(input.models.Location.objects.count(), countLocation)
    self.assertEqual(input.models.OperationPlan.objects.count(), countOperationPlan)
    self.assertEqual(input.models.Operation.objects.count(), countOperation)
    self.assertEqual(input.models.ResourceSkill.objects.count(), countResourceSkill)
    self.assertEqual(input.models.Resource.objects.count(), countResource)
    self.assertEqual(input.models.SetupMatrix.objects.count(), countSetupMatrix)
    self.assertEqual(input.models.Skill.objects.count(), countSkill)
    self.assertEqual(input.models.SubOperation.objects.count(), countSubOperation)
    self.assertEqual(input.models.Supplier.objects.count(), countSupplier)
    self.assertEqual(common.models.Bucket.objects.count(), countBucket)
    self.assertEqual(common.models.BucketDetail.objects.count(), countBucketDetail)
    self.assertEqual(common.models.Parameter.objects.count(), countParameter)

  def test_workbook_english(self):
    self.run_workbook("en")

  def test_workbook_chinese(self):
    self.run_workbook("zh-cn")

  def test_workbook_dutch(self):
    self.run_workbook("nl")

  def test_workbook_french(self):
    self.run_workbook("fr")

  def test_workbook_japanese(self):
    self.run_workbook("ja")

  def test_workbook_portuguese(self):
    self.run_workbook("pt")

  def test_workbook_brazilian_portuguese(self):
    self.run_workbook("pt-br")


@override_settings(INSTALLED_APPS=settings.INSTALLED_APPS + ('django.contrib.sessions',))
class freppleREST(APITestCase):


  fixtures = ["demo"]

  #Default request format is multipart
  self.factory = APIRequestFactory(enforce_csrf_checks=True) 

  def setUp(self):
    # Login
    self.client = APIClient()
    self.client.login(username='admin', password='admin')

  # REST API framework
  def test_api_listpages_getapi(self):
    response = self.client.get('/api/')
    self.assertEqual(response.status_code, 200)

    response = self.client.get('/api/input/demand/')
    self.assertEqual(response.status_code, 200)
    self.assertEqual(demand.objects.count(), 24)
    data = { 
	  "name": "Order UFO 25",
          "description": null,
          "category": null,
          "subcategory": null,
          "item": "product",
          "location": null,
          "due": "2013-12-01T00:00:00",
          "status": "closed",
          "operation": null,
          "quantity": "110.0000",
          "priority": 1,
          "minshipment": null,
          "maxlateness": null
    }
    response = self.client.post(url, data)
    self.assertEqual(customer.objects.count(), 1)
    self.assertEqual(demand.objects.get().name, 'Order UFO 25')

    response = self.client.get('/api/input/customer/')
    self.assertEqual(response.status_code, 200)
    self.assertEqual(customer.objects.count(), 2)
    data = {
	  "name": "Customer near Area 51"
    }
    self.assertEqual(customer.objects.count(), 1)    
    self.assertEqual(customer.objects.get().name, 'Customer near Area 51')

  def test_api_detailpages_getapi(self):
    response = self.client.get('/api/input/demand/Demand 1/')
    self.assertEqual(response.status_code, 200)
    self.assertEqual(demand.objects.get().name, 'Demand 1')

    response = self.client.get('/api/input/customer/Customer near factory 1/')
    self.assertEqual(response.status_code, 200)
    self.assertEqual(customer.objects.get().name, 'Customer near factory 1')

  def test_api_listpages_getjson(self):
    response = self.client.get('/api/input/demand/',format='json')
    self.assertEqual(response.status_code, 200)
    self.assertEqual(demand.objects.get().pk, 'Demand 1')

    response = self.client.get('/api/input/customer/', format='json')
    self.assertEqual(response.status_code, 200)
    self.assertEqual(customer.objects.get().pk, 'Customer near factory 1')

  def test_api_detailpages_getjson(self):
    response = self.client.get('/api/input/demand/Demand 1/')
    self.assertEqual(response.status_code, 200)
    response = self.client.get('/api/input/customer/Customer near factory 1/')
    self.assertEqual(response.status_code, 200)
