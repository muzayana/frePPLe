#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from django.db import models
from django.utils.translation import ugettext_lazy as _

from freppledb.common.models import User

import logging
logger = logging.getLogger(__name__)


class Task(models.Model):
  '''
  Expected status values are:
    - 'Waiting'
    - 'Done'
    - 'Failed'
    - 'Canceled'
    - 'DD%', where DD represents the percentage completed
  Other values are okay, but the above have translations.
  '''
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True, editable=False)
  #. Translators: Translation included with Django
  name = models.CharField(_('name'), max_length=20, db_index=True, editable=False)
  submitted = models.DateTimeField(_('submitted'), editable=False)
  started = models.DateTimeField(_('started'), blank=True, null=True, editable=False)
  finished = models.DateTimeField(_('submitted'), blank=True, null=True, editable=False)
  arguments = models.TextField(_('arguments'), max_length=200, null=True, editable=False)
  status = models.CharField(_('status'), max_length=20, editable=False)
  message = models.TextField(_('message'), max_length=200, null=True, editable=False)
  #. Translators: Translation included with Django
  user = models.ForeignKey(User, verbose_name=_('user'), blank=True, null=True, editable=False)

  def __str__(self):
    return "%s - %s - %s" % (self.id, self.name, self.status)

  class Meta:
    db_table = "execute_log"
    verbose_name_plural = _('tasks')
    verbose_name = _('task')

  @staticmethod
  def submitTask():
    # Add record to the database
    # Check if a worker is present. If not launch one.
    return 1
