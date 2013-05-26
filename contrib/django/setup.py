#!/usr/bin/env python

#
# Copyright (C) 2009-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from distutils.core import setup
import sys, os

def fullsplit(path, result=None):
    """
    Split a pathname into components (the opposite of os.path.join) in a
    platform-neutral way.
    """
    if result is None: result = []
    head, tail = os.path.split(path)
    if head == '': return [tail] + result
    if head == path: return result
    return fullsplit(head, [tail] + result)

# Tell distutils not to put the data_files in platform-specific installation
# locations. See here for an explanation:
# http://groups.google.com/group/comp.lang.python/browse_thread/thread/35ec7b2fed36eaec/2105ee4d9e8042cb
from distutils.command.install import INSTALL_SCHEMES
for scheme in INSTALL_SCHEMES.values():
  scheme['data'] = scheme['purelib']

# Compile the list of packages and data files
packages = []
data_files = []
root_dir = os.path.dirname(__file__)
if root_dir != '': os.chdir(root_dir) 
for dirpath, dirnames, filenames in os.walk('freppledb'):
  # Ignore dirnames that start with '.'
  for i, dirname in enumerate(dirnames):
    if dirname.startswith('.'): del dirnames[i]
  if '__init__.py' in filenames:
    packages.append('.'.join(fullsplit(dirpath)))
  elif filenames:
    data_files.append([dirpath, [os.path.join(dirpath, f) for f in filenames]])

# Small hack for working with bdist_wininst.
# See http://mail.python.org/pipermail/distutils-sig/2004-August/004134.html
if len(sys.argv) > 1 and sys.argv[1] == 'bdist_wininst':
    for file_info in data_files:
        file_info[0] = '\\PURELIB\\%s' % file_info[0]
    
setup(name = 'freppledb',
      version = __import__('freppledb').VERSION,
      author = "frepple.com",
      author_email = "info@frepple.com",
      url = "http://frepple.com",
      scripts = ['manage.py'],
      packages = packages,
      data_files = data_files,
      options = { "install" : {'optimize': 2}},
      classifiers = [
        'License :: Other/Proprietary License',
        'Intended Audience :: Manufacturing',
        'Framework :: Django',        
        ],
      description = "FREE Production PLanning",
      long_description = '''FrePPLe stands for "Free Production Planning Library".
It is a framework for modeling and solving production planning problems,
targeted primarily at discrete manufacturing industries.
'''
      )
