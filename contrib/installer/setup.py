#!/usr/bin/env python

#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import sys, os, os.path
sys.path.append(os.path.join(os.path.split(__file__)[0],'..','django'))
import py2exe, django, freppledb
from freppledb import VERSION
from distutils.core import setup

# Add default command lines
if len(sys.argv) == 1:
    sys.argv.append("py2exe")

# Figure out where the django and frepple directories are
djangodirectory = django.__path__[0]
freppledirectory = freppledb.__path__[0]

# Define what is to be included and excluded
packages = [# Required for django standalone deployment
            'django', 'email', 'cherrypy.wsgiserver', 'csv', 
            'htmlentitydefs','HTMLParser','markupbase',
            # Added for MySQL
            'MySQLdb', 'MySQLdb.constants', 'MySQLdb.converters',
            # Added for PostgreSQL
            'psycopg2', 'psycopg2.extensions',
            # Added for oracle
            'cx_Oracle',
            # Required for the python initialization
            'site',
            # Required for graphing
            'pygraphviz',
            # Added to package a more complete python library with frePPLe
            'ftplib', 'poplib', 'imaplib', 'telnetlib', 'xmlrpclib',  
            'gzip', 'bz2','zipfile', 'tarfile', 'SimpleXMLRPCServer', 
            # Added for unicode and internationalization
            'encodings',
           ]
includes = []
excludes = ['pydoc', 'Tkinter', 'tcl', 'Tkconstants', 'freppledb']
ignores = [# Not using docutils
           'docutils', 'docutils.core', 'docutils.nodes', 'docutils.parsers.rst.roles',
           # Not using Microsoft ADO
           'adodbapi',
           # Not using psycopg (using psycopg2 instead)
           'psycopg',
           # Not using pysqlite2 (using pysqlite3 instead)
           'pysqlite2',
           # Not using mod_python
           'mod_python', 'mod_python.util',
           # Not using memcache
           'cmemcache', 'memcache',
           # Not using markdown tags of django
           'markdown', 'textile',
           # Not using WSCGI
           'flup', 'flup.server.fcgi', 'flup.server.fcgi_fork',
           # Not using ImageFields
           'PIL', 'ImageFile',
           # Not needing special datetime handling
           'mx', 'mx.TextTools',
           # Not using yaml serialization
           'yaml',
           # Not storing templates in python eggs
           'pkg_resources', 'resource',
           # Not using the python interactive interpreter
           'IPython',
           # Not sure where django references these...
           'crypt',
           # Not using SSL
           'OpenSSL',
           # Not needed to include frePPLe's own python interface
           'frepple',
           ]

# Collect all static files to be included in the distribution.
# This includes our custom python code as well.
from distutils.command.install import INSTALL_SCHEMES
for scheme in INSTALL_SCHEMES.values(): scheme['data'] = scheme['purelib']
data_files = []
for srcdir, targetdir in [
   (os.path.join(djangodirectory,'contrib','admin','templates'), 'templates'),
   (os.path.join(djangodirectory,'contrib','admin','static'), 'static'),
   (os.path.join(djangodirectory,'conf','locale'), os.path.join('locale','django')),
   (os.path.join(djangodirectory,'contrib','auth','locale'), os.path.join('locale','auth')),
   (os.path.join(djangodirectory,'contrib','contenttypes','locale'), os.path.join('locale','contenttypes')),
   (os.path.join(djangodirectory,'contrib','sessions','locale'), os.path.join('locale','sessions')),
   (os.path.join(djangodirectory,'contrib','admin','locale'), os.path.join('locale','admin')),
   (os.path.join(djangodirectory,'contrib','messages','locale'), os.path.join('locale','messages')),
   (freppledirectory, os.path.join('custom','freppledb')),
   ]:
   root_path_length = len(srcdir) + 1
   for dirpath, dirnames, filenames in os.walk(os.path.join(srcdir)):
     # Ignore dirnames that start with '.'
     for i, dirname in enumerate(dirnames):
       if dirname.startswith('.'): del dirnames[i]
     # Append data files for this subdirectory
     data_files.append([
       os.path.join(targetdir, dirpath[root_path_length:]),
       [os.path.join(dirpath, f) for f in filenames if not f.endswith(".pyc") and not f.endswith(".pyo")]
       ])

# Run the py2exe program
setup(
    # Options
    options = {"py2exe": {
          # create a compressed zip archive
          "compressed": 1,
          # optimize the bytecode
          "optimize": 2,
          # Next option is commented out: Gives a cleaner install, but doesn't work for sqlite
          # bundle python modules in the zip file as well.
          #"bundle_files": 2,
          # content of the packaged python
          "packages": packages,
          "excludes": excludes,
          "includes": includes,
          "ignores": ignores,
          # ignore this file that is useful only in archaic windows versions
          "dll_excludes": ['w9xpopen.exe'],
          }},
    data_files = data_files,
    # Attributes
    version = VERSION,
    description = "frePPLe web application",
    name = "frePPLe",
    author = "www.frepple.com",
    url = "http://www.frepple.com",
    # Target to build a Windows service
    service = [{
       "modules":["freppleservice"], 
       "icon_resources": [(1, "frepple.ico")], 
       "cmdline_style": 'pywin32',
       }],
    # Target to build a console application
    console = [{
       "script": "frepplectl.py",
       "icon_resources": [(1, "frepple.ico")],
       }],
    # Name of the zip file with the bytecode of the Python library.
    # This zip file with the name mentioned below is automatically included
    # in the Python search path (while the default output file "library.zip"
    # isn't)
    zipfile = "python%d%d.zip" % (sys.version_info[0], sys.version_info[1])
    )
