#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

"""
Configuration for frePPLe django WSGI web application.
This is used by the different WSGI deployment options:
  - mod_wsgi on apache web server
  - django development server 'frepplectl.py runserver'
  - cherrypy server 'frepplectl.py frepple_runserver
"""

import os, sys
from django.core.wsgi import get_wsgi_application

# Assure frePPLe is found in the Python path.
sys.path.append(os.path.join(os.path.dirname(__file__),'..'))

os.environ['LC_ALL'] = 'en_US.UTF-8'
os.environ['DJANGO_SETTINGS_MODULE'] = 'freppledb.settings'

application = get_wsgi_application()
