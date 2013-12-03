#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
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

from freppledb.output.models import Problem
from freppledb.common.report import GridReport, GridFieldText, GridFieldNumber, GridFieldDateTime


def getEntities(request):
  return tuple([
    (i['entity'], string_concat(_(i['entity']),":",i['id__count']))
    for i in Problem.objects.using(request.database).values('entity').annotate(Count('id')).order_by('entity')
    ])


def getNames(request):
  return tuple([
    (i['name'], string_concat(_(i['name']),":",i['id__count']))
    for i in Problem.objects.using(request.database).values('name').annotate(Count('id')).order_by('name')
    ])


class Report(GridReport):
  '''
  A list report to show problems.
  '''
  template = 'output/problem.html'
  title = _("Problem Report")
  basequeryset = Problem.objects # TODO .extra(select={'forecast': "select name from forecast where out_problem.owner like forecast.name || ' - %%'",})
  model = Problem
  permissions = (("view_problem_report", "Can view problem report"),)
  frozenColumns = 0
  editable = False
  multiselect = False
  rows = (
    GridFieldText('entity', title=_('entity'), editable=False, align='center'), # TODO choices=getEntities
    GridFieldText('name', title=_('name'), editable=False, align='center'),  # TODO choices=getNames
    GridFieldText('owner', title=_('owner'), editable=False, extra='formatter:probfmt'),
    GridFieldText('description', title=_('description'), editable=False, width=350),
    GridFieldDateTime('startdate', title=_('start date'), editable=False),
    GridFieldDateTime('enddate', title=_('end date'), editable=False),
    GridFieldNumber('weight', title=_('weight'), editable=False),
    )
