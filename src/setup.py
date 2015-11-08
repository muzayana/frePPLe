#
# Copyright (C) 20015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

# This setup script is used to build a Python extension for the frePPLe library.
# The script is intended to be called ONLY from the makefile, and not as standalone command.

from distutils.core import setup, Extension
import os

mod = Extension(
  'frepple',
  sources=['pythonextension.cpp'],
  include_dirs=["../include"],
  define_macros=[("HAVE_LOCALTIME_R","1")],
  libraries=['frepple', 'xerces-c'],
  library_dirs=[os.environ['LIB_DIR']]
  )

setup (
  name = 'frepple',
  version = os.environ['VERSION'],
  author = "frepple.com",
  author_email = "info@frepple.com",
  url = "http://frepple.com",
  ext_modules = [mod],
  license="Other/Proprietary License",
  classifiers = [
    'License :: Other/Proprietary License',
    'Intended Audience :: Manufacturing',
    ],
  description = 'Bindings for the frePPLe production planning application',
  long_description = '''FrePPLe stands for "Free Production Planning Library".
It is a framework for modeling and solving production planning problems,
targeted primarily at discrete manufacturing industries.
'''
  )
