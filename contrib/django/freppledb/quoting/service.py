#
# Copyright (C) 2009-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

# file : $URL: file:///C:/Users/Johan/Dropbox/SVNrepository/frepple/addon/contrib/django/freppledb_extra/quoting/service.py $
# revision : $LastChangedRevision: 432 $  $LastChangedBy: Johan $
# date : $LastChangedDate: 2012-10-31 16:50:04 +0100 (Wed, 31 Oct 2012) $

import cherrypy
import os
from django.conf import settings
from django.template.loader import render_to_string

import frepple


def error_page(status, message, traceback, version):
  return '''
    <!DOCTYPE html>
    <html lang="en-us">
    <head>
    <title>frePPLe %(version)s: error %(status)s</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <meta name="robots" content="NONE,NOARCHIVE" />
    </head>
    <body>
    <h2>FrePPLe error %(status)s</h2>
    <p>frePPLe %(version)s:<br/><span id="message">%(message)s</span></p>
    <pre style="color:red">%(traceback)s</pre>
    </body>
    </html>
    ''' % {'status':status, 'message':message, 'traceback':traceback, 'version':frepple.version}


def Server(address=None, port=None):
  import socket

  # Pick up the address
  if address == None:
    try: address = socket.gethostbyname(socket.gethostname())
    except: address = '127.0.0.1'

  # Pick up the port number
  try: port = int(port or 8080)
  except: port = 8080

  # Validate the address and port number
  try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind( (address, port) )
    s.close()
  except socket.error as e:
    raise Exception("Invalid address '%s' and/or port '%s': %s" % (address, port, e))

  cherrypy.config.update({
    'server.environment': 'development',
    'server.socket_host': address,
    'server.socket_port': port,
    'log.access_file': "access.log",
    'log.error_file': "error.log",
    'log.screen': True,
    #'tools.gzip.on': True,   
    'error_page.default': error_page, 
    'log.level': 'info',
    'server.threadPool': 10,
    'engine.autoreload_on': False, 
    'tools.response_headers.on': True,
    'tools.response_headers.headers': [('Server', 'frepple/%s' % frepple.version)] 
    })
  config = {
    '/': {},
    '/frepple.xsd': {
      'tools.staticfile.on': True,
      'tools.staticfile.filename': os.path.join(settings.FREPPLE_HOME, 'frepple.xsd'),
      'tools.staticfile.content_types':  {'xsd': 'application/xml'}
      },
    '/frepple_core.xsd': {
      'tools.staticfile.on': True,
      'tools.staticfile.filename': os.path.join(settings.FREPPLE_HOME, 'frepple_core.xsd'),
      'tools.staticfile.content_types': {'xsd': 'application/xml'}
      },
    '/favicon.ico': {
      'tools.staticfile.on': True,
      'tools.staticfile.filename': os.path.join(settings.FREPPLE_APP, 'static', 'favicon.ico')
      },            
    '/static': {
       'tools.staticdir.on': True,
       'tools.staticdir.root': settings.FREPPLE_APP,
       'tools.staticdir.dir': 'static'
       }
    }
  print 'Order quoting web service starting on http://%s:%s/' % (address, port)
  cherrypy.quickstart(Interface(), "", config=config)
  print 'Order quoting web service stopped'


class collectdemands:
  def __init__(self):
    self.demands = []
    
  def __call__(self, o):
      if isinstance(o, frepple.demand):
        if o not in self.demands:
          self.demands.append(o) 
               
          
