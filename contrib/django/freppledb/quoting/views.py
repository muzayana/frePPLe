#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import json, httplib

from django.views.decorators.csrf import csrf_protect
from django.contrib.auth.decorators import login_required
from django.utils.translation import ugettext_lazy as _
from django import forms
from django.utils.encoding import iri_to_uri
from django.http import Http404, HttpResponseServerError, HttpResponse
from django.contrib.admin.widgets import ForeignKeyRawIdWidget, AdminSplitDateTime
from django.db import DEFAULT_DB_ALIAS

from freppledb.input.models import Demand, Item, Customer
from freppledb.common.models import Parameter
from freppledb.common.report import GridReport, GridFieldDateTime, GridFieldText, GridFieldInteger
from freppledb.common.report import GridFieldNumber, GridFieldLastModified

import logging
logger = logging.getLogger(__name__)


BOUNDARY = '----------ThIs_Is_tHe_bouNdaRY_$'

from freppledb.admin import data_site

class QuoteForm(forms.ModelForm):
  # ASSUMPTION: quoting is assumed to be on the default database only
  due = forms.DateField(widget=AdminSplitDateTime())
  customer = forms.ModelChoiceField(queryset=Customer.objects.all(), widget=ForeignKeyRawIdWidget(Demand._meta.get_field("customer").rel, data_site, using=DEFAULT_DB_ALIAS))
  item = forms.ModelChoiceField(queryset=Item.objects.all(), widget=ForeignKeyRawIdWidget(Demand._meta.get_field("item").rel, data_site, using=DEFAULT_DB_ALIAS))
  class Meta:
    model = Demand
    fields = ('name', 'description', 'item', 'customer', 'quantity', 'due', 'minshipment', 'maxlateness')


class QuoteReport(GridReport):
  template = 'quoting/quote.html'
  title = _('Order quotes')
  basequeryset = Demand.objects.all().filter(status='quote')
  permissions = (('view_quote_report','Can view quote report'),)
  model = Demand
  frozenColumns = 1
  multiselect = False
  editable = False
  height = 150

  rows = (
    GridFieldText('name', title=_('name'), key=True, formatter='demand'),
    GridFieldText('item', title=_('item'), field_name='item__name', formatter='item'),
    GridFieldText('customer', title=_('customer'), field_name='customer__name', formatter='customer'),
    GridFieldText('description', title=_('description')),
    GridFieldText('category', title=_('category')),
    GridFieldText('subcategory', title=_('subcategory')),
    GridFieldDateTime('due', title=_('due')),
    GridFieldNumber('quantity', title=_('quantity')),
    GridFieldText('operation', title=_('delivery operation'), formatter='operation'),
    GridFieldInteger('priority', title=_('priority')),
    GridFieldText('owner', title=_('owner'), formatter='demand'),
    GridFieldNumber('maxlateness', title=_('maximum lateness')),
    GridFieldNumber('minshipment', title=_('minimum shipment')),
    GridFieldLastModified('lastmodified'),
    )

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    return {
      'form': QuoteForm(),
      }


@login_required
@csrf_protect
def InfoView(request, action):
  '''
  This view is a proxy for the order quoting service.
  This design mimics the interaction with the order quoting service which also
  external systems (eg ERP system or web shop frontend) would make.

  TODO  A more robust and secure version of this view would better validate and
  control the data sent to order quoting service: less logic in the browser HTML
  and more in this view...
  '''
  if request.method != 'POST' or not request.is_ajax():
    raise Http404('Only ajax get requests allowed')
  try:
    # Should we cache the URL parameter?
    #   +: one less database query
    #   -: parameter value change only takes effect upon restart
    url = Parameter.getValue('quoting.service_location', database=request.database, default="localhost:8001")
    conn = httplib.HTTPConnection(url)
    if action == 'info':
      data = json.loads(request.body)
      conn.request("GET", '/demand/' + iri_to_uri(data[0]) + '/?plan=P')
    elif action == 'cancel':
      data = request.GET['name']
      conn.request("POST", '/demand/' + iri_to_uri(data) + '/?action=R&persist=1', "", {"content-length": 0})
    elif action == 'confirm':
      data = request.GET['name']
      conn.request("POST", '/demand/' + iri_to_uri(data) + '/?status=open&persist=1', "", {"content-length": 0})
    elif action == 'inquiry' or action == 'quote':
      data = '\r\n'.join([
        '--' + BOUNDARY,
        'Content-Disposition: form-data; name="xmldata"',
        '',
        request.body,
        '--' + BOUNDARY + '--',
        ''
        ])
      headers = {
        "Content-type": 'multipart/form-data; boundary=%s' % BOUNDARY,
        "content-length": len(data)
        }
      conn.request("POST", "/%s" % action, data, headers)
    else:
      raise Exception('Invalid action')
    response = conn.getresponse()
    result = response.read()
    conn.close()
    if response.status == httplib.OK:
      return HttpResponse(result, mimetype="text/plain")
    else:
      return HttpResponseServerError(result, mimetype="text/plain")
  except Exception as e:
    msg = "Error: %s" % e
    logger.error(msg)
    return HttpResponseServerError(msg)
