#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from importlib import import_module

from django.conf import settings
from django.contrib.admin.sites import AdminSite, AlreadyRegistered


class freppleAdminSite(AdminSite):
  def register(self, model_or_iterable, admin_class=None, force=False, **options):
    try:
      super(freppleAdminSite, self).register(model_or_iterable, admin_class, **options)
    except AlreadyRegistered:
      # Ignore exception if the model is already registered. It indicates that
      # another app has already registered it.
      if force:
        # Unregister the previous one and register ourselves
        self.unregister(model_or_iterable)
        super(freppleAdminSite, self).register(model_or_iterable, admin_class, **options)


# Create two admin sites where all our apps will register their models
data_site = freppleAdminSite(name='data')

# Adding the admin modules of each installed application.
for app in settings.INSTALLED_APPS:
  try:
    mod = import_module('%s.admin' % app)
  except ImportError as e:
    # Silently ignore if its the admin module which isn't found
    if str(e) not in ("No module named %s.admin" % app, "No module named '%s.admin'" % app):
      raise e
