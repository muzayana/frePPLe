#
# Copyright (C) 2015 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf import settings
from django.db import models
from django.utils.translation import ugettext_lazy as _

from freppledb.common.models import User


class Chat(models.Model):
  '''
  This model stores the history of all chat messages.
  '''
  id = models.AutoField(_('identifier'), primary_key=True)
  message = models.TextField(_('message'), max_length=settings.COMMENT_MAX_LENGTH)
  user = models.ForeignKey(User, verbose_name=_('user'), blank=False, null=False, editable=False, related_name='chat')
  lastmodified = models.DateTimeField(_('last modified'), auto_now=True, null=True, editable=False, db_index=True)

  class Meta:
    db_table = "planningboard_chat"
    ordering = ('id',)
    verbose_name = _('chat history')
    verbose_name_plural = _('chat history')

  def __str__(self):
    return "%s: %s: %s" % (self.lastmodified.strftime('%Y-%m-%d %H:%M:%S'), self.user.username, self.message)
