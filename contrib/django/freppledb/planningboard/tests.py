#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
import http.client
import os
import _thread
import time
import xml.etree.ElementTree as ElementTree
import json
from cherrypy.process import servers

from django.core import management
from django.test import TestCase
from django.db import close_old_connections

from freppledb.common.models import Parameter


class baseTest(TestCase):

  @staticmethod
  def runService():
    management.call_command('frepple_run', plantype=1, env="webservice")
    # Workaround: Django doesn't properly clean up database connections when a thread exits
    close_old_connections()


  @classmethod
  def setUpClass(cls):
    # Init
    cls.url = Parameter.getValue('quoting.service_location')
    (cls.host, cls.port) = cls.url.split(':')

    # Check port is free
    servers.wait_for_free_port(cls.host, cls.port)

    # Start the service asynchronously
    os.environ['FREPPLE_TEST'] = "YES"
    _thread.start_new_thread(baseTest.runService, ())

    # Wait till port is occupied
    # This method waits for up to 50 seconds. Hopefully that's enough.
    servers.wait_for_occupied_port(cls.host, cls.port)


  @classmethod
  def tearDownClass(cls):
    del os.environ['FREPPLE_TEST']
    # Stop the service
    management.call_command('frepple_stop_web_service', force=True)
    servers.wait_for_free_port(cls.host, cls.port)
    time.sleep(1)  # Just to be sure all database connections are closed
    

class apiTest(baseTest):

  fixtures = ["demo"]

  def validateXML(self, method, url):
    self.conn.request(method, url)
    resp = self.conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    data = resp.read()
    ElementTree.fromstring(data)  
        
  def testGetXML(self):
    # Luckily the dataset is small enough to load all data in memory
    self.conn = http.client.HTTPConnection(self.url, timeout=50)
    self.validateXML("GET", '/')
    self.validateXML("GET", '/customer')
    self.validateXML("GET", '/buffer')
    self.validateXML("GET", '/resource')
    self.validateXML("GET", '/location')
    self.validateXML("GET", '/item')
    self.validateXML("GET", '/resource')
    self.validateXML("GET", '/buffer')
    self.validateXML("GET", '/calendar')
    self.validateXML("GET", '/operation')
    self.validateXML("GET", '/problem')
    self.validateXML("GET", '/setupmatrix')
    self.validateXML("GET", '/supplier')

  def testGetXMLDetail(self):
    # Luckily the dataset is small enough to load all data in memory
    self.conn = http.client.HTTPConnection(self.url, timeout=50)
    self.validateXML("GET", '/?type=detail')
    self.validateXML("GET", '/customer?type=detail')
    self.validateXML("GET", '/buffer?type=detail')
    self.validateXML("GET", '/resource?type=detail')
    self.validateXML("GET", '/location?type=detail')
    self.validateXML("GET", '/item?type=detail')
    self.validateXML("GET", '/resource?type=detail')
    self.validateXML("GET", '/buffer?type=detail')
    self.validateXML("GET", '/calendar?type=detail')
    self.validateXML("GET", '/operation?type=detail')
    self.validateXML("GET", '/problem?type=detail')
    self.validateXML("GET", '/setupmatrix?type=detail')
    self.validateXML("GET", '/supplier?type=detail')

  def validateJSON(self, method, url):
    self.conn.request(method, url)
    resp = self.conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    json.loads(resp.readall().decode('utf-8'))

  def testGetJSON(self):
    # Luckily the dataset is small enough to load all data in memory
    self.conn = http.client.HTTPConnection(self.url, timeout=50)
    self.validateJSON("GET", '/?format=json')
    self.validateJSON("GET", '/customer?format=json')
    self.validateJSON("GET", '/buffer?format=json')
    self.validateJSON("GET", '/resource?format=json')
    self.validateJSON("GET", '/location?format=json')
    self.validateJSON("GET", '/item?format=json')
    self.validateJSON("GET", '/resource?format=json')
    self.validateJSON("GET", '/buffer?format=json')
    self.validateJSON("GET", '/calendar?format=json')
    self.validateJSON("GET", '/operation?format=json')
    self.validateJSON("GET", '/problem?format=json')
    self.validateJSON("GET", '/setupmatrix?format=json')
    self.validateJSON("GET", '/supplier?format=json')

  def testGetJSONDetail(self):
    # Luckily the dataset is small enough to load all data in memory
    self.conn = http.client.HTTPConnection(self.url, timeout=50)
    self.validateJSON("GET", '/?format=json&type=detail')
    self.validateJSON("GET", '/customer?format=json&type=detail')
    self.validateJSON("GET", '/buffer?format=json&type=detail')
    self.validateJSON("GET", '/resource?format=json&type=detail')
    self.validateJSON("GET", '/location?format=json&type=detail')
    self.validateJSON("GET", '/item?format=json&type=detail')
    self.validateJSON("GET", '/resource?format=json&type=detail')
    self.validateJSON("GET", '/buffer?format=json&type=detail')
    self.validateJSON("GET", '/calendar?format=json&type=detail')
    self.validateJSON("GET", '/operation?format=json&type=detail')
    self.validateJSON("GET", '/problem?format=json&type=detail')
    self.validateJSON("GET", '/setupmatrix?format=json&type=detail')
    self.validateJSON("GET", '/supplier?format=json&type=detail')
