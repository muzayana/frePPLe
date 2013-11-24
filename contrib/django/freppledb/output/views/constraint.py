#
# Copyright (C) 2010-2012 by Johan De Taeye, frePPLe bvba
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public
# License along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

from django.utils.translation import ugettext_lazy as _
from django.utils.translation import string_concat
from django.db.models import Count

from freppledb.output.models import Constraint
from freppledb.common.report import GridReport, GridFieldText, GridFieldNumber, GridFieldDateTime


entities = (
 ('demand',_('demand')),
 ('material',_('material')),
 ('capacity',_('capacity')),
 ('operation',_('operation'))
 )

names = (
  ('overload',_('overload')),
  ('underload',_('underload')),
  ('material excess',_('material excess')),
  ('material shortage',_('material shortage')),
  ('excess',_('excess')),
  ('short',_('short')),
  ('early',_('early')),
  ('late',_('late')),
  ('unplanned',_('unplanned')),
  ('precedence',_('precedence')),
  ('before fence',_('before fence')),
  ('before current',_('before current'))
  )


def getEntities(request):
  return tuple([
    (i['entity'], string_concat(_(i['entity']),":",i['id__count']))
    for i in Constraint.objects.using(request.database).values('entity').annotate(Count('id')).order_by('entity')
    ])


def getNames(request):
  return tuple([
    (i['name'], string_concat(_(i['name']),":",i['id__count']))
    for i in Constraint.objects.using(request.database).values('name').annotate(Count('id')).order_by('name')
    ])


class BaseReport(GridReport):
  '''
  A list report to show constraints.
  '''
  template = 'output/constraint.html'
  title = _("Constraint Report")
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
    return {'active_tab': 'constraint'}


class ReportByDemand(BaseReport):

  template = 'output/constraint_demand.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      return Constraint.objects.all().filter(demand__exact=args[0])
    else:
      return Constraint.objects.all()


class ReportByBuffer(BaseReport):

  template = 'output/constraint_buffer.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      return Constraint.objects.all().filter(owner__exact=args[0], entity__exact='material')
    else:
      return Constraint.objects.all()


class ReportByResource(BaseReport):

  template = 'output/constraint_resource.html'

  @ classmethod
  def basequeryset(reportclass, request, args, kwargs):
    if args and args[0]:
      return Constraint.objects.all().filter(owner__exact=args[0], entity__exact='capacity')
    else:
      return Constraint.objects.all()
