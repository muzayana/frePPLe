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
Load information from a database in frePPLe memory.

The code in this file is executed NOT by Django, but by the embedded Python
interpreter from the frePPLe engine.

It extracts the information fields from the database, and then uses the Python
API of frePPLe to bring the data into the frePPLe C++ core engine.
'''
from __future__ import print_function
from time import time
from datetime import datetime

from django.db import connections, DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.input.models import Resource

import frepple


class loadData(object):

  def __init__(self, database=None, filter=None):
    if database:
      self.database = database
    elif 'FREPPLE_DATABASE' in os.environ:
      self.database = os.environ['FREPPLE_DATABASE']
    else:
      self.database = DEFAULT_DB_ALIAS
    if filter:
      self.filter_and = "and %s " % filter
      self.filter_where = "where %s " % filter
    else:
      self.filter_and = ""
      self.filter_where = ""


  def loadParameter(self):
    print('Importing parameters...')
    try:
      self.cursor.execute("SELECT value FROM common_parameter where name='currentdate'")
      d = self.cursor.fetchone()
      frepple.settings.current = datetime.strptime(d[0], "%Y-%m-%d %H:%M:%S")
    except:
      print('Invalid or missing currentdate parameter: using system clock instead')
      frepple.settings.current = datetime.now()


  def loadLocations(self):
    print('Importing locations...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        name, description, owner_id, available_id, category, subcategory, source
      FROM location %s
      ''' % self.filter_where)
    for i,j,k,l,m,n,o in self.cursor.fetchall():
      cnt += 1
      try:
        x = frepple.location(name=i, description=j, category=m, subcategory=n, source=o)
        if k:
          x.owner = frepple.location(name=k)
        if l:
          x.available = frepple.calendar(name=l)
      except Exception as e:
        print("Error:", e)
    print('Loaded %d locations in %.2f seconds' % (cnt, time() - starttime))


  def loadCalendars(self):
    print('Importing calendars...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        name, defaultvalue, source
      FROM calendar %s
      ''' % self.filter_where)
    for i,j,k in self.cursor.fetchall():
      cnt += 1
      try:
        frepple.calendar(name=i, default=j, source=k)
      except Exception as e:
        print("Error:", e)
    print('Loaded %d calendars in %.2f seconds' % (cnt, time() - starttime))


  def loadCalendarBuckets(self):
    print('Importing calendar buckets...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
       SELECT
         calendar_id, startdate, enddate, id, priority, value,
         sunday, monday, tuesday, wednesday, thursday, friday, saturday,
         starttime, endtime
      FROM calendarbucket %s
      ORDER BY calendar_id, startdate desc
      ''' % self.filter_where)
    for i, j, k, l, m, n, o1, o2, o3, o4, o5, o6, o7, t1, t2 in self.cursor.fetchall():
      cnt += 1
      try:
        days = 0
        if o1:
          days += 1
        if o2:
          days += 2
        if o3:
          days += 4
        if o4:
          days += 8
        if o5:
          days += 16
        if o6:
          days += 32
        if o7:
          days += 64
        b = frepple.calendar(name=i).addBucket(l)
        b.value = n
        b.days = days
        if t1:
          b.starttime = t1.hour * 3600 + t1.minute * 60 + t1.second
        if t2:
          b.endtime = t2.hour * 3600 + t2.minute * 60 + t2.second + 1
        if m:
          b.priority = m
        if j:
          b.start = j
        if k:
          b.end = k
      except Exception as e:
        print("Error:", e)
    print('Loaded %d calendar buckets in %.2f seconds' % (cnt, time() - starttime))


  def loadCustomers(self):
    print('Importing customers...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        name, description, owner_id, category, subcategory, source
      FROM customer %s
      ''' % self.filter_where)
    for i,j,k,l,m,n in self.cursor.fetchall():
      cnt += 1
      try:
        x = frepple.customer(name=i, description=j, category=l, subcategory=m, source=n)
        if k:
          x.owner = frepple.customer(name=k)
      except Exception as e:
        print("Error:", e)
    print('Loaded %d customers in %.2f seconds' % (cnt, time() - starttime))


  def loadOperations(self):
    print('Importing operations...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        name, fence, pretime, posttime, sizeminimum, sizemultiple, sizemaximum,
        type, duration, duration_per, location_id, cost, search, description,
        category, subcategory, source
      FROM operation %s
      ''' % self.filter_where)
    for i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y in self.cursor.fetchall():
      cnt += 1
      try:
        if not p or p == "fixed_time":
          x = frepple.operation_fixed_time(name=i, description=v, category=w, subcategory=x, source=y)
          if q:
            x.duration = q
        elif p == "time_per":
          x = frepple.operation_time_per(name=i, description=v, category=w, subcategory=x, source=y)
          if q:
            x.duration = q
          if r:
            x.duration_per = r
        elif p == "alternate":
          x = frepple.operation_alternate(name=i, description=v, category=w, subcategory=x, source=y)
        elif p == "routing":
          x = frepple.operation_routing(name=i, description=v, category=w, subcategory=x, source=y)
        else:
          raise ValueError("Operation type '%s' not recognized" % p)
        if j:
          x.fence = j
        if k:
          x.pretime = k
        if l:
          x.posttime = l
        if m:
          x.size_minimum = m
        if n:
          x.size_multiple = n
        if o:
          x.size_maximum = o
        if s:
          x.location = frepple.location(name=s)
        if t:
          x.cost = t
        if u:
          x.search = u
      except Exception as e:
        print("Error:", e)
    print('Loaded %d operations in %.2f seconds' % (cnt, time() - starttime))
    #  SELECT operation_id, suboperation_id, priority, effective_start, effective_end, operation.type
    #  FROM suboperation, operation
    #  WHERE suboperation.operation_id = operation.name
    #    AND priority >= 0 %s
    #  ORDER BY operation_id, priority


  def loadSuboperations(self):
    print('Importing suboperations...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT operation_id, suboperation_id, priority, effective_start, effective_end,
        (select type
         from operation
         where suboperation.operation_id = operation.name) as type
      FROM suboperation
      WHERE priority >= 0 %s
      ORDER BY operation_id, priority
      ''' % self.filter_and)
    curopername = None
    for i, j, k, l, m, n in self.cursor.fetchall():
      cnt += 1
      try:
        if i != curopername:
          curopername = i
          if n == 'alternate':
            curoper = frepple.operation_alternate(name=curopername)
          else:
            curoper = frepple.operation_routing(name=curopername)
        if isinstance(curoper,frepple.operation_routing):
          curoper.addStep(frepple.operation(name=j))
        else:
          if l:
            if m:
              curoper.addAlternate(operation=frepple.operation(name=j),priority=k,effective_start=l,effective_end=m)
            else:
              curoper.addAlternate(operation=frepple.operation(name=j),priority=k,effective_start=l)
          elif m:
              curoper.addAlternate(operation=frepple.operation(name=j),priority=k,effective_end=m)
          else:
            curoper.addAlternate(operation=frepple.operation(name=j),priority=k)
      except Exception as e:
        print("Error:", e)
    print('Loaded %d suboperations in %.2f seconds' % (cnt, time() - starttime))


  def loadItems(self):
    print('Importing items...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        name, description, operation_id, owner_id,
        price, category, subcategory, source
      FROM item %s
      ''' % self.filter_where)
    for i,j,k,l,m,n,o,p in self.cursor.fetchall():
      cnt += 1
      try:
        x = frepple.item(name=i, description=j, category=n, subcategory=o, source=p)
        if k:
          x.operation = frepple.operation(name=k)
        if l:
          x.owner = frepple.item(name=l)
        if m:
          x.price = m
      except Exception as e:
        print("Error:", e)
    print('Loaded %d items in %.2f seconds' % (cnt, time() - starttime))


  def loadBuffers(self):
    print('Importing buffers...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT name, description, location_id, item_id, onhand,
        minimum, minimum_calendar_id, producing_id, type, leadtime, min_inventory,
        max_inventory, min_interval, max_interval, size_minimum,
        size_multiple, size_maximum, fence, carrying_cost,
        category, subcategory, source
      FROM buffer %s
      ''' % self.filter_where)
    for i,j,k,l,m,t,n,o,q,f1,f2,f3,f4,f5,f6,f7,f8,f9,p,r,s,t in self.cursor.fetchall():
      cnt += 1
      if q == "procure":
        b = frepple.buffer_procure(
          name=i, description=j, item=frepple.item(name=l), onhand=m,
          category=r, subcategory=s, source=t
          )
        if f1:
          b.leadtime = f1
        if f2:
          b.mininventory = f2
        if f3:
          b.maxinventory = f3
        if f6:
          b.size_minimum = f6
        if f7:
          b.size_multiple = f7
        if f8:
          b.size_maximum = f8
        if f9:
          b.fence = f9
      elif q == "infinite":
        b = frepple.buffer_infinite(
          name=i, description=j, item=frepple.item(name=l), onhand=m,
          category=r, subcategory=s, source=t
          )
      elif not q or q == "default":
        b = frepple.buffer(
          name=i, description=j, item=frepple.item(name=l), onhand=m,
          category=r, subcategory=s, source=t
          )
      else:
        raise ValueError("Buffer type '%s' not recognized" % q)
      if k:
        b.location = frepple.location(name=k)
      if t:
        b.minimum = t
      if n:
        b.minimum_calendar = frepple.calendar(name=n)
      if o:
        b.producing = frepple.operation(name=o)
      if p:
        b.carrying_cost = p
      if f4:
        b.mininterval = f4
      if f5:
        b.maxinterval = f5
    print('Loaded %d buffers in %.2f seconds' % (cnt, time() - starttime))


  def loadSetupMatrices(self):
    print('Importing setup matrix rules...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        setupmatrix_id, priority, fromsetup, tosetup, duration, cost
      FROM setuprule %s
      ORDER BY setupmatrix_id, priority DESC
      ''' % self.filter_where)
    for i,j,k,l,m,n,o,p in self.cursor.fetchall():
      cnt += 1
      try:
        r = frepple.setupmatrix(name=i,source=p).addRule(priority=j)
        if k:
          r.fromsetup = k
        if l:
          r.tosetup = l
        if m:
          r.duration = m
        if n:
          r.cost = n
      except Exception as e:
        print("Error:", e)
    print('Loaded %d setup matrix rules in %.2f seconds' % (cnt, time() - starttime))


  def loadResources(self):
    print('Importing resources...')
    cnt = 0
    starttime = time()
    Resource.rebuildHierarchy(database=self.database)
    self.cursor.execute('''
      SELECT
        name, description, maximum, maximum_calendar_id, location_id, type, cost,
        maxearly, setup, setupmatrix_id, category, subcategory, owner_id, source
      FROM %s %s
      ORDER BY lvl ASC, name
      ''' % (connections[self.cursor.db.alias].ops.quote_name('resource'), self.filter_where) )
    for i,j,t,k,l,m,n,o,p,q,r,s,u,v in self.cursor.fetchall():
      cnt += 1
      try:
        if m == "infinite":
          x = frepple.resource_infinite(name=i,description=j,category=r,subcategory=s, source=v)
        elif m == "buckets":
          x = frepple.resource_buckets(name=i,description=j,category=r,subcategory=s, source=v)
          if k:
            x.maximum_calendar = frepple.calendar(name=k)
          if o:
            x.maxearly = o
        elif not m or m == "default":
          x = frepple.resource_default(name=i,description=j,category=r,subcategory=s, source=v)
          if k:
            x.maximum_calendar = frepple.calendar(name=k)
          if o:
            x.maxearly = o
          if t:
            x.maximum = t
        else:
          raise ValueError("Resource type '%s' not recognized" % m)
        if l:
          x.location = frepple.location(name=l)
        if n:
          x.cost = n
        if p:
          x.setup = p
        if q:
          x.setupmatrix = frepple.setupmatrix(name=q)
        if u:
          x.owner = frepple.resource(name=u)
      except Exception as e:
        print("Error:", e)
    print('Loaded %d resources in %.2f seconds' % (cnt, time() - starttime))


  def loadResourceSkills(self):
    print('Importing resource skills...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        resource_id, skill_id, effective_start, effective_end, priority, source
      FROM resourceskill %s
      ORDER BY skill_id, priority, resource_id
      ''' % self.filter_where)
    for i,j,k,l,m,n in self.cursor.fetchall():
      cnt += 1
      try:
        cur = frepple.resourceskill(resource=frepple.resource(name=i), skill=frepple.skill(name=j), priority=m or 1, source=n)
        if k:
          cur.effective_start = k
        if l:
          cur.effective_end = l
      except Exception as e:
        print("Error:", e)
    print('Loaded %d resource skills in %.2f seconds' % (cnt, time() - starttime))


  def loadFlows(self):
    print('Importing flows...')
    cnt = 0
    starttime = time()
    # Note: The sorting of the flows is not really necessary, but helps to make
    # the planning progress consistent across runs and database engines.
    self.cursor.execute('''
      SELECT
        operation_id, thebuffer_id, quantity, type, effective_start,
        effective_end, name, priority, search, source
      FROM flow
      WHERE (alternate IS NULL OR alternate = '') %s
      ORDER BY operation_id, thebuffer_id
      ''' % self.filter_and)
    curbufname = None
    for i,j,k,l,m,n,o,p,q,r in self.cursor.fetchall():
      cnt += 1
      try:
        if j != curbufname:
          curbufname = j
          curbuf = frepple.buffer(name=curbufname)
        curflow = frepple.flow(operation=frepple.operation(name=i), type="flow_%s" % l, buffer=curbuf, quantity=k, source=r)
        if m:
          curflow.effective_start = m
        if n:
          curflow.effective_end = n
        if o:
          curflow.name = o
        if p:
          curflow.priority = p
        if q:
          curflow.search = q
      except Exception as e:
        print("Error:", e)
    self.cursor.execute('''
      SELECT
        operation_id, thebuffer_id, quantity, type, effective_start,
        effective_end, name, alternate, priority, search, source
      FROM flow
      WHERE (alternate IS NOT NULL AND alternate <> '') %s
      ORDER BY operation_id, thebuffer_id
      ''' % self.filter_and)
    curbufname = None
    for i,j,k,l,m,n,o,p,q,r,s in self.cursor.fetchall():
      cnt += 1
      try:
        if j != curbufname:
          curbufname = j
          curbuf = frepple.buffer(name=curbufname)
        curflow = frepple.flow(operation=frepple.operation(name=i), type=l, buffer=curbuf, quantity=k, source=s)
        if m:
          curflow.effective_start = m
        if n:
          curflow.effective_end = n
        if o:
          curflow.name = o
        if p:
          curflow.alternate = p
        if q:
          curflow.priority = q
        if r:
          curflow.search = r
      except Exception as e:
        print("Error:", e)
    print('Loaded %d flows in %.2f seconds' % (cnt, time() - starttime))


  def loadLoads(self):
    print('Importing loads...')
    cnt = 0
    starttime = time()
    # Note: The sorting of the loads is not really necessary, but helps to make
    # the planning progress consistent across runs and database engines.
    self.cursor.execute('''
      SELECT
        operation_id, resource_id, quantity, effective_start, effective_end, name,
        priority, setup, search, skill_id, source
      FROM resourceload
      WHERE (alternate IS NULL OR alternate = '') %s
      ORDER BY operation_id, resource_id
      ''' % self.filter_and)
    curresname = None
    for i,j,k,l,m,n,o,p,q,r,s in self.cursor.fetchall():
      cnt += 1
      try:
        if j != curresname:
          curresname = j
          curres = frepple.resource(name=curresname)
        curload = frepple.load(operation=frepple.operation(name=i), resource=curres, quantity=k, source=s)
        if l:
          curload.effective_start = l
        if m:
          curload.effective_end = m
        if n:
          curload.name = n
        if o:
          curload.priority = o
        if p:
          curload.setup = p
        if q:
          curload.search = q
        if r:
          curload.skill = frepple.skill(name=r)
      except Exception as e:
        print("Error:", e)
    self.cursor.execute('''
      SELECT
        operation_id, resource_id, quantity, effective_start, effective_end,
        name, alternate, priority, setup, search, skill_id, source
      FROM resourceload
      WHERE (alternate IS NOT NULL AND alternate <> '') %s
      ORDER BY operation_id, resource_id
      ''' % self.filter_and)
    curresname = None
    for i,j,k,l,m,n,o,p,q,r,s,t in self.cursor.fetchall():
      cnt += 1
      try:
        if j != curresname:
          curresname = j
          curres = frepple.resource(name=curresname)
        curload = frepple.load(operation=frepple.operation(name=i), resource=curres, quantity=k, source=t)
        if l:
          curload.effective_start = l
        if m:
          curload.effective_end = m
        if n:
          curload.name = n
        if o:
          curload.alternate = o
        if p:
          curload.priority = p
        if q:
          curload.setup = q
        if r:
          curload.search = r
        if s:
          curload.skill = frepple.skill(name=s)
      except Exception as e:
        print("Error:", e)
    print('Loaded %d loads in %.2f seconds' % (cnt, time() - starttime))


  def loadOperationPlans(self):
    print('Importing operationplans...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        operation_id, id, quantity, startdate, enddate, locked, source
      FROM operationplan
      WHERE owner_id IS NULL %s
      ORDER BY id ASC
      ''' % self.filter_and)
    for i,j,k,l,m,n,o in self.cursor.fetchall():
      cnt += 1
      frepple.operationplan(operation=frepple.operation(name=i),
        id=j, quantity=k, start=l, end=m, source=o).locked = n
    self.cursor.execute('''
      SELECT
        operation_id, id, quantity, startdate, enddate, locked, owner_id, source
      FROM operationplan
      WHERE owner_id IS NOT NULL %s
      ORDER BY id ASC
      ''' % self.filter_and)
    for i,j,k,l,m,n,o,p in self.cursor.fetchall():
      cnt += 1
      frepple.operationplan(operation=frepple.operation(name=i),
        id=j, quantity=k, start=l, end=m, owner=frepple.operationplan(id=o), source=p).locked = n
    print('Loaded %d operationplans in %.2f seconds' % (cnt, time() - starttime))


  def loadDemand(self):
    print('Importing demands...')
    cnt = 0
    starttime = time()
    self.cursor.execute('''
      SELECT
        name, due, quantity, priority, item_id,
        operation_id, customer_id, owner_id, minshipment, maxlateness,
        category, subcategory, source
      FROM demand
      WHERE (status IS NULL OR status ='open' OR status = 'quote') %s
      ''' % self.filter_and)
    for i,j,k,l,m,n,o,p,q,r,s,t,u in self.cursor.fetchall():
      cnt += 1
      try:
        x = frepple.demand( name=i, due=j, quantity=k, priority=l,
              item=frepple.item(name=m), category=s, subcategory=t, source=u)
        if n:
          x.operation = frepple.operation(name=n)
        if o:
          x.customer = frepple.customer(name=o)
        if p:
          x.owner = frepple.demand(name=p)
        if q:
          x.minshipment = q
        if r is not None:
          x.maxlateness = r
      except Exception as e:
        print("Error:", e)
    print('Loaded %d demands in %.2f seconds' % (cnt, time() - starttime))


  def run(self):
    '''
    This function is expected to be run by the python interpreter in the
    frepple application.
    It loads data from the database into the frepple memory.
    '''
    # Make sure the debug flag is not set!
    # When it is set, the django database wrapper collects a list of all sql
    # statements executed and their timings. This consumes plenty of memory
    # and cpu time.
    settings.DEBUG = False

    # Create a database connection
    self.cursor = connections[self.database].cursor()

    # Sequential load of all entities
    # Some entities could be loaded in parallel threads, but in preliminary tests
    # we haven't seen a clear performance gain. It is unclear what the limiting
    # bottleneck is: python or frepple, definitely not the database...
    self.loadParameter()
    self.loadCalendars()
    self.loadCalendarBuckets()
    self.loadLocations()
    self.loadCustomers()
    self.loadOperations()
    self.loadSuboperations()
    self.loadItems()
    self.loadBuffers()
    self.loadSetupMatrices()
    self.loadResources()
    self.loadResourceSkills()
    self.loadFlows()
    self.loadLoads()
    self.loadOperationPlans()
    self.loadDemand()

    # Close the database connection
    self.cursor.close()

    # Finalize
    print('Done')
