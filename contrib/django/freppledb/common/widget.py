#
# Copyright (C) 2014 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.contrib.auth.models import AnonymousUser
from django.contrib.admin.models import LogEntry
from django.db import DEFAULT_DB_ALIAS
from django.utils.translation import ugettext_lazy as _
from django.utils.text import capfirst
from django.utils.encoding import force_text

from freppledb.common.dashboard import Dashboard, Widget


class WelcomeWidget(Widget):
  name = "welcome"
  title = _("Welcome")
  tooltip = _("Some links to get started")
  async = False

  def render(self, request=None):
    return _('''Welcome to frePPLe, the world's leading open source production planning tool!<br/><br/>
How to get started?
<ol><li>Start the <span class="underline"><a href="javascript:void(0);" onclick="tour.start('0,0,0'); return false;">guided tour</a></span></li>
<li>Check out the <span class="underline"><a href="/static/doc/index.html" target="_blank">documentation</a></span></li>
<li>Visit and join the <span class="underline"><a href="http://groups.google.com/group/frepple-users" target="_blank">user community</a></span></li>
<li><span class="underline"><a href="http://frepple.com/contact/" target="_blank">Contact us</a></span></li>
</ol>
''')

Dashboard.register(WelcomeWidget)


class NewsWidget(Widget):
  name = "news"
  title = _("News")
  tooltip = _("Show the latest news items from the frePPLe website")
  async = False

  def render(self, request=None):
    return '<iframe style="width:100%; border:none;" src="http://frepple.com/news-summary/"></iframe>'

Dashboard.register(NewsWidget)


class RecentActionsWidget(Widget):
  name = "recent_actions"
  title = _("My actions")
  tooltip = _("Display a list of the entities you recently changed")
  async = False
  limit = 10

  def render(self, request=None):
    # This code is a slightly modified version of a standard Django tag.
    # The only change is to look for the logentry records in the right database.
    # See the file django\contrib\admin\templatetags\log.py
    from freppledb.common.middleware import _thread_locals
    try:
      db = _thread_locals.request.database or DEFAULT_DB_ALIAS
    except:
      db = DEFAULT_DB_ALIAS
    if isinstance(_thread_locals.request.user, AnonymousUser):
      q = LogEntry.objects.using(db).select_related('content_type', 'user')[:self.limit]
    else:
      q = LogEntry.objects.using(db).filter(user__id__exact=_thread_locals.request.user.pk).select_related('content_type', 'user')[:self.limit]
    result = []
    for entry in q:
      if entry.is_change():
        result.append('<span style="display: inline-block;" class="ui-icon ui-icon-pencil"></span><a href="%s%s">%s</a>' % (_thread_locals.request.prefix, entry.get_admin_url(), entry.object_repr))
      elif entry.is_addition():
        result.append('<span style="display: inline-block;" class="ui-icon ui-icon-plusthick"></span><a href="%s%s">%s</a>' % (_thread_locals.request.prefix, entry.get_admin_url(), entry.object_repr))
      elif entry.is_deletion():
        result.append('<span style="display: inline-block;" class="ui-icon ui-icon-minusthick"></span>%s' % entry.object_repr)
      else:
        raise "Unexpected log entry type"
      if entry.content_type:
        result.append('<span class="mini">%s</span><br/>' % capfirst(force_text(_(entry.content_type.name))) )
      else:
        result.append('<span class="mini">%s</span><br/>' % force_text(_('Unknown content')))
    return result and '\n'.join(result) or force_text(_('None available'))

Dashboard.register(RecentActionsWidget)
