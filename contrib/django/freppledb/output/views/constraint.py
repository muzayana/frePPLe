#
# Copyright (C) 2010-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _
from django.utils.translation import string_concat
from django.db.models import Count

from freppledb.output.models import Constraint
from freppledb.common.report import GridReport, GridFieldText, GridFieldNumber, GridFieldDateTime


entities = (
 ('demand', _('demand')),
 ('material', _('material')),
 ('capacity', _('capacity')),
 ('operation', _('operation'))
 )

names = (
  ('overload', _('overload')),
  ('underload', _('underload')),
  ('material excess', _('material excess')),
  ('material shortage', _('material shortage')),
  ('excess', _('excess')),
  ('short', _('short')),
  ('early', _('early')),
  ('late', _('late')),
  ('unplanned', _('unplanned')),
  ('precedence', _('precedence')),
  ('before fence', _('before fence')),
  ('before current', _('before current'))
  )


def getEntities(request):
  return tuple([
    (i['entity'], string_concat(_(i['entity']), ":", i['id__count']))
    for i in Constraint.objects.using(request.database).values('entity').annotate(Count('id')).order_by('entity')
    ])


def getNames(request):
  return tuple([
    (i['name'], string_concat(_(i['name']), ":", i['id__count']))
    for i in Constraint.objects.using(request.database).values('name').annotate(Count('id')).order_by('name')
    ])


class BaseReport(GridReport):
  '''
  A list report to show constraints.
  '''
  template = 'output/constraint.html'
  title = _("Constraint report")
  basequeryset = Constraint.objects.all()
  model = Constraint
  permissions = (("view_constraint_report", "Can view constraint report"),)
  frozenColumns = 0
  editable = False
  multiselect = False
  rows = (
    GridFieldText('demand', title=_('demand'), editable=False, formatter='demand'),
    GridFieldText('entity', title=_('entity'), editable=False, width=80, align='center'),
    GridFieldText('name', title=_('name'), editable=False, width=100, align='center'),
    GridFieldText('owner', title=_('owner'), editable=False, extra='formatter:probfmt'),
    GridFieldText('description', title=_('description'), editable=False, width=350),
    GridFieldDateTime('startdate', title=_('start date'), editable=False),
    GridFieldDateTime('enddate', title=_('end date'), editable=False),
    GridFieldNumber('weight', title=_('weight'), editable=False),
    )

  @classmethod
  def extra_context(reportclass, request, *args, **kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'constraint'
    return {'active_tab': 'constraint'}


class ReportByDemand(BaseReport):

  template = 'output/constraint_demand.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'constraint'
      return Constraint.objects.all().filter(demand__exact=args[0])
    else:
      return Constraint.objects.all()


class ReportByBuffer(BaseReport):

  template = 'output/constraint_buffer.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'constraint'
      return Constraint.objects.all().filter(owner__exact=args[0], entity__exact='material')
    else:
      return Constraint.objects.all()


class ReportByOperation(BaseReport):

  template = 'output/constraint_operation.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'constraint'
      return Constraint.objects.all().filter(owner__exact=args[0], entity__exact='operation')
    else:
      return Constraint.objects.all()


class ReportByResource(BaseReport):

  template = 'output/constraint_resource.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      request.session['lasttab'] = 'constraint'
      return Constraint.objects.all().filter(owner__exact=args[0], entity__exact='capacity')
    else:
      return Constraint.objects.all()
