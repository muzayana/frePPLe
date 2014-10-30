#
# Copyright (C) 2013 by Johan De Taeye, frePPLe bvba
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
from xml.dom import minidom
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


  boundary = "----------ThIs_Is_tHe_bouNdaRY_$"


  xmltemplate = '''<?xml version="1.0" encoding="UTF-8" ?>
    <plan xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <demands>
     <demand name="%(name)s">
      <customer name="%(customer)s" action="C"/>
      <quantity>%(quantity)d</quantity>
      <item name="%(item)s" action="C"/>
      <due>%(due)s</due>
      <minshipment>%(minshipment)d</minshipment>
      <maxlateness>P%(maxlateness)dD</maxlateness>
     </demand>
    </demands>
    </plan>'''


  def buildQuoteXML(self, name=None, customer=None, quantity=1, item=None,
        due=None, minshipment=1, maxlateness=1000):
    msg = '\r\n'.join([
      '--' + self.boundary,
      'Content-Disposition: form-data; name="xmldata"',
      '',
      self.xmltemplate % {
        'name': name, 'customer': customer, 'quantity': quantity,
        'item': item, 'due': due, 'minshipment': minshipment,
        'maxlateness': maxlateness
        },
      '--' + self.boundary + '--',
      ''
      ])
    headers = {
      "Content-type": 'multipart/form-data; boundary=%s' % self.boundary,
      "content-length": len(msg)
      }
    return (msg, headers)


  def getXMLText(self, node):
    rc = []
    for n in node.childNodes:
      if n.nodeType == n.TEXT_NODE:
        rc.append(n.data)
    return ''.join(rc)


  def parseQuoteResponse(self, data):
    #print ('XML reply', data)
    result = []
    for i in minidom.parseString(data).getElementsByTagName("operationplan"):
      end = i.getElementsByTagName("end")[0]
      qty = i.getElementsByTagName("quantity")[0]
      result.append( (self.getXMLText(end), self.getXMLText(qty)) )
    return result


class apiTest(baseTest):

  fixtures = ["demo"]

  def testReloadReplan(self):
    # Get original model
    conn = http.client.HTTPConnection(self.url, timeout=50)
    conn.request("GET", '/problem/')
    resp = conn.getresponse()
    oldProblems = resp.read()   # Luckily the dataset is small enough...
    self.assertEqual(resp.status, http.client.OK)
    # Reload the input data
    conn.request("GET", '/reload/')
    resp = conn.getresponse()
    resp.read()
    self.assertEqual(resp.status, http.client.SEE_OTHER)
    conn.request("GET", '/problem/')
    resp = conn.getresponse()
    newProblems = resp.read()
    self.assertNotEqual(newProblems, oldProblems, "Reload didn't work correctly")
    # Replan the reloaded model
    conn.request("GET", '/replan/')
    resp = conn.getresponse()
    resp.read()
    self.assertEqual(resp.status, http.client.SEE_OTHER)
    conn.request("GET", '/problem/')
    resp = conn.getresponse()
    newProblems = resp.read()
    self.assertEqual(newProblems, oldProblems, "Replanning doesn't give the same results")

  def testURLs(self):
    conn = http.client.HTTPConnection(self.url)
    conn.request("GET", '/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/main/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/customer/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/buffer/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/resource/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/location/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/item/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/flow/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/load/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/calendar/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/operation/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/problem/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/setupmatrix/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())


class quoteAndInquiry(baseTest):

  fixtures = ["demo"]

  def testQuoteAndInquiry(self):
    # Send a first inquiry
    conn = http.client.HTTPConnection(self.url)
    (msg1, headers1) = self.buildQuoteXML(
      name="test", customer="Customer near factory 1",
      quantity=100, item="product",
      due='2013-01-01T00:00:00', minshipment=1
      )
    conn.request("POST", "/inquiry/", msg1, headers1)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    firstInquiry = self.parseQuoteResponse(resp.read())

    # Repeat the first inquiry
    conn.request("POST", "/inquiry/", msg1, headers1)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    firstInquiryDouble = self.parseQuoteResponse(resp.read())
    self.assertEqual(firstInquiry, firstInquiryDouble, "Resending a inquiry should return the same result")

    # Send a first quote
    conn.request("POST", "/quote/", msg1, headers1)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    firstQuote = self.parseQuoteResponse(resp.read())
    self.assertEqual(firstInquiry, firstQuote, "Inquiry and quote should return the same result")

    # Send a second inquiry
    (msg2, headers2) = self.buildQuoteXML(
      name="test2", customer="Customer near factory 1",
      quantity=100, item="product",
      due='2013-01-01T00:00:00', minshipment=1
      )
    conn.request("POST", "/inquiry/", msg2, headers2)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    secondInquiry = self.parseQuoteResponse(resp.read())

    # Send a second quote
    conn.request("POST", "/quote/", msg2, headers2)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    secondQuote = self.parseQuoteResponse(resp.read())
    self.assertEqual(secondInquiry, secondQuote, "Inquiry and quote should return the same result")
    self.assertNotEqual(firstQuote, secondQuote, "Expected a different quote")


class requoteTest(baseTest):

  fixtures = ["demo"]

  def testRequote(self):
    # Send a quote
    conn = http.client.HTTPConnection(self.url)
    (msg, headers) = self.buildQuoteXML(
      name="test", customer="Customer near factory 1",
      quantity=100, item="product",
      due='2013-01-01T00:00:00', minshipment=1
      )
    conn.request("POST", "/quote/", msg, headers)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    firstQuote = self.parseQuoteResponse(resp.read())

    # Cancel the quote
    conn.request("POST", '/demand/test/?action=R&persist=1', "", {"content-length": 0})
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    resp.read()

    # Resend the quote
    conn.request("POST", "/quote/", msg, headers)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    secondQuote = self.parseQuoteResponse(resp.read())
    self.assertEqual(firstQuote, secondQuote, "Expecting the repeated quote to be identical to the original")
