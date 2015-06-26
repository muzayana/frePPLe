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
This module holds a number of functions that are useful to make SQL statements
portable across different databases.

Django also includes a set of wrapper functions around incompatible
database functionality. A seperate one was required to add functions and
enhance others.
  - sql_datediff:
    Returns the time diffence between 2 datetime values, expressed in days.
  - sql_overlap:
    Returns the overlap between 2 date ranges, expressed in days.
  - sql_min:
    Returns the maximum of 2 numbers.
  - sql_max:
    Returns the minimum of 2 numbers.
  - python_date:
    A datetime database field is represented differently by the different
    database connectors.
    Oracle, PostgreSQL and mySQL return a python datetime object.
    SQLite returns a string.
    This method does what one might intuitively expect: a python date object
    is always returned.

The code assumes that all database engines are identical in the frePPLe
application.
'''

from datetime import datetime

from django.conf import settings
from django.db import DEFAULT_DB_ALIAS


# Functions for POSTGRESQL
if settings.DATABASES[DEFAULT_DB_ALIAS]['ENGINE'] == 'django.db.backends.postgresql_psycopg2':

  def sql_true():
    return 'true'

  def sql_datediff(d1, d2):
    return '(extract(epoch from (cast(%s as timestamp) - cast(%s as timestamp))) / 86400)' % (d1, d2)

  def sql_overlap(s1, e1, s2, e2):
    return 'greatest(0,extract(epoch from ' \
      '(least(cast(%s as timestamp),cast(%s as timestamp)) ' \
      ' - greatest(cast(%s as timestamp),cast(%s as timestamp)))) / 86400)' % (e1, e2, s1, s2)

  def sql_overlap3(s1, e1, s2, e2, s3, e3):
    return 'greatest(0,extract(epoch from ' \
      '(least(cast(%s as timestamp),cast(%s as timestamp),cast(%s as timestamp)) ' \
      ' - greatest(cast(%s as timestamp),cast(%s as timestamp),cast(%s as timestamp)))) / 86400)' % (e1, e2, e3, s1, s2, s3)

  def sql_max(d1, d2):
    return "greatest(%s,%s)" % (d1, d2)

  def sql_min(d1, d2):
    return "least(%s,%s)" % (d1, d2)

  def python_date(d):
    return d.date()

  def string_agg():
    return 'string_agg'


else:
  raise NameError('The %s database is not support by frePPLe' % settings.DATABASES[DEFAULT_DB_ALIAS]['ENGINE'])
