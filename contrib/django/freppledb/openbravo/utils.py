#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import base64
import http.client


def get_data(url, host, user, password):
  '''
  Utility function to read data from the Openbravo web service.
  '''
  # Connect to openbravo
  webservice = http.client.HTTPConnection(host)
  webservice.putrequest("GET", url)
  webservice.putheader("Host", host)
  webservice.putheader("User-Agent", "frePPLe-Openbravo connector")
  webservice.putheader("Content-type", "text/html; charset=\"UTF-8\"")
  webservice.putheader("Content-length", "0")
  webservice.putheader("Authorization", "Basic %s" % base64.encodestring(
    ('%s:%s' % (user, password)).replace('\n', '').encode("utf-8")).decode("utf-8")
    )
  webservice.endheaders()
  webservice.send('')

  # Get the openbravo response
  response = webservice.getresponse()
  if response.status != http.client.OK:
    raise Exception(response.reason)
  return response.read().decode("utf-8")


def post_data(xmldoc, url, host, user, password):
  '''
  Utility function to post data to the Openbravo web service.
  '''
  # Send the data to openbravo
  webservice = http.client.HTTPConnection(host)
  webservice.putrequest("POST", url)
  webservice.putheader("Host", host)
  webservice.putheader("User-Agent", "frePPLe-Openbravo connector")
  webservice.putheader("Content-type", 'text/xml')
  webservice.putheader("Content-length", str(len(xmldoc)))
  webservice.putheader("Authorization", "Basic %s" % base64.encodestring(
    ('%s:%s' % (user, password)).replace('\n', '').encode("utf-8")).decode("utf-8")
    )
  webservice.endheaders()
  webservice.send(xmldoc)

  # Get the openbravo response
  response = webservice.getresponse()
  if response.status != http.client.OK:
    raise Exception(response.reason)


def delete_data(url, host, user, password):
  '''
  Utility function to delete data from the Openbravo web service.
  '''
  # Connect to openbravo
  webservice = http.client.HTTPConnection(host)
  webservice.putrequest("DELETE", url)
  webservice.putheader("Host", host)
  webservice.putheader("User-Agent", "frePPLe-Openbravo connector")
  webservice.putheader("Content-type", "text/html; charset=\"UTF-8\"")
  webservice.putheader("Content-length", "0")
  webservice.putheader("Authorization", "Basic %s" % base64.encodestring(
    ('%s:%s' % (user, password)).replace('\n', '').encode("utf-8")).decode("utf-8")
    )
  webservice.endheaders()
  webservice.send('')

  # Get the openbravo response
  response = webservice.getresponse()
  if response.status != http.client.OK:
    raise Exception(response.reason)
  return response.read().decode("utf-8")

