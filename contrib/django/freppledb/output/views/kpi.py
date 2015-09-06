#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _
from django.db import connections

from freppledb.common.db import sql_datediff
from freppledb.common.models import Parameter
from freppledb.common.report import GridReport, GridFieldText, GridFieldInteger


class Report(GridReport):
  title = _("Performance Indicators")
  frozenColumns = 0
  basequeryset = Parameter.objects.all()
  permissions = (("view_kpi_report", "Can view kpi report"),)
  rows = (
    GridFieldText('category', title=_('category'), sortable=False, editable=False, align='center'),
    GridFieldText('name', title=_('name'), sortable=False, editable=False, align='center'),
    GridFieldInteger('value', title=_('value'), sortable=False, editable=False, align='center'),
    )
  default_sort = (1, 'asc')
  filterable = False
  multiselect = False

  @staticmethod
  def query(request, basequery):
    # Execute the query
    cursor = connections[request.database].cursor()
    query = '''
      select 101 as id, 'Problem count' as category, name as name, count(*) as value
      from out_problem
      group by name
      union all
      select 102, 'Problem weight', name, round(sum(weight))
      from out_problem
      group by name
      union all
      select 201, 'Demand', 'Requested', coalesce(round(sum(quantity)),0)
      from out_demand
      union all
      select 202, 'Demand', 'Planned', coalesce(round(sum(planquantity)),0)
      from out_demand
      union all
      select 203, 'Demand', 'Planned late', coalesce(round(sum(planquantity)),0)
      from out_demand
      where plandate > due and plandate is not null
      union all
      select 204, 'Demand', 'Unplanned', coalesce(round(sum(quantity)),0)
      from out_demand
      where planquantity is null
      union all
      select 205, 'Demand', 'Total lateness', coalesce(round(sum(planquantity * %s)),0)
      from out_demand
      where plandate > due and plandate is not null
      union all
      select 301, 'Operation', 'Count', count(*)
      from out_operationplan
      union all
      select 301, 'Operation', 'Quantity', coalesce(round(sum(quantity)),0)
      from out_operationplan
      union all
      select 302, 'Resource', 'Usage', coalesce(round(sum(quantity * %s)),0)
      from out_loadplan
      union all
      select 401, 'Material', 'Produced', coalesce(round(sum(quantity)),0)
      from out_flowplan
      where quantity>0
      union all
      select 402, 'Material', 'Consumed', coalesce(round(sum(-quantity)),0)
      from out_flowplan
      where quantity<0
      order by 1
      ''' % (
        sql_datediff('plandate', 'due'),
        sql_datediff('enddate', 'startdate')
      )
    cursor.execute(query)

    # Build the python result
    for row in cursor.fetchall():
      yield {
        'category': row[1],
        'name': row[2],
        'value': row[3],
        }
