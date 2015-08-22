#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from datetime import datetime
import logging

from django.conf import settings
from django.contrib.admin.utils import quote
from django.contrib.auth.models import AbstractUser
from django.contrib.contenttypes.fields import GenericForeignKey
from django.contrib.contenttypes.models import ContentType
from django.core.urlresolvers import NoReverseMatch, reverse
from django.db import models, DEFAULT_DB_ALIAS, connections, transaction
from django.utils import timezone
from django.utils.translation import ugettext_lazy as _

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
      for i in children.get(me, []):
        # Recursive execution of this function for each child of this node
        right = tagChildren(i, right, level + 1)

      # After processing the children of this node now know its left and right values
      updates.append( (left, right, level, me) )

      # Remove from node list (to mark as processed)
      del nodes[me]

      # Return the right value of this node + 1
      return right + 1

    # Load all nodes in memory
    for i in cls.objects.using(database).values('name', 'owner'):
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
        cnt = tagChildren(i, cnt, 0)

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
          cnt = tagChildren(i, cnt, 0)

    # Write all results to the database
    with transaction.atomic(using=database):
      connections[database].cursor().executemany(
        'update %s set lft=%%s, rght=%%s, lvl=%%s where name = %%s' % connections[database].ops.quote_name(cls._meta.db_table),
        updates
        )


class MultiDBManager(models.Manager):
  def get_queryset(self):
    from freppledb.common.middleware import _thread_locals
    req = getattr(_thread_locals, 'request', None)
    if req:
      return super(MultiDBManager, self).get_queryset().using(getattr(req, 'database', DEFAULT_DB_ALIAS))
    else:
      return super(MultiDBManager, self).get_queryset().using(DEFAULT_DB_ALIAS)


class AuditModel(models.Model):
  '''
  This is an abstract base model.
  It implements the capability to maintain:
    - the date of the last modification of the record.
    - a string intended to describe the source system that supplied the record
  '''
  # Database fields
  source = models.CharField(_('source'), db_index=True, max_length=settings.CATEGORYSIZE, null=True, blank=True)
  lastmodified = models.DateTimeField(_('last modified'), editable=False, db_index=True, default=timezone.now)

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

  def __str__(self):
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


class Scenario(models.Model):
  scenarioStatus = (
    ('free', _('Free')),
    ('in use', _('In use')),
    ('busy', _('Busy')),
  )

  # Database fields
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True)
  description = models.CharField(_('description'), max_length=settings.DESCRIPTIONSIZE, null=True, blank=True)
  status = models.CharField(
    _('status'), max_length=10,
    null=False, blank=False, choices=scenarioStatus
    )
  lastrefresh = models.DateTimeField(_('last refreshed'), null=True, editable=False)

  def __str__(self):
    return self.name

  @staticmethod
  def syncWithSettings():
    try:
      # Bring the scenario table in sync with settings.databases
      with transaction.atomic(savepoint=False):
        dbs = [ i for i, j in settings.DATABASES.items() if j['NAME'] ]
        for sc in Scenario.objects.all():
          if sc.name not in dbs:
            sc.delete()
        scs = [sc.name for sc in Scenario.objects.all()]
        for db in dbs:
          if db not in scs:
            if db == DEFAULT_DB_ALIAS:
              Scenario(name=db, status="In use", description='Production database').save()
            else:
              Scenario(name=db, status="Free").save()
    except Exception as e:
      logger.error("Error synchronizing the scenario table with the settings: %s" % e)

  class Meta:
    db_table = "common_scenario"
    permissions = (
        ("copy_scenario", "Can copy a scenario"),
        ("release_scenario", "Can release a scenario"),
       )
    verbose_name_plural = _('scenarios')
    verbose_name = _('scenario')
    ordering = ['name']


