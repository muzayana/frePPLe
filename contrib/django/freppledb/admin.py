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
from django.contrib import admin
from django.utils.importlib import import_module

# Create an admin site where all our apps will register their models
site = admin.sites.AdminSite()

# Adding the admin modules of each installed application.
for app in settings.INSTALLED_APPS:
  try:
    mod = import_module('%s.admin' % app)
  except ImportError, e:
    # Silently ignore 
    pass
