#
# Copyright (C) 2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
import http.client
import json
import os
import _thread
import time
from xml.dom import minidom
from cherrypy.process import servers

from django.core import management
from django.test import TransactionTestCase
from django.db import close_old_connections

from freppledb.common.models import Parameter


class baseTest(TransactionTestCase):

  fixtures = ["demo"]

  @staticmethod
  def runService():
    management.call_command('frepple_run', plantype=1, env="webservice")
    # Workaround: Django doesn't properly clean up database connections when a thread exits
    close_old_connections()


  def setUp(self):
    # Init
    self.url = Parameter.getValue('quoting.service_location', default="localhost:8001")
    (self.host, self.port) = self.url.split(':')

    # Check port is free
    servers.wait_for_free_port(self.host, self.port)

    # Start the service asynchronously
    os.environ['FREPPLE_TEST'] = "YES"
    _thread.start_new_thread(baseTest.runService, ())

    # Wait till port is occupied
    # This method waits for up to 50 seconds. Hopefully that's enough.
    servers.wait_for_occupied_port(self.host, self.port)


  def tearDown(self):
    del os.environ['FREPPLE_TEST']

    # Stop the service
    management.call_command('frepple_stop_web_service')
    servers.wait_for_free_port(self.host, self.port)
    time.sleep(1)  # Just to be sure all database connections are closed


  boundary = "----------ThIs_Is_tHe_bouNdaRY_$"


  xmltemplate = '''<?xml version="1.0" encoding="UTF-8" ?>
    <plan xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <demands>
     <demand name="%(name)s">
      <customer name="%(customer)s" action="C"/>
      <quantity>%(quantity)d</quantity>
      <item name="%(item)s" action="C"/>
      <location name="%(location)s" action="C"/>
      <due>%(due)s</due>
      <minshipment>%(minshipment)d</minshipment>
      <maxlateness>P%(maxlateness)dD</maxlateness>
     </demand>
    </demands>
    </plan>'''


  def buildQuoteXML(self, name=None, customer=None, location=None,
        quantity=1, item=None, due=None, minshipment=1, maxlateness=1000):
    msg = '\r\n'.join([
      '--' + self.boundary,
      'Content-Disposition: form-data; name="xmldata"',
      '',
      self.xmltemplate % {
        'name': name, 'customer': customer, 'quantity': quantity,
        'item': item, 'due': due, 'minshipment': minshipment,
        'maxlateness': maxlateness, 'location': location
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
    result = {}
    for i in minidom.parseString(data.decode('utf-8')).getElementsByTagName("operationplan"):
      end = i.getElementsByTagName("end")[0]
      qty = i.getElementsByTagName("quantity")[0]
      result[self.getXMLText(end)] =  result.get(self.getXMLText(end), 0) + float(self.getXMLText(qty))
    return result

  def parseJSONResponse(self, data):
    return json.loads(data.decode('utf-8'))


class apiTest(baseTest):

  def testURLs(self):
    # Note: slash after category name is optional
    conn = http.client.HTTPConnection(self.url)
    conn.request("GET", '/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/customer/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/buffer')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/resource/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/location')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/item/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/flow')   # TODO returns OK, but empty which isn't right
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/load/')   # TODO returns OK, but empty which isn't right
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/calendar/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/operation')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/problem/?type=plan')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/setupmatrix')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/supplier/')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())

    # Note: slash after category name is optional
    conn = http.client.HTTPConnection(self.url)
    conn.request("GET", '/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/customer/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/buffer/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/resource/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/location/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/item/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(resp.read())
    conn.request("GET", '/flow/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    #self.assertTrue(self.parseJSONResponse(resp.read())) # TODO returns OK, but empty which isn't right
    conn.request("GET", '/load/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    #self.assertTrue(self.parseJSONResponse(resp.read())) # TODO returns OK, but empty which isn't right
    conn.request("GET", '/calendar/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/operation/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/problem?format=json&type=plan')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))
    conn.request("GET", '/setupmatrix/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertEqual(self.parseJSONResponse(resp.read()), {})
    conn.request("GET", '/supplier/?format=json')
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    self.assertTrue(self.parseJSONResponse(resp.read()))


class quoteAndInquiry(baseTest):

  def testQuoteAndInquiry(self):
    conn = http.client.HTTPConnection(self.url)

    # Send a first inquiry
    (msg1, headers1) = self.buildQuoteXML(
      name="test", customer="Customer near factory 1",
      quantity=100, item="product", location="factory 1",
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
      quantity=100, item="product", location="factory 1",
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

    conn.close()


class requoteTest(baseTest):

  def testRequote(self):
    conn = http.client.HTTPConnection(self.url)

    # Send a quote
    (msg, headers) = self.buildQuoteXML(
      name="test", customer="Customer near factory 1",
      quantity=100, item="product", location="factory 1",
      due='2013-01-01T00:00:00', minshipment=1
      )
    conn.request("POST", "/quote/", msg, headers)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    firstQuote = self.parseQuoteResponse(resp.read())

    # Cancel the quote
    conn.request("DELETE", "/demand/test?persist=1")
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    resp.read()

    # Resend the quote
    conn.request("POST", "/quote/", msg, headers)
    resp = conn.getresponse()
    self.assertEqual(resp.status, http.client.OK)
    secondQuote = self.parseQuoteResponse(resp.read())
    self.assertEqual(firstQuote, secondQuote, "Expecting the repeated quote to be identical to the original")

    conn.close()