class User(AbstractUser):
  languageList = tuple( [ ('auto', _('Detect automatically')), ] + list(settings.LANGUAGES) )
  language = models.CharField(
    _('language'), max_length=10, choices=languageList,
    default='auto'
    )
  theme = models.CharField(
    _('theme'), max_length=20, default=settings.DEFAULT_THEME,
    choices=settings.THEMES
    )
  pagesize = models.PositiveIntegerField(_('page size'), default=settings.DEFAULT_PAGESIZE)
  horizonbuckets = models.CharField(max_length=settings.NAMESIZE, blank=True, null=True)
  horizonstart = models.DateTimeField(blank=True, null=True)
  horizonend = models.DateTimeField(blank=True, null=True)
  horizontype = models.BooleanField(blank=True, default=True)
  horizonlength = models.IntegerField(blank=True, default=6, null=True)
  horizonunit = models.CharField(
    blank=True, max_length=5, default='month', null=True,
    choices=(("day", "day"), ("week", "week"), ("month", "month"))
    )
  lastmodified = models.DateTimeField(
    _('last modified'), auto_now=True, null=True, blank=True,
    editable=False, db_index=True
    )


  def save(self, force_insert=False, force_update=False, using=DEFAULT_DB_ALIAS, update_fields=None):
    '''
    Every change to a user model is saved to all active scenarios.

    The is_superuser and is_active fields can be different in each scenario.
    All other fields are expected to be identical in each database.

    Because of the logic in this method creating users directly in the
    database tables is NOT a good idea!
    '''
    # We want to automatically give access to the django admin to all users
    self.is_staff = True

    scenarios = [ i['name'] for i in Scenario.objects.filter(status='In use').values('name') ]

    # The same id of a new user MUST be identical in all databases.
    # We manipulate the sequences, and correct if required.
    newuser = False
    tmp_is_active = self.is_active
    tmp_is_superuser = self.is_superuser
    if not self.id:
      newuser = True
      self.id = 0
      cur_seq = {}
      for db in scenarios:
        cursor = connections[db].cursor()
        cursor.execute("select nextval('common_user_id_seq')")
        cur_seq[db] = cursor.fetchone()[0]
        if cur_seq[db] > self.id:
          self.id = cur_seq[db]
      for db in scenarios:
        if cur_seq[db] != self.id:
          cursor = connections[db].cursor()
          cursor.execute("select setval('common_user_id_seq', %s)", [self.id - 1])
      self.is_active = False
      self.is_superuser = False

    # Save only specific fields which we want to have identical across
    # all scenario databases.
    if not update_fields:
      update_fields2=[
        'username', 'password', 'last_login', 'first_name', 'last_name',
        'email', 'date_joined', 'language', 'theme', 'pagesize',
        'horizonbuckets', 'horizonstart', 'horizonend', 'horizonunit',
        'lastmodified', 'is_staff'
        ]
    else:
      # Important is NOT to save the is_active and is_superuser fields.
      update_fields2 = update_fields[:]  # Copy!
      if 'is_active' in update_fields2:
        update_fields2.remove('is_active')
      if 'is_superuser' in update_fields:
        update_fields2.remove('is_superuser')
    if update_fields2 or newuser:
      for db in scenarios:
        with transaction.atomic(using=db, savepoint=False):
          if db == using:
            continue
          super(User, self).save(
            force_insert=force_insert,
            force_update=force_update,
            using=db,
            update_fields=update_fields2 if not newuser else None
            )

    # Continue with the regular save, as if nothing happened.
    self.is_active = tmp_is_active
    self.is_superuser = tmp_is_superuser
    return super(User, self).save(
      force_insert=force_insert,
      force_update=force_update,
      using=using,
      update_fields=update_fields
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

  def getPreference(self, prop, default=None):
    try:
      return self.preferences.get(property=prop).value
    except ValueError:
      logger.error("Invalid preference '%s' of user '%s'" % (prop, self.username))
      return default
    except:
      return default

  def setPreference(self, prop, val):
    if val is None:
      # Delete a preference
      try:
        self.preferences.get(property=prop).delete()
      except UserPreference.DoesNotExist:
        # No such preferences exists now
        pass
    else:
      # Create or update a preference
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
    unique_together = (('user', 'property'),)
    verbose_name = 'preference'
    verbose_name_plural = 'preferences'


class Comment(models.Model):
  id = models.AutoField(_('identifier'), primary_key=True)
  content_type = models.ForeignKey(
    ContentType, verbose_name=_('content type'),
    related_name="content_type_set_for_%(class)s"
    )
  object_pk = models.TextField(_('object ID'))
  content_object = GenericForeignKey(ct_field="content_type", fk_field="object_pk")
  comment = models.TextField(_('comment'), max_length=settings.COMMENT_MAX_LENGTH)
  user = models.ForeignKey(User, verbose_name=_('user'), blank=True, null=True, editable=False)
  lastmodified = models.DateTimeField(_('last modified'), default=timezone.now, editable=False)

  class Meta:
      db_table = "common_comment"
      ordering = ('id',)
      verbose_name = _('comment')
      verbose_name_plural = _('comments')

  def __str__(self):
      return "%s: %s..." % (self.object_pk, self.comment[:50])

  def get_admin_url(self):
    """
    Returns the admin URL to edit the object represented by this comment.
    """
    if self.content_type and self.object_pk:
      url_name = 'data:%s_%s_change' % (self.content_type.app_label, self.content_type.model)
      try:
        return reverse(url_name, args=(quote(self.object_pk),))
      except NoReverseMatch:
        try:
          url_name = 'admin:%s_%s_change' % (self.content_type.app_label, self.content_type.model)
          return reverse(url_name, args=(quote(self.object_pk),))
        except NoReverseMatch:
          pass
    return None


class Bucket(AuditModel):
  # Create some dummy string for common bucket names to force them to be translated.
  extra_strings = ( _('day'), _('week'), _('month'), _('quarter'), _('year'), _('telescope') )

  # Database fields
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, primary_key=True)
  description = models.CharField(_('description'), max_length=settings.DESCRIPTIONSIZE, null=True, blank=True)

  def __str__(self):
    return str(self.name)

  class Meta:
    verbose_name = _('bucket')
    verbose_name_plural = _('buckets')
    db_table = 'common_bucket'


class BucketDetail(AuditModel):
  # Database fields
  id = models.AutoField(_('identifier'), primary_key=True)
  bucket = models.ForeignKey(Bucket, verbose_name=_('bucket'), db_index=True)
  name = models.CharField(_('name'), max_length=settings.NAMESIZE, db_index=True)
  startdate = models.DateTimeField(_('start date'))
  enddate = models.DateTimeField(_('end date'))

  def __str__(self):
    return "%s %s" % (self.bucket.name or "", self.startdate)

  class Meta:
    verbose_name = _('bucket date')
    verbose_name_plural = _('bucket dates')
    db_table = 'common_bucketdetail'
    unique_together = (('bucket', 'startdate'),)
    ordering = ['bucket', 'startdate']
