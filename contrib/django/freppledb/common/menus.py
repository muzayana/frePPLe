#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import operator

from django.utils.encoding import force_unicode
from django.contrib.auth.models import Permission
from django.contrib.contenttypes.models import ContentType
from django.utils.text import capfirst

import logging
logger = logging.getLogger(__name__)


class MenuItem:

  def __init__(self, name, model=None, report=None, url=None, label=None, index=None, prefix=True, window=False):
    self.name = name
    self.url = url
    self.report = report
    self.model = model
    self.label = None
    if report: self.label = report.title
    elif model: self.label = model._meta.verbose_name_plural
    if label: self.label = label
    self.index = index
    self.prefix = prefix
    self.window = window

  def __unicode__(self):
    return self.name

  def has_permission(self, user):
    if self.report:
      # The menu item is a report class
      for perm in self.report.permissions:
        if not user.has_perm(u"%s.%s" % (self.report.getAppLabel(), perm[0])):
          return False
      return True
    elif self.model:
      # The menu item is a model, belonging to an admin site
      return user.has_perm("%s.%s" % (self.model._meta.app_label, self.model._meta.get_change_permission()))
    else:
      # Other item is always available
      return True

  def can_add(self, user):
    return self.model and user.has_perm("%s.%s" % (self.model._meta.app_label, self.model._meta.get_add_permission()))


class Menu:

  def __init__(self):
    # Structure of the _groups field:
    #   [
    #     (name, label, id, [ Menuitem1, Menuitem2, ]),
    #     (name, label, id, [ Menuitem3, Menuitem3, ]),
    #   ]
    self._groups = []
    # Structure of the _cached_menu field for a certain language:
    #   [
    #     (label as unicode, [ (index, unicode label, Menuitem1), (index, unicode label, Menuitem2), ]),
    #     (label as unicode, [ (index, unicode label, Menuitem3), (index, unicode label, Menuitem4), ]),
    #   ]
    self._cached_menu = {}


  def __str__(self):
    return str(self._groups)


  def addGroup(self, name, index=None, label=None):
    # Search across existing groups
    gr = None
    for i in range(len(self._groups)):
      if self._groups[i][0] == name:
        # Update existing group
        gr = self._groups[i]
        if label: gr[1] = label
        if index: gr[2] = index
        return
    # Create new group, if it wasn't found already
    self._groups.append( (name, label or name, index, []) )


  def removeGroup(self, name):
    # Scan across groups
    for i in range(len(self._groups)):
      if self._groups[i][0] == name:
        del self._groups[i]
        return
    # No action required when the group isn't found


  def addItem(self, group, name, admin=None, report=None, url=None, label=None, index=None, prefix=True, window=False):
    for i in range(len(self._groups)):
      if self._groups[i][0] == group:
        # Found the group
        for j in range(len(self._groups[i][3])):
          if self._groups[i][3][j].name == name:
            # Update existing item
            it = self._groups[i][3][j]
            if index: it['index'] = index
            if url: it['url'] = url
            if report: it['report'] = report
            if label: it['label'] = label
            it['prefix'] = prefix
            it['window'] = window
            return
        # Create a new item
        if admin:
          # Add all models from an admin site
          for m in admin._registry:
            self._groups[i][3].append( MenuItem(
                m.__name__.lower(),
                model = m,
                url='/%s/%s/%s/' % (admin.name, m._meta.app_label, m.__name__.lower()),
                index=index
                ) )
        else:
          # Add a single item
          self._groups[i][3].append( MenuItem(name, report=report, url=url, label=label, index=index, prefix=prefix, window=window) )
        return
    # Couldn't locate the group
    raise Exception("Menu group %s not found" % group)


  def removeItem(self, group, name):
    for i in range(len(self._groups)):
      if self._groups[i][0] == group:
        # Found the group
        for j in range(len(self._groups[i][3])):
          if self._groups[i][3][j][0] == name:
            # Update existing item
            del self._groups[i][3][j]
            return
    # Couldn't locate the group or the item
    raise Exception("Menu item %s not found in group %s " % (name, group))


  def getMenu(self, language):
    # Lookup in the cache
    m = self._cached_menu.get(language, None)
    if m: return m
    # Build new menu for this language
    # Sort the groups by 1) id and 2) order of append.
    self._groups.sort(key=operator.itemgetter(2))
    # Build the list of items in each group
    m = []
    for i in self._groups:
      items = []
      for j in i[3]:
        items.append( (j.index, capfirst(force_unicode(j.label)), j) )
      # Sort by 1) id and 2) label. Note that the order can be different for each language!
      items.sort(key=operator.itemgetter(0,1))
      m.append( ( force_unicode(i[1]), items ))
    # Put the new result in the cache and return
    self._cached_menu[language] = m
    return m


  def createReportPermissions(self, app):
    # Find all registered menu items of the app.
    content_type = None
    for i in self._groups:
      for j in i[3]:
        if j.report and j.report.__module__.startswith(app):
          # Loop over all permissions of the class
          for k in j.report.permissions:
            if content_type == None:
              # Create a dummy contenttype in the app
              content_type = ContentType.objects.get_or_create(name="reports", model="", app_label=app.split('.')[-1])[0]
            # Create the permission object
            # TODO: cover the case where the permission refers to a permission of a model in the same app.
            # TODO: cover the case where app X wants to refer to a permission defined in app Y.
            p = Permission.objects.get_or_create(codename=k[0], content_type=content_type)[0]
            p.name = k[1]
            p.save()

