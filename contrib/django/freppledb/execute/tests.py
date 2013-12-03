#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import os

from django.core import management, serializers
from django.test import TransactionTestCase, TestCase
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS, transaction

import freppledb.output as output
import freppledb.input as input


class execute_with_commands(TransactionTestCase):

  def setUp(self):
    # Make sure the test database is used
    if not 'django.contrib.sessions' in settings.INSTALLED_APPS:
      settings.INSTALLED_APPS += ('django.contrib.sessions',)
    os.environ['FREPPLE_TEST'] = "YES"

  def tearDown(self):
    del os.environ['FREPPLE_TEST']

  def test_run_cmd(self):
    # Empty the database tables
    self.assertNotEqual(input.models.Calendar.objects.count(),0)
    management.call_command('frepple_flush')
    self.assertEqual(input.models.Calendar.objects.count(),0)
    self.assertEqual(input.models.Demand.objects.count(),0)
    self.assertEqual(output.models.Problem.objects.count(),0)
    self.assertEqual(output.models.FlowPlan.objects.count(),0)
    self.assertEqual(output.models.LoadPlan.objects.count(),0)
    self.assertEqual(output.models.OperationPlan.objects.count(),0)

    # Create a new model
    management.call_command('frepple_createmodel', cluster='1', verbosity='0')
    self.assertNotEqual(input.models.Calendar.objects.count(),0)
    self.assertNotEqual(input.models.Demand.objects.count(),0)

    # Run frePPLe on the test database
    management.call_command('frepple_run', plantype=1, constraint=15)
    self.assertEqual(output.models.Problem.objects.count(),387)
    self.assertEqual(output.models.FlowPlan.objects.count(),1847)
    self.assertEqual(output.models.LoadPlan.objects.count(),199)
    self.assertEqual(output.models.OperationPlan.objects.count(),840)


class execute_multidb(TransactionTestCase):
  multi_db = True

  def setUp(self):
    os.environ['FREPPLE_TEST'] = "YES"

  def tearDown(self):
    del os.environ['FREPPLE_TEST']

  def test_multidb_cmd(self):
    # Find out which databases to use
    db1 = DEFAULT_DB_ALIAS
    db2 = None
    for i in settings.DATABASES.keys():
      if i != DEFAULT_DB_ALIAS:
        db2 = i
        break
    if not db2:
      # Only a single database is configured and we skip this test
      return

    # Check count in both databases
    count1 = output.models.FlowPlan.objects.all().using(db1).count()
    count2 = output.models.FlowPlan.objects.all().using(db2).count()
    self.assertEqual(count1,0)
    self.assertEqual(count2,0)

    # Erase second database
    count1 = input.models.Demand.objects.all().using(db1).count()
    management.call_command('frepple_flush', database=db2)
    count1new = input.models.Demand.objects.all().using(db1).count()
    count2 = input.models.Demand.objects.all().using(db2).count()
    self.assertEqual(count1new,count1)
    self.assertEqual(count2,0)

    # Copy the db1 into db2.
    # We need to close the transactions, since they can block the copy
    transaction.commit(using=db1)
    transaction.commit(using=db2)
    management.call_command('frepple_copy', db1, db2)
    count1 = output.models.Demand.objects.all().using(db1).count()
    count2 = output.models.Demand.objects.all().using(db2).count()
    self.assertEqual(count1,count2)

    # Run the plan on db1.
    # The count changes in db1 and not in db2.
    management.call_command('frepple_run', plantype=1, constraint=15, database=db1)
    count1 = output.models.FlowPlan.objects.all().using(db1).count()
    count2 = output.models.FlowPlan.objects.all().using(db2).count()
    self.assertNotEqual(count1,0)
    self.assertEqual(count2,0)

    # Run a plan on db2.
    # The count changes in db1 and not in db2.
    # The count in both databases is expected to be different since we run a different plan
    management.call_command('frepple_run', plantype=1, constraint=0, database=db2)
    count1new = output.models.FlowPlan.objects.all().using(db1).count()
    count2 = output.models.FlowPlan.objects.all().using(db2).count()
    self.assertEqual(count1new,count1)
    self.assertNotEqual(count2,0)
    self.assertNotEqual(count2,count1new)


class FixtureTest(TestCase):

  def setUp(self):
    self.fixture_dir = os.path.join(settings.FREPPLE_APP, 'freppledb', 'input', 'fixtures')

  def test_fixture_tutorial_1(self):
    try:
      full_path = os.path.join(self.fixture_dir, 'tutorial_1.json')
      objects = serializers.deserialize("json", open(full_path, 'r'))
      for obj in objects: True
    except Exception as e:
      self.fail("Invalid fixture: %s" % e)

  def test_fixture_small_demo(self):
    try:
      full_path = os.path.join(self.fixture_dir, 'small_demo.json')
      objects = serializers.deserialize("json", open(full_path, 'r'))
      for obj in objects: True
    except Exception as e:
      self.fail("Invalid fixture: %s" % e)

  def test_fixture_jobshop(self):
    try:
      full_path = os.path.join(self.fixture_dir, 'jobshop.json')
      objects = serializers.deserialize("json", open(full_path, 'r'))
      for obj in objects: True
    except Exception as e:
      self.fail("Invalid fixture: %s" % e)

  def test_fixture_unicode_test(self):
    try:
      full_path = os.path.join(self.fixture_dir, 'unicode_test.json')
      objects = serializers.deserialize("json", open(full_path, 'r'))
      for obj in objects: True
    except Exception as e:
      self.fail("Invalid fixture: %s" % e)
