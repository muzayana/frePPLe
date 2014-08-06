#
# Copyright (C) 2014 by Johan De Taeye, frePPLe bvba
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

from django.middleware.csrf import get_token
from django.utils.translation import ugettext_lazy as _
from django.utils.encoding import force_unicode

from freppledb.common.dashboard import Dashboard, Widget


class ExecuteWidget(Widget):
  name = "execute"
  title = _("Execute")
  permissions = (("generate_plan", "Can generate plans"),)
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
      ''' % (_thread_locals.request.prefix, get_token(_thread_locals.request), force_unicode(_("Create a plan")))

Dashboard.register(ExecuteWidget)
