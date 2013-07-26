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
from stat import S_ISDIR, ST_MODE

# Environment settings (which are used in the Django settings file and need
# to be updated BEFORE importing the settings)
os.environ.setdefault('FREPPLE_HOME', os.path.split(sys.path[0])[0])
os.environ.setdefault('DJANGO_SETTINGS_MODULE', "freppledb.settings")
os.environ.setdefault('FREPPLE_APP', os.path.join(os.path.split(sys.path[0])[0],'custom'))

# Sys.path contains the zip file with all packages. We need to put the
# application directory into the path as well.
sys.path += [ os.environ['FREPPLE_APP'] ]

# Import django
from django.core.management import execute_from_command_line, call_command
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS

# Create the database if it doesn't exist yet
noDatabaseSchema = False
if settings.DATABASES[DEFAULT_DB_ALIAS]['ENGINE'] == 'django.db.backends.sqlite3':
  # Verify if the sqlite database file exists
  if not os.path.isfile(settings.DATABASES[DEFAULT_DB_ALIAS]['NAME']):
    noDatabaseSchema = True
elif settings.DATABASES[DEFAULT_DB_ALIAS]['ENGINE'] not in ['django.db.backends.postgresql_psycopg2', 'django.db.backends.mysql', 'django.db.backends.oracle']:
    print 'Aborting: Unknown database engine %s' % settings.DATABASES[DEFAULT_DB_ALIAS]['ENGINE']
    raw_input("Hit any key to continue...")
    sys.exit(1)
else:
  # PostgreSQL, Oracle or MySQL database:
  # Try connecting and check for a table called 'parameter'.
  from django.db import connection, transaction
  try: cursor = connection.cursor()
  except Exception as e:
    print "Aborting: Can't connect to the database"
    print "   %s" % e
    raw_input("Hit any key to continue...")
    sys.exit(1)
  try: cursor.execute("SELECT 1 FROM common_parameter")
  except: noDatabaseSchema = True
  transaction.commit_unless_managed()

if noDatabaseSchema and len(sys.argv)>1 and sys.argv[1]!='syncdb':
  print "\nDatabase schema has not been initialized yet."
  confirm = raw_input("Do you want to do that now? (yes/no): ")
  while confirm not in ('yes', 'no'):
    confirm = raw_input('Please enter either "yes" or "no": ')
  if confirm == 'yes':
    # Create the database
    print "\nCreating database scheme"
    call_command('syncdb', verbosity=1)

# Execute the command
execute_from_command_line(sys.argv)
