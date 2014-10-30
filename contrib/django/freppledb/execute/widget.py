#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.middleware.csrf import get_token
from django.utils.translation import ugettext_lazy as _
from django.utils.encoding import force_text

from freppledb.common.dashboard import Dashboard, Widget


class ExecuteWidget(Widget):
  name = "execute"
  title = _("Execute")
  permissions = (("generate_plan", "Can generate plans"),)
  tooltip = _("Generate a constrained plan")
  async = False
  url = '/execute/'

  def render(self, request=None):
    from freppledb.common.middleware import _thread_locals
    return '''<div style="text-align:center">
      <form method="post" action="%s/execute/launch/frepple_run/">
      <input type="hidden" name="csrfmiddlewaretoken" value="%s">
      <input type="hidden" name="plantype" value="1"/>
      <input type="hidden" name="constraint" value="15"/>
      <input class="button" type="submit" value="%s"/>
      </form></div>
      ''' % (_thread_locals.request.prefix, get_token(_thread_locals.request), force_text(_("Create a plan")))

Dashboard.register(ExecuteWidget)
