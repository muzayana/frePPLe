============================================
Deployment on Windows with Apache web server
============================================

The windows installer installs a Python based web server. For environments
with a few concurrent users or for a trial installation this will suffice,
and it is the recommended configuration.

If the number of concurrent users is higher or when more complex configurations
are required on the network (such as HTTPS encryption, additional web pages
to be served from the same web server, access from the internet as well as
from the intranet, using external authentication instead, configure compression
and caching, etc…), you can deploy frePPLe with an Apache web server.

Note that scalability of the web application on Linux is significantly better
on Linux than on Windows. Environments with more than 20 concurrent users
should prefer Linux.

To configure frePPLe on Windows with an Apache web server, the following steps
are required:

#. Assure you **have administrator rights** on the machine.

#. **Install frePPLe** using the Windows installer, following the steps from the
   previous page.

#. **Collect all static files**

   The static files will be served by the Apache web server, and we need to
   collect all of these files in a separate folder.
   Open a command prompt in the bin folder of your frePPLe installation and run:
   ::

     frepplectl collectstatic

#. **Install PostgreSQL database.**

   The recommended version is 9.3, 64-bit. Information on tuning the database
   configuration is easily found on Google.

#. **Install Python 3.3 or higher**

   The download URL is http://www.python.org/download/releases/2.7/
   Use the 32-bit version, even on 64-bit platforms.

#. **Install Psycopg2**

   The Python database driver for PostgreSQLcan be downloaded from
   http://stickpeople.com/projects/python/win-psycopg/

   Pick the executable that matches the Python version. The executable built for PostgreSQL 9.2
   also works with PostgreSQL 9.3.

#. **Install PyWin32**

   The Python Windows extensions can be downloaded from
   http://sourceforge.net/projects/pywin32/

   Select the 32-bit installer for Python 2.7.

#. **Install the Python database drivers, Django and other python modules**

   Since frePPle requires some patches to the standard Django package, so the source 
   from our cloned and patched version of django will be downloaded and installed.

   In the root of your python install you will find a "requirements.txt" file containing a list like:
   ::

      CherryPy >= 3.2.2
      et-xmlfile >= 1.0.0
      html5lib >= 0.999
      jdcal >= 1.0
      openpyxl >= 2.3.0-b2
      https://github.com/frePPLe/django/tarball/frepple_3.0
      djangorestframework >= 3.3.1

   To install the requirements just issue a pip3 (or pip depending on your distribution) command:
   ::

      sudo pip install -r requirements.txt

#. **Install mod_wsgi**

   Mod_wsgi is python WSGI adapter module for Apache.

   The download URL is http://www.lfd.uci.edu/~gohlke/pythonlibs/#mod_wsgi
   Choose the 32-bit for Python 2.7 and Apache 2.4, and copy the file to the Apache
   modules folder.

#. **Configure the Apache web server**

   Add a line to the file conf/httpd.conf:

   ::

       Include conf/extra/httpd-frepple.conf

   Create a file conf/extra/httpd-frepple.conf using the example we provide in
   the file contrib/debian/httpd/conf.
   Adjust the paths, review carefully, and tweak to your preferences and needs!

#. **Test the setup**

   Open your browser and verify the frePPLe pages display correctly.
