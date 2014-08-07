#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from datetime import datetime
import logging

from django.db import models, DEFAULT_DB_ALIAS, connections, transaction
from django.contrib.contenttypes import generic
from django.contrib.contenttypes.models import ContentType
from django.utils.translation import ugettext_lazy as _
from django.conf import settings
from django.contrib.auth.models import AbstractUser

from freppledb.common.fields import JSONField

logger = logging.getLogger(__name__)


class HierarchyModel(models.Model):
  lft = models.PositiveIntegerField(db_index=True, editable=False, null=True, blank=True)
  rght = models.PositiveIntegerField(null=True, editable=False, blank=True)
  lvl = models.PositiveIntegerField(null=True, editable=False, blank=True)
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True,
                          help_text=_('Unique identifier'))
  owner = models.ForeignKey('self', verbose_name=_('owner'), null=True, blank=True,
                            related_name='xchildren', help_text=_('Hierarchical parent'))

  def save(self, *args, **kwargs):
    # Trigger recalculation of the hieracrhy
    self.lft = None
    self.rght = None
    self.lvl = None

    # Call the real save() method
    super(HierarchyModel, self).save(*args, **kwargs)

  class Meta:
    abstract = True

  @classmethod
  def rebuildHierarchy(cls, database=DEFAULT_DB_ALIAS):

    # Verify whether we need to rebuild or not.
    # We search for the first record whose lft field is null.
    if len(cls.objects.using(database).filter(lft__isnull=True)[:1]) == 0:
      return

    nodes = {}
    children = {}
    updates = []

    def tagChildren(me, left, level):
      right = left + 1
      # Get all children of this node
      for i in children.get(me,[]):
        # Recursive execution of this function for each child of this node
        right = tagChildren(i, right, level + 1)

      # After processing the children of this node now know its left and right values
      updates.append( (left, right, level, me) )

      # Remove from node list (to mark as processed)
      del nodes[me]

      # Return the right value of this node + 1
      return right + 1

    # Load all nodes in memory
    for i in cls.objects.using(database).values('name','owner'):
      if i['name'] == i['owner']:
        logging.error("Data error: '%s' points to itself as owner" % i['name'])
        nodes[i['name']] = None
      else:
        nodes[i['name']] = i['owner']
        if i['owner']:
          if not i['owner'] in children:
            children[i['owner']] = set()
          children[i['owner']].add(i['name'])
    keys = sorted(nodes.items())

    # Loop over nodes without parent
    cnt = 1
    for i, j in keys:
      if j is None:
        cnt = tagChildren(i,cnt,0)

    if nodes:
      # If the nodes dictionary isn't empty, it is an indication of an
      # invalid hierarchy.
      # There are loops in your hierarchy, ie parent-chains not ending
      # at a top-level node without parent.
      bad = nodes.copy()
      updated = True
      while updated:
        updated = False
        for i in bad.keys():
          ok = True
          for j, k in bad.items():
            if k == i:
              ok = False
              break
          if ok:
            # If none of the bad keys points to me as a parent, I am unguilty
            del bad[i]
            updated = True
      logging.error("Data error: Hierarchy loops among %s" % sorted(bad.keys()))
      for i, j in sorted(bad.items()):
        nodes[i] = None

      # Continue loop over nodes without parent
      keys = sorted(nodes.items())
      for i, j in keys:
        if j is None:
          cnt = tagChildren(i,cnt,0)

    # Write all results to the database
    with transaction.atomic(using=database):
      connections[database].cursor().executemany(
        'update %s set lft=%%s, rght=%%s, lvl=%%s where name = %%s' % connections[database].ops.quote_name(cls._meta.db_table),
        updates
        )


class MultiDBManager(models.Manager):
  def get_query_set(self):
    from freppledb.common.middleware import _thread_locals
    req = getattr(_thread_locals, 'request', None)
    if req:
      return super(MultiDBManager, self).get_query_set().using(getattr(req, 'database', DEFAULT_DB_ALIAS))
    else:
      return super(MultiDBManager, self).get_query_set().using(DEFAULT_DB_ALIAS)


class AuditModel(models.Model):
  '''
  This is an abstract base model.
  It implements the capability to maintain:
    - the date of the last modification of the record.
    - a string intended to describe the source system that supplied the record
  '''
  # Database fields
  source = models.CharField(_('source'), db_index=True, max_length=settings.CATEGORYSIZE, null=True, blank=True)
  lastmodified = models.DateTimeField(_('last modified'), editable=False, db_index=True, default=datetime.now())

  objects = MultiDBManager()  # The default manager.

  def save(self, *args, **kwargs):
    # Update the field with every change
    self.lastmodified = datetime.now()

    # Call the real save() method
    super(AuditModel, self).save(*args, **kwargs)

  class Meta:
    abstract = True


