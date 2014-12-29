#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public
# License along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import os.path
from io import StringIO

from django.conf import settings
from django.core import management
from django.test import TransactionTestCase

import freppledb.output as output


class cookbooktest(TransactionTestCase):

  def setUp(self):
    # Make sure the test database is used
    if not 'django.contrib.sessions' in settings.INSTALLED_APPS:
      settings.INSTALLED_APPS += ('django.contrib.sessions',)
    os.environ['FREPPLE_TEST'] = "YES"

  def tearDown(self):
    del os.environ['FREPPLE_TEST']

  def loadExcel(self, *filepath):
    # Login
    self.client.login(username='admin', password='admin')
    try:
      with open(os.path.join(*filepath), "rb") as myfile:
        response = self.client.post('/execute/launch/importworkbook/', {'spreadsheet': myfile})
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
    with open(os.path.join(*resultpath), 'r') as f:
      for line in f:
        if opplans[row].strip() != line.strip():
          print("Got:")
          for i in opplans:
            print("  ", i.strip())
          self.fail("Difference in expected results on line %s" % (row + 1))
        row += 1
    if row != len(opplans):
      self.fail("More output rows than expected")

  def test_calendar_working_hours(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "calendar", "calendar-working-hours.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "calendar", "calendar-working-hours.expect")

  def test_resource_type(self):
    self.loadExcel(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-type.xlsx")
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertOperationplans(settings.FREPPLE_HOME, "..", "doc", "cookbook", "resource", "resource-type.expect")

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
