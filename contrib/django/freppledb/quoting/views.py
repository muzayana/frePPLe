#
# Copyright (C) 2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.  
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code 
# or in the form of compiled binaries.
#

import uuid
from datetime import datetime

from django.views.decorators.csrf import csrf_protect
from django.contrib.auth.decorators import login_required
from django.shortcuts import render_to_response
from django.db.models.fields.related import RelatedField

from django.db import connections, transaction
from django.utils import simplejson
from django.utils.translation import ugettext_lazy as _
from django.utils.text import capfirst
from django.utils.encoding import force_unicode
from django.contrib.admin.views.decorators import staff_member_required
from django.http import HttpResponse
from django.template import RequestContext, loader
from django.conf import settings
from django import forms
from django.forms import ModelForm
from django.forms.models import modelform_factory


from freppledb.input.models import Demand, Customer, Operation, Item


class QuoteForm(forms.ModelForm):
  class Meta:
    model = Demand
    customer = forms.ModelChoiceField(queryset=Customer.objects.all(), widget=forms.TextInput)
    item = forms.ModelChoiceField(queryset=Item.objects.all(), widget=forms.TextInput)
    fields = ()
    widgets = {
      'item': forms.TextInput(attrs={'cols': 80, 'rows': 20}),
      }
  
@login_required
@csrf_protect
def Main(request):
  if request.method == 'POST':
    # Pick up form info
    form = QuoteForm(request.POST)
    if form.is_valid():
      try:
        print "ok"
        print '''
        <?xml version="1.0" encoding="UTF-8" ?>
<plan xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
<demands>
 <demand name="Order quote %s">
  <customer name="%s" action="C"/>
  <quantity>%s</quantity>
  <item name="%s" action="C"/>
  <due>%s</due>     {yyyy-mm-ddTHH:MM:SS}
  %s      <minshipment></minshipment>
  %s      <maxlateness>{PzzzD}</maxlateness>
 </demand>
</demands>
</plan>''' #% (uuid.uuid1(), )
      except Exception as e:
        print "not ok"
      
  else:
    form = modelform_factory(Demand,
        fields = ('name','customer', 'item', 'quantity', 'due', 'minshipment', 'maxlateness'),
        formfield_callback = lambda f: (isinstance(f, RelatedField) and f.formfield(using=request.database, localize=True)) or f.formfield(localize=True)
        )
  return render_to_response('quoting/quote.html', {
     'title': _('Quote an order'),
     'form': form,
     },
     context_instance=RequestContext(request))
 