class Parameter(AuditModel):
  # Database fields
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True)
  value = models.CharField(_('value'), max_length=settings.NAMESIZE, null=True, blank=True)
  description = models.CharField(_('description'), max_length=settings.DESCRIPTIONSIZE, null=True, blank=True)

  def __unicode__(self):
    return self.name

  class Meta(AuditModel.Meta):
    db_table = 'common_parameter'
    verbose_name = _('parameter')
    verbose_name_plural = _('parameters')

  @staticmethod
  def getValue(key, database=DEFAULT_DB_ALIAS, default=None):
    try:
      return Parameter.objects.using(database).get(pk=key).value
    except:
      return default


class User(AbstractUser):
  languageList = tuple( [ ('auto',_('Detect automatically')), ] + list(settings.LANGUAGES) )
  language = models.CharField(_('language'), max_length=10, choices=languageList,
                              default='auto')
  theme = models.CharField(_('theme'), max_length=20, default=settings.DEFAULT_THEME,
                           choices=settings.THEMES)
  pagesize = models.PositiveIntegerField(_('page size'), default=settings.DEFAULT_PAGESIZE)
  horizonbuckets = models.CharField(max_length=settings.NAMESIZE, blank=True, null=True)
  horizonstart = models.DateTimeField(blank=True, null=True)
  horizonend = models.DateTimeField(blank=True, null=True)
  horizontype = models.BooleanField(blank=True, default=True)
  horizonlength = models.IntegerField(blank=True, default=6, null=True)
  horizonunit = models.CharField(
                  blank=True, max_length=5, default='month', null=True,
                  choices=(("day","day"),("week","week"),("month","month"))
                  )
  lastmodified = models.DateTimeField(
                   _('last modified'), auto_now=True, null=True, blank=True,
                   editable=False, db_index=True
                   )


  def joined_age(self):
    '''
    Returns the number of days since the user joined
    '''
    if self.date_joined.year == 2000:
      # This is the user join date from the demo database.
      # We'll consider that a new user.
      self.date_joined = self.last_login
      self.save()
    return (datetime.now() - self.date_joined).total_seconds() / 86400


  class Meta:
    db_table = "common_user"
    verbose_name = _('user')
    verbose_name_plural = _('users')

  def getPreference(self, prop, default = None):
    try:
      return self.preferences.get(property=prop).value
    except ValueError:
      logger.error("Invalid preference '%s' of user '%s'" % (prop, self.username))
      return default
    except:
      return default

  def setPreference(self, prop, val):
    pref = self.preferences.get_or_create(property=prop)[0]
    pref.value = val
    # Always saved in the main database, to have the same preferences for all scenarios
    pref.save(update_fields=['value'])


class UserPreference(models.Model):
  id = models.AutoField(_('identifier'), primary_key=True)
  user = models.ForeignKey(User, verbose_name=_('user'), blank=False, null=False, editable=False, related_name='preferences')
  property = models.CharField(max_length=settings.NAMESIZE, blank=False, null=False)
  value = JSONField(max_length=1000, blank=False, null=False)

  class Meta:
    db_table = "common_preference"
    unique_together = (('user','property'),)
    verbose_name = 'preference'
    verbose_name_plural = 'preferences'


class Comment(models.Model):
  id = models.AutoField(_('identifier'), primary_key=True)
  content_type = models.ForeignKey(
    ContentType, verbose_name=_('content type'),
    related_name="content_type_set_for_%(class)s"
    )
  object_pk = models.TextField(_('object ID'))
  content_object = generic.GenericForeignKey(ct_field="content_type", fk_field="object_pk")
  comment = models.TextField(_('comment'), max_length=settings.COMMENT_MAX_LENGTH)
  user = models.ForeignKey(User, verbose_name=_('user'), blank=True, null=True, editable=False)
  lastmodified = models.DateTimeField(_('last modified'), default=datetime.now(), editable=False)

  class Meta:
      db_table = "common_comment"
      ordering = ('id',)
      verbose_name = _('comment')
      verbose_name_plural = _('comments')

  def __unicode__(self):
      return "%s: %s..." % (self.object_pk, self.comment[:50])


class Bucket(AuditModel):
  # Create some dummy string for common bucket names to force them to be translated.
  extra_strings = ( _('day'), _('week'), _('month'), _('quarter'), _('year'), _('telescope') )

  # Database fields
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True)
  description = models.CharField(_('description'), max_length=settings.DESCRIPTIONSIZE, null=True, blank=True)

  def __unicode__(self):
    return str(self.name)

  class Meta:
    verbose_name = _('bucket')
    verbose_name_plural = _('buckets')
    db_table = 'common_bucket'


class BucketDetail(AuditModel):
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True)
  bucket = models.ForeignKey(Bucket, verbose_name=_('bucket'), db_index=True)
  name = models.CharField(_('name'), max_length=settings.NAMESIZE)
  startdate = models.DateTimeField(_('start date'))
  enddate = models.DateTimeField(_('end date'))

  def __unicode__(self):
    return u"%s %s" % (self.bucket.name or "", self.startdate)

  class Meta:
    verbose_name = _('bucket date')
    verbose_name_plural = _('bucket dates')
    db_table = 'common_bucketdetail'
    unique_together = (('bucket', 'startdate'),)
    ordering = ['bucket','startdate']
