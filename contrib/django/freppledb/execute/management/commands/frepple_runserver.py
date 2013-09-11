#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from __future__ import print_function
import sys, socket
from threading import Thread
from optparse import make_option
from cherrypy.wsgiserver import CherryPyWSGIServer

from django.conf import settings
from django.core.management.base import BaseCommand
from django.core.handlers.wsgi import WSGIHandler
from django.contrib.staticfiles.handlers import StaticFilesHandler

from freppledb import VERSION


class Command(BaseCommand):

  help = '''
    Runs a multithreaded web server for frePPLe.

    Because of the multithreading it is is more performant than the default
    development web server bundled with django.
    However, it should still only be used for configurations with a single user,
    and is not a full alternative to using Apache + mod_wsgi.
  '''

  option_list = BaseCommand.option_list + (
    make_option("--port", dest="port", type="int",
                  help="Port number of the server."),
    make_option("--address", dest="address", type="string",
                  help="IP address for the server to listen."),
    )

  requires_model_validation = False

  def get_version(self):
    return VERSION

  def handle(self, **options):
    # Determine the port number
    if 'port' in options:
      port = int(options['port'] or settings.PORT)
    else:
      port = settings.PORT

    # Determine the IP-address to listen on:
    # - either as command line argument
    # - either 0.0.0.0 by default, which means all active IPv4 interfaces
    address = 'address' in options and options['address'] or '0.0.0.0'

    # Validate the address and port number
    try:
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.bind( (address, port) )
      s.close()
    except socket.error as e:
      raise Exception("Invalid address '%s' and/or port '%s': %s" % (address, port, e))

    # Print a header message
    hostname = socket.getfqdn()
    print('Starting frePPLe %s web server\n' % VERSION)
    print('To access the server, point your browser to either of the following URLS:')
    if address == '0.0.0.0':
      print('    http://%s:%s/' % (hostname, port))
      for ip in socket.gethostbyname_ex(socket.gethostname())[2]:
        print('    http://%s:%s/' % (ip, port))
    else:
      print('    http://%s:%s/' % (address, port))
    print('\nThree users are created by default: "admin", "frepple" and "guest" (the default password is equal to the user name)\n')
    print('Quit the server with CTRL-C.\n')

    # Start a separate thread that will check for updates
    # We don't wait for it to finish
    CheckUpdates().start()

    # Run the WSGI server
    server = CherryPyWSGIServer((address, port),
      StaticFilesHandler(WSGIHandler())
      )
    # Want SSL support? Just set these attributes apparently, but I haven't tested or verified this
    #  server.ssl_certificate = <filename>
    #  server.ssl_private_key = <filename>
    try:
      server.start()
    except KeyboardInterrupt:
      server.stop()


class CheckUpdates(Thread):
  def run(self):
    try:
      import urllib
      import urllib2
      import re
      values = {
        'platform' : sys.platform,
        'executable' : sys.executable,
        'version' : VERSION,
        }
      request = urllib2.Request('http://www.frepple.com/usage.php?' + urllib.urlencode(values))
      response = urllib2.urlopen(request).read()
      match = re.search("<release>(.*)</release>", response)
      release = match.group(1)
      if release > VERSION:
        print("A new frePPLe release %s is available. Your current release is %s." % (release, VERSION))
    except:
      # Don't worry if something went wrong.
      pass
