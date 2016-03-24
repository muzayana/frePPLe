#
# Copyright (C) 2014 by frePPLe bvba
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
from django.utils import formats
from django.utils.html import escape
from django.utils.translation import ugettext_lazy as _
from django.utils.text import capfirst
from django.utils.encoding import force_text

from freppledb import VERSION
from freppledb.common.dashboard import Dashboard, Widget
from freppledb.common.models import Comment


class WelcomeWidget(Widget):
  name = "welcome"
  title = _("Welcome")
  tooltip = _("Some links to get started")
  asynchronous = False

  def render(self, request=None):
    versionnumber = VERSION.split('.', 2)
    return _('''Welcome to frePPLe, the world's leading open source production planning tool!<br/><br/>
How to get started?
<ol><li>Start the <span class="underline"><a href="javascript:void(0);" onclick="tour.start('0,0,0'); return false;">guided tour</a></span></li>
<li>Check out the <span class="underline"><a href="%(docurl)s" target="_blank">documentation</a></span></li>
<li>Visit and join the <span class="underline"><a href="http://groups.google.com/group/frepple-users" target="_blank">user community</a></span></li>
<li><span class="underline"><a href="https://frepple.com/contact/" target="_blank">Contact us</a></span></li>
</ol>
''') % {'docurl': "https://frepple.com/docs/%s.%s/" % (versionnumber[0], versionnumber[1])}

Dashboard.register(WelcomeWidget)


class NewsWidget(Widget):
  name = "news"
  title = _("News")
  tooltip = _("Show the latest news items from the frePPLe website")
  asynchronous = False

  def render(self, request=None):
    return '<iframe style="width:100%; border:none;" src="https://frepple.com/news-summary/"></iframe>'

Dashboard.register(NewsWidget)


class RecentActionsWidget(Widget):
  name = "recent_actions"
  #. Translators: Translation included with Django
  title = _("My Actions")
  tooltip = _("Display a list of the entities you recently changed")
  asynchronous = False
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
        result.append('<span style="display: inline-block;" class="fa fa-pencil"></span><a href="%s%s">&nbsp;%s</a>' % (_thread_locals.request.prefix, entry.get_admin_url(), escape(entry.object_repr)))
      elif entry.is_addition():
        result.append('<span style="display: inline-block;" class="fa fa-plus"></span><a href="%s%s">&nbsp;%s</a>' % (_thread_locals.request.prefix, entry.get_admin_url(), escape(entry.object_repr)))
      elif entry.is_deletion():
        result.append('<span style="display: inline-block;" class="fa fa-minus"></span>&nbsp;%s' % escape(entry.object_repr))
      else:
        raise "Unexpected log entry type"
      if entry.content_type:
        result.append('<span class="small">%s</span><br/>' % capfirst(force_text(_(entry.content_type.name))) )
      else:
        result.append('<span class="small">%s</span><br/>' % force_text(_('Unknown content')))
    #. Translators: Translation included with Django
    return result and '\n'.join(result) or force_text(_('None available'))

Dashboard.register(RecentActionsWidget)


class RecentCommentsWidget(Widget):
  name = "recent_comments"
  title = _("comments")
  tooltip = _("Display a list of recent comments")
  url = '/data/common/comment/?sord=desc&sidx=lastmodified'
  asynchronous = False
  limit = 10

  def render(self, request=None):
    from freppledb.common.middleware import _thread_locals
    try:
      db = _thread_locals.request.database or DEFAULT_DB_ALIAS
    except:
      db = DEFAULT_DB_ALIAS
    cmts = Comment.objects.using(db).order_by('-lastmodified').select_related('content_type', 'user')[:self.limit]
    result = []
    result.append('<div class="table-responsive"><table class="table table-condensed"><tbody>');
    for c in cmts:
      result.append('<tr><td><a href="%s%s">%s</a>&nbsp;<span class="small">%s</span><div class="small" style="float: right;">%s&nbsp;&nbsp;%s</div></br><p style="padding-left: 10px; display: inline-block;">%s</p>' % (
        _thread_locals.request.prefix, c.get_admin_url(), escape(c.object_pk),
        escape(capfirst(force_text(_(c.content_type.name))) if c.content_type else force_text(_('Unknown content'))),
        escape(c.user.username if c.user else ''),
        formats.date_format(c.lastmodified, 'SHORT_DATETIME_FORMAT'),
        escape(c.comment)
        )+'</td></tr>')
    result.append('</tbody></table></div>')
    #. Translators: Translation included with Django
    return '\n'.join(result) if result else force_text(_('None available'))

Dashboard.register(RecentCommentsWidget)
