#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import json, httplib, urllib, urllib2

from django.views.decorators.csrf import csrf_protect
from django.contrib.auth.decorators import login_required
from django.shortcuts import render_to_response
from django.db.models.fields.related import RelatedField
from django.utils.translation import ugettext_lazy as _
from django.template import RequestContext
from django import forms
from django.utils.encoding import iri_to_uri
from django.forms.models import modelform_factory
from django.http import Http404, HttpResponseServerError, HttpResponse

from freppledb.input.models import Demand
from freppledb.output.models import Demand as DemandOut
from freppledb.output.models import Constraint, Problem
from freppledb.common.models import Parameter
from freppledb.common.report import GridReport, GridFieldDateTime, GridFieldText, GridFieldInteger
from freppledb.common.report import GridFieldNumber, GridFieldLastModified


import logging
logger = logging.getLogger(__name__)


BOUNDARY = '----------ThIs_Is_tHe_bouNdaRY_$'


class QuoteForm(forms.ModelForm):
  class Meta:
    model = Demand
    #customer = forms.ModelChoiceField(queryset=Customer.objects.all(), widget=forms.TextInput)
    #item = forms.ModelChoiceField(queryset=Item.objects.all(), widget=forms.TextInput)
    fields = ('name', 'description', 'item', 'customer', 'quantity', 'due', 'minshipment', 'maxlateness')
    #formfield_callback = lambda f: (isinstance(f, RelatedField) and f.formfield(using=request.database, localize=True)) or f.formfield(localize=True)
    #widgets = {
    #  'item': forms.TextInput(attrs={'cols': 80, 'rows': 20}),
    #  }


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
  if request.method != 'POST' or not request.is_ajax():
    raise Http404('Only ajax get requests allowed')
  try:
    # Should we cache the url value?
    #   +: one less database query
    #   -: parameter value change only takes effect upon restart
    url = Parameter.getValue('quoting.service_location', database=request.database, default="localhost:8001")
    conn = httplib.HTTPConnection(url)
    if action == 'info':
      data = json.loads(request.body)
      conn.request("GET", '/demand/' + iri_to_uri(data[1]) + '/?plan=P')
    elif action == 'cancel':
      data = json.loads(request.body)
      conn.request("POST", '/demand/' + iri_to_uri(data[1]) + '/?action=D')
    elif action == 'confirm':
      data = json.loads(request.body)
      conn.request("POST", '/demand/' + iri_to_uri(data[1]) + '/?status=open')
    elif action == 'enquiry' or action == 'quote':
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
    return HttpResponse(result, mimetype="text/plain")
  except Exception as e:
    msg = "Error getting quote info: %s" % e
    logger.error(msg)
    return HttpResponseServerError(msg)