class Interface:
  
  top = [
    '<?xml version="1.0" encoding="UTF-8" ?>\n',
    '<plan xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">\n',
    ]

  bottom = [
    '</plan>\n',
    ]
         
  def __init__(self):
    # Create a solver
    self.solver = frepple.solver_mrp(name="quote", constraints=15, plantype=1, loglevel=2)
    
  # A generic way to expose XML data.
  # Use this decorator function to decorate a generator function.
  def simpleXMLdata(gen):
    @cherrypy.expose
    def decorator(self, *__args, **__kw):
      if cherrypy.request.method == 'GET':
        # Get requests
        cherrypy.response.headers['content-type'] = 'application/xml'
        res = []
        for i in self.top: res.append(i)
        for i in gen(self, *__args): 
          if i: res.append(i)
        for i in self.bottom: res.append(i)
        return "".join(res)
      elif cherrypy.request.method  == 'POST' or cherrypy.request.method == 'PUT':
        # Post and put requests
        cherrypy.response.headers['content-type'] = 'application/xml'
        res = []
        for i in gen(self, *__args): 
          if i: res.append(i)
        return "".join(res)
      else:
        # Other HTTP verbs (such as head, delete, ...) are not supported
        raise cherrypy.HTTPError(404,"Not found")
    return decorator

  # Top-level interface handling URLs of the format:
  #    POST /
  #    PUT /
  #    GET /
  @cherrypy.expose
  def index(self, xmldata=None):
    request = cherrypy.request
    if cherrypy.request.method == 'GET':
      # Main index page
      return render_to_string("quoting/index.html", {
        'request': request,
        'version': frepple.version
        })
    else:
      # Posting XML data file
      cherrypy.response.headers['content-type'] = 'text/html'
      error = []
      if xmldata:
        try:
          if not isinstance(xmldata,basestring):
            xmldata = xmldata.file.read()
          frepple.readXMLdata(xmldata)
        except Exception as e:
          error.append(str(e))
      if len(error) > 0:
        raise cherrypy.HTTPError(500,'\n'.join(error))
      return "OK\n"

  #  Top-level interface for getting all in-memory objects
  #    GET /main/
  @simpleXMLdata
  def main(self, name=None):
    if cherrypy.request.method == 'GET':      
      # GET request
      res = []
      res.append('<locations>\n')
      for f in frepple.locations(): res.append(f.toXML() or '')
      res.append('</locations>\n')
      res.append('<customers>\n')
      for f in frepple.customers(): res.append(f.toXML() or '')
      res.append('</customers>\n')
      res.append('<calendars>\n')
      for f in frepple.calendars(): res.append(f.toXML() or '')
      res.append('</calendars>\n')
      res.append('<operations>\n')
      for f in frepple.operations(): res.append(f.toXML() or '')
      res.append('</operations>\n')
      res.append('<items>\n')
      for f in frepple.items(): res.append(f.toXML() or '')
      res.append('</items>\n')
      res.append('<buffers>\n')
      for f in frepple.buffers(): res.append(f.toXML() or '')
      res.append('</buffers>\n')
      res.append('<demands>\n')
      for f in frepple.demands(): res.append(f.toXML() or '')
      res.append('</demands>\n')
      res.append('<resources>\n')
      for f in frepple.resources(): res.append(f.toXML() or '')
      res.append('</resources>\n')
      res.append('<operationplans>\n')
      for f in frepple.operationplans(): res.append(f.toXML() or '')
      res.append('</operationplans>\n')
      res.append('<problems>\n')
      for f in frepple.problems(): res.append(f.toXML() or '')
      res.append('</problems>\n')
      return "".join(res)
    else:
      raise cherrypy.HTTPError(403, "Only GET requests to this URL are allowed")
    
  # Interface for locations handling URLs of the format:
  #    GET /location/
  #    GET /location/<name>/
  #    POST /location/<name>/?<parameter>=<value>
  @simpleXMLdata
  def location(self, name=None):
    if cherrypy.request.method == 'GET':
      # GET information
      yield '<locations>\n'
      if name:
        # Return a single location
        try:
          yield frepple.location(name=name,action="C").toXML()
        except:
          # Location not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all locations
        for f in frepple.locations(): yield f.toXML()
      yield '</locations>\n'
    else:
      # Create or update a location
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.location(name=name)
      except:
        # Location not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for buffers handling URLs of the format:
  #    GET /buffer/
  #    GET /buffer/<name>/
  #    POST /buffer/<name>/?<parameter>=<value>
  @simpleXMLdata
  def buffer(self, name=None):
    if cherrypy.request.method == 'GET':
      try: mode = str(cherrypy.request.params.get('plan','S'))
      except: mode = 'S'
      yield '<buffers>\n'
      if name:
        # Return a single buffer
        try:
          yield frepple.buffer(name=name,action="C").toXML(mode)
        except:
          # Buffer not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all buffers
        for f in frepple.buffers(): yield f.toXML(mode)
      yield '</buffers>\n'
    else:
      # Create or update a buffer
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.buffer(name=name)
      except:
        # Buffer not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for items handling URLs of the format:
  #    GET /item/
  #    GET /item/<name>/
  #    POST /item/<name>/?<parameter>=<value>
  @simpleXMLdata
  def item(self, name=None):
    if cherrypy.request.method == 'GET':
      yield '<items>\n'
      if name:
        # Return a single item
        try:
          yield frepple.item(name=name,action="C").toXML()
        except:
          # Item not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all items
        for f in frepple.items(): yield f.toXML()
      yield '</items>\n'
    else:
      # Create or update an item
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.item(name=name)
      except:
        # Item not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for operations handling URLs of the format:
  #    GET /operation/
  #    GET /operation/<name>/
  #    POST /operation/<name>/?<parameter>=<value>
  @simpleXMLdata
  def operation(self, name=None):
    if cherrypy.request.method == 'GET':
      try: mode = str(cherrypy.request.params.get('plan','S'))
      except: mode = 'S'
      yield '<operations>\n'
      if name:
        # Return a single operation
        try:
          yield frepple.operation(name=name,action="C").toXML(mode)
        except:
          # Operation not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all operations
        for f in frepple.operations(): 
          t = f.toXML(mode)
          if t: yield t
      yield '</operations>\n'
    else:
      # Create or update an operation
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.operation(name=name)
      except:
        # Operation not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for demands handling URLs of the format:
  #    GET /demand/
  #    GET /demand/<name>/
  #    POST /demand/<name>/?<parameter>=<value>
  @simpleXMLdata
  def demand(self, name=None):
    if cherrypy.request.method == 'GET':
      try: mode = str(cherrypy.request.params.get('plan','S'))
      except: mode = 'S'
      yield '<demands>\n'
      if name:
        # Return a single demand
        try:
          yield frepple.demand(name=name,action="C").toXML(mode)
        except:
          # Demand not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all locations
        for f in frepple.demands(): yield f.toXML(mode)
      yield '</demands>\n'
    else:
      # Create or update a demand
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.demand(name=name)
      except:
        # Demand not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for resources handling URLs of the format:
  #    GET /resource/
  #    GET /resource/<name>/
  #    POST /resource/<name>/?<parameter>=<value>
  @simpleXMLdata
  def resource(self, name=None):
    if cherrypy.request.method == 'GET':
      try: mode = str(cherrypy.request.params.get('plan','S'))
      except: mode = 'S'
      yield '<resources>\n'
      if name:
        # Return a single resource
        try:
          yield frepple.resource(name=name,action="C").toXML(mode)
        except:
          # Resource not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all resources
        for f in frepple.resources(): yield f.toXML(mode)
      yield '</resources>\n'
    else:
      # Create or update a resource
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.resource(name=name)
      except:
        # Resource not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for calendars handling URLs of the format:
  #    GET /calendar/
  #    GET /calendar/<name>/
  #    POST /calendar/<name>/?<parameter>=<value>
  @simpleXMLdata
  def calendar(self, name=None):
    if cherrypy.request.method == 'GET':
      yield '<calendars>\n'
      if name:
        # Return a single calendar
        try:
          yield frepple.calendar(name=name,action="C").toXML()
        except:
          # Calendar not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all calendars
        for f in frepple.calendars(): yield f.toXML()
      yield '</calendars>\n'
    else:
      # Create or update a calendar
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.calendar(name=name)
      except:
        # Calendar not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"


  # Interface for calendars handling URLs of the format:
  #    GET /setupmatrix/
  #    GET /setupmatrix/<name>/
  #    POST /setupmatrix/<name>/?<parameter>=<value>
  @simpleXMLdata
  def setupmatrix(self, name=None):
    if cherrypy.request.method == 'GET':
      yield '<setupmatrices>\n'
      if name:
        # Return a single setupmatrix
        try:
          yield frepple.setupmatrix(name=name,action="C").toXML()
        except:
          # Calendar not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all setupmatrix
        for f in frepple.setupmatrices(): yield f.toXML()
      yield '</setupmatrices>\n'
    else:
      # Create or update a calendar
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.setupmatrix(name=name)
      except:
        # Calendar not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"
      
  # Interface for customers handling URLs of the format:
  #    GET /customer/
  #    GET /customer/<name>/
  #    POST /customer/<name>/?<parameter>=<value>
  @simpleXMLdata
  def customer(self, name=None):
    if cherrypy.request.method == 'GET':
      yield '<customers>\n'
      if name:
        # Return a single customer
        try:
          yield frepple.customer(name=name,action="C").toXML()
        except:
          # Customer not found
          raise cherrypy.HTTPError(404,"Entity not found")
      else:
        # Return all customers
        for f in frepple.customers(): yield f.toXML()
      yield '</customers>\n'
    else:
      # Create or update a customer
      if name == None: raise cherrypy.HTTPError(404,"Entity not found")
      try:
        loc = frepple.customer(name=name)
      except:
        # Customer not found
        raise cherrypy.HTTPError(404,"Entity not found")
      ok = True
      for i in cherrypy.request.params:
        try:
          setattr(loc, i, cherrypy.request.params[i])
        except Exception as e:
          yield "Error: %s\n" % e
          ok = False
      if ok: yield "OK\n"

  # Interface for flows handling URLs of the format:
  #    GET /flow/
  @simpleXMLdata
  def flow(self):
    if cherrypy.request.method == 'GET':
      yield '<flows>\n'
      for b in frepple.buffers():
        for f in b.flows: yield f.toXML()
      yield '</flows>\n'
    else:
      raise cherrypy.HTTPError(404,"Not supported")

  # Interface for loads handling URLs of the format:
  #    GET /load/
  @simpleXMLdata
  def load(self):
    if cherrypy.request.method == 'GET':
      yield '<loads>\n'
      for b in frepple.resources():
        for f in b.loads: yield f.toXML()
      yield '</loads>\n'
    else:
      raise cherrypy.HTTPError(404,"Not supported")

  # Interface for problems handling URLs of the format:
  #    GET /problem/
  @simpleXMLdata
  def problem(self):
    if cherrypy.request.method == 'GET':
      yield '<problems>\n'
      for f in frepple.problems(): yield f.toXML()
      yield '</problems>\n'
    else:
      raise cherrypy.HTTPError(404,"Not supported")

  # Top-level interface handling URLs of the format:
  #    POST /quote
  #    PUT /quote
  @cherrypy.expose
  def quote(self, xmldata=None):
    # Verify request type
    request = cherrypy.request
    if request.method not in ('POST','PUT'):
      raise cherrypy.HTTPError(404,"Not supported")
    if not xmldata: 
      raise cherrypy.HTTPError(404,"No data")
    
    # Read all demands
    callback = collectdemands()   
    cherrypy.response.headers['content-type'] = 'application/xml'
    try:
      if not isinstance(xmldata,basestring):
        xmldata = xmldata.file.read()
      frepple.readXMLdata(xmldata,1,0, callback)
    except Exception as e:
      raise cherrypy.HTTPError(500,str(e))
    if not callback.demands:
      raise cherrypy.HTTPError(404,"No data")
    
    # Process the demands
    res = []
    for i in self.top: res.append(i)
    res.append('<demands>')
    errors = []
    for d in callback.demands:
      try:
        self.solver.solve(d)
        self.solver.commit()
      except Exception as e:
        errors.append(str(e))      
      res.append(d.toXML('P'))     
    res.append('</demands>')
    for i in self.bottom: res.append(i)     
    return "".join(res)