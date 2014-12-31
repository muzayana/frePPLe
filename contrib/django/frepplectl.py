#!/usr/bin/env python3

#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

r'''
This command is the wrapper for all administrative actions on frePPLe.
'''

import os
import sys

if __name__ == "__main__":
  os.environ.setdefault("DJANGO_SETTINGS_MODULE", "freppledb.settings")
  from django.core.management import execute_from_command_line
  execute_from_command_line(sys.argv)
