#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
import http.client
import os
import _thread
import time
from xml.dom import minidom
from cherrypy.process import servers

from django.core import management
from django.test import TestCase
from django.db import close_old_connections
