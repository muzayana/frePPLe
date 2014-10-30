#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf import settings
from django.utils.importlib import import_module
from django.utils.translation import ugettext_lazy as _

from freppledb.common.menus import Menu

# Create the navigation menu.
# This is the one and only menu object in the application.
menu = Menu()

# Add our default topics.
menu.addGroup("input", label=_("Input"), index=100)
menu.addGroup("reports", label=_("Reports"), index=200)
menu.addGroup("admin", label=_("Admin"), index=300)
menu.addGroup("user", label=_("User"), index=400)
menu.addGroup("help", label="?", index=500)

# Adding the menu modules of each installed application.
# Note that the menus of the apps are processed in reverse order.
# This is required to allow the first apps to override the entries
# of the later ones.
for app in reversed(settings.INSTALLED_APPS):
  try:
    mod = import_module('%s.menu' % app)
  except ImportError as e:
    # Silently ignore if it's the menu module which isn't found
    if str(e) not in ("No module named %s.menu" % app, "No module named '%s.menu'" % app):
      raise e
