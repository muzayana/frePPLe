#
# Copyright (C) 2014 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os.path

from django.conf import settings
from django.core import management
from django.http.response import StreamingHttpResponse
from django.test import TransactionTestCase
from django.test.utils import override_settings

from freppledb.common.models import User
import freppledb.output as output


@override_settings(INSTALLED_APPS=settings.INSTALLED_APPS + ('django.contrib.sessions',))
class cookbooktest(TransactionTestCase):

  def setUp(self):
    # Make sure the test database is used
    os.environ['FREPPLE_TEST'] = "YES"

  def tearDown(self):
    del os.environ['FREPPLE_TEST']

  def loadExcel(self, *filepath):
    # Login
    User.objects.create_superuser('admin', 'your@company.com', 'admin')
    self.client.login(username='admin', password='admin')
    try:
      with open(os.path.join(*filepath), "rb") as myfile:
        response = self.client.post('/execute/launch/importworkbook/', {'spreadsheet': myfile})
        if not isinstance(response, StreamingHttpResponse):
          raise Exception("expected a streaming response")
        for rec in response.streaming_content:
          rec
    except Exception as e:
      self.fail("Can't load excel file: %s" % e)
    self.assertEqual(response.status_code, 200)
    self.client.logout()

  def assertOperationplans(self, *resultpath):
    opplans = [
      "%s,%s,%s,%s" % (i.operation, i.startdate, i.enddate, round(i.quantity, 1))
      for i in output.models.OperationPlan.objects \
        .extra(select={'lower_operation':'lower(operation)'}) \
        .order_by('lower_operation', 'startdate', 'quantity') \
        .only('operation', 'startdate', 'enddate', 'quantity')
      ]
    row = 0
    maxrow = len(opplans)
    with open(os.path.join(*resultpath), 'r') as f:
      for line in f:
        if row >= maxrow or opplans[row].strip() != line.strip():
          print("Got:")
          for i in opplans:
            print("  ", i.strip())
          if row > len(opplans):
            self.fail("Difference in expected results on line %s" % (row + 1))
          else:
            self.fail("Less output rows than expected")
        row += 1
    if row != maxrow:
      self.fail("More output rows than expected")

  def test_calendar_working_hours(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "calendar", "calendar-working-hours.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "calendar", "calendar-working-hours.expect")

  def test_resource_type(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-type.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-type.expect")

  def test_resource_skills(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-skills.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-skills.expect")

  def test_resource_tool(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-tool.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-tool.expect")

  def test_demand_priorities(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "demand", "demand-priorities.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "demand", "demand-priorities.expect")

  def test_demand_policies(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "demand", "demand-policies.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "demand", "demand-policies.expect")

  def test_operation_type(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "operation", "operation-type.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "operation", "operation-type.expect")

  def test_operation_posttime(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "operation", "operation-posttime.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "operation", "operation-posttime.expect")
