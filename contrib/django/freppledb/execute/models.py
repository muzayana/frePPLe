#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#


from django.db import models, transaction, DEFAULT_DB_ALIAS
from django.utils.translation import ugettext_lazy as _
from django.conf import settings


class log(models.Model):
  # Database fields
  lastmodified = models.DateTimeField(_('last modified'), auto_now=True, editable=False, db_index=True)
  category = models.CharField(_('category'), max_length=10, db_index=True)
  message = models.TextField(_('message'), max_length=200, null=True)
  theuser = models.CharField(_('user'), max_length=30, null=True)

  def __unicode__(self):
    return self.lastmodified + ' - ' + self.category + ' - ' + self.user

  class Meta:
      permissions = (
          ("run_frepple", "Can run frepple"),
          ("run_db","Can run database procedures"),
         )
      verbose_name_plural = _('log entries')
      verbose_name = _('log entry')


scenarioStatus = (
  ('free',_('Free')),
  ('in use',_('In use')),
  ('busy',_('Busy')),
)


class Scenario(models.Model):
  # Database fields
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True)
  description = models.CharField(_('description'), max_length=settings.DESCRIPTIONSIZE, null=True, blank=True)
  status = models.CharField(_('status'), max_length=10,
    null=False, blank=False, choices=scenarioStatus
    )
  lastrefresh = models.DateTimeField(_('last refreshed'), null=True, editable=False)

  def __unicode__(self):
    return self.name

  @staticmethod
  @transaction.commit_manually
  def syncWithSettings():
    try:
      # Bring the scenario table in sync with settings.databases
      dbs = [ i for i,j in settings.DATABASES.items() if j['NAME'] ]
      for sc in Scenario.objects.all():
        if sc.name not in dbs:
          sc.delete()
      scs = [sc.name for sc in Scenario.objects.all()]
      for db in dbs:
        if db not in scs:
          if db == DEFAULT_DB_ALIAS:
            Scenario(name=db, status=u"In use", description='Production database').save()
          else:
            Scenario(name=db, status=u"Free").save()
    except Exception as e:
      print "Error synchronizing the scenario table with the settings:", e
      transaction.rollback()
    finally:
      transaction.commit()

  class Meta:
    permissions = (
        ("copy_scenario", "Can copy a scenario"),
        ("release_scenario", "Can release a scenario"),
       )
    verbose_name_plural = _('scenarios')
    verbose_name = _('scenario')
    ordering = ['name']
