#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#


r'''
Django URL mapping file.
'''

from django.conf.urls import patterns, include
from django.conf import settings
from django.views.generic.base import RedirectView
from django.utils.importlib import import_module

import freppledb.admin

urlpatterns = patterns('',
    # Root url redirects to the admin index page
    (r'^$', RedirectView.as_view(url='/admin/')),
    
    # Handle browser icon and robots.txt
    (r'favicon\.ico$', RedirectView.as_view(url='/static/favicon.ico')),    
    (r'robots\.txt$', RedirectView.as_view(url='/static/robots.txt')),    
)

# Custom handler for page-not-found errors. It does a redirect to the main page.
handler404 = 'freppledb.common.views.handler404'

# Adding urls for each installed application.
for app in settings.INSTALLED_APPS:
  try:
    mod = import_module('%s.urls' % app)
    if hasattr(mod, 'urlpatterns'):
      if getattr(mod, 'autodiscover', False):
        urlpatterns += mod.urlpatterns
  except ImportError:
    # Silently ignore 
    pass
    
# Admin pages, and the Javascript i18n library.
# It needs to be added as the last item since the applications can
# hide/override some admin urls.
urlpatterns += patterns('',
    (r'^admin/jsi18n/$', 'django.views.i18n.javascript_catalog', {'packages': ('django.conf','freppledb'),}),
    (r'^admin/', include(freppledb.admin.site.urls)),
)
