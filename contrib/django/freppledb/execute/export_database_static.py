#
# Copyright (C) 2011-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

r'''
Exports frePPLe information into a database.

The code in this file is executed NOT by Django, but by the embedded Python
interpreter from the frePPLe engine.

The code iterates over all objects in the C++ core engine, and creates
database records with the information. The Django database wrappers are used
to keep the code portable between different databases.
'''
import datetime
from time import time
from threading import Thread
import os
import inspect
import traceback

from django.db import connections, transaction, DEFAULT_DB_ALIAS
from django.conf import settings

import frepple


class exportStaticModel(object):

  def __init__(self, database=None, source=None):
    if database:
      self.database = database
    elif 'FREPPLE_DATABASE' in os.environ:
      self.database = os.environ['FREPPLE_DATABASE']
    else:
      self.database = DEFAULT_DB_ALIAS
    self.source = source

  def exportLocations(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting locations...")
      starttime = time()
      cursor.execute("SELECT name FROM location")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into location \
        (name,description,available_id,category,subcategory,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s,%s)",
        [
          (
            i.name, i.description, i.available and i.available.name or None,
            i.category, i.subcategory, i.source, self.timestamp
          )
          for i in frepple.locations()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update location \
         set description=%s, available_id=%s, category=%s, subcategory=%s, source=%s, lastmodified=%s \
         where name=%s",
        [
          (
            i.description, i.available and i.available.name or None,
            i.category, i.subcategory, i.source, self.timestamp, i.name
          )
          for i in frepple.locations()
          if i.name in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update location set owner_id=%s where name=%s",
        [
          (i.owner.name, i.name)
          for i in frepple.locations()
          if i.owner and (not self.source or self.source == i.source)
        ])
      print('Exported locations in %.2f seconds' % (time() - starttime))


  def exportCalendars(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting calendars...")
      starttime = time()
      cursor.execute("SELECT name FROM calendar")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into calendar \
        (name,defaultvalue,source,lastmodified) \
        values(%s,%s,%s,%s)",
        [
          (
            i.name, round(i.default, settings.DECIMAL_PLACES), i.source,
            self.timestamp
          )
          for i in frepple.calendars()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update calendar \
         set defaultvalue=%s, source=%s, lastmodified=%s \
         where name=%s",
        [
          (
            round(i.default, settings.DECIMAL_PLACES), i.source, self.timestamp,
            i.name
          )
          for i in frepple.calendars()
          if i.name in primary_keys and (not self.source or self.source == i.source)
        ])
      print('Exported calendars in %.2f seconds' % (time() - starttime))


  def exportCalendarBuckets(self, cursor):

    def buckets():
      cursor.execute("SELECT max(id) FROM calendarbucket")
      cnt = cursor.fetchone()[0] or 1
      for c in frepple.calendars():
        if self.source and self.source != c.source:
          continue
        for i in c.buckets:
          cnt += 1
          yield i, cnt

    def int_to_time(i):
      hour = i // 3600
      i -= (hour * 3600)
      minute = i // 60
      i -= (minute * 60)
      second = i
      if hour >= 24:
        hour -= 24
      return "%s:%s:%s" % (hour, minute, second)

    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting calendar buckets...")
      starttime = time()
      if self.source:
        cursor.execute("delete from calendarbucket where source = %s", [self.source])
      else:
        cursor.execute("delete from calendarbucket")

      cursor.executemany(
        '''insert into calendarbucket
        (calendar_id,startdate,enddate,id,priority,value,
         sunday,monday,tuesday,wednesday,thursday,friday,saturday,
         starttime,endtime,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i[0].calendar.name, str(i[0].start), str(i[0].end), i[1], i[0].priority,
            round(i[0].value, settings.DECIMAL_PLACES),
            (i[0].days & 1) and True or False, (i[0].days & 2) and True or False,
            (i[0].days & 4) and True or False, (i[0].days & 8) and True or False,
            (i[0].days & 16) and True or False, (i[0].days & 32) and True or False,
            (i[0].days & 64) and True or False,
            int_to_time(i[0].starttime), int_to_time(i[0].endtime - 1),
            i[0].calendar.source, self.timestamp
          )
          for i in buckets()
        ])
      print('Exported calendar buckets in %.2f seconds' % (time() - starttime))


  def exportOperations(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting operations...")
      starttime = time()
      cursor.execute("SELECT name FROM operation")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into operation
        (name,fence,posttime,sizeminimum,sizemultiple,sizemaximum,type,duration,
         duration_per,location_id,cost,search,description,category,subcategory,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i.name, i.fence, i.posttime, round(i.size_minimum, settings.DECIMAL_PLACES),
            round(i.size_multiple, settings.DECIMAL_PLACES),
            i.size_maximum < 9999999999999 and round(i.size_maximum, settings.DECIMAL_PLACES) or None,
            i.__class__.__name__[10:],
            isinstance(i, (frepple.operation_fixed_time, frepple.operation_time_per)) and i.duration or None,
            isinstance(i, frepple.operation_time_per) and i.duration_per or None,
            i.location and i.location.name or None, round(i.cost, settings.DECIMAL_PLACES),
            isinstance(i, frepple.operation_alternate) and i.search or None,
            i.description, i.category, i.subcategory, i.source, self.timestamp
          )
          for i in frepple.operations()
          if i.name not in primary_keys and not i.hidden and i.name != 'setup operation' and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update operation
         set fence=%s, posttime=%s, sizeminimum=%s, sizemultiple=%s,
         sizemaximum=%s, type=%s, duration=%s, duration_per=%s, location_id=%s, cost=%s, search=%s,
         description=%s, category=%s, subcategory=%s, source=%s, lastmodified=%s
         where name=%s''',
        [
          (
            i.fence, i.posttime, round(i.size_minimum, settings.DECIMAL_PLACES),
            round(i.size_multiple, settings.DECIMAL_PLACES),
            i.size_maximum < 9999999999999 and round(i.size_maximum, settings.DECIMAL_PLACES) or None,
            i.__class__.__name__[10:],
            isinstance(i, (frepple.operation_fixed_time, frepple.operation_time_per)) and i.duration or None,
            isinstance(i, frepple.operation_time_per) and i.duration_per or None,
            i.location and i.location.name or None, round(i.cost, settings.DECIMAL_PLACES),
            isinstance(i, frepple.operation_alternate) and i.search or None,
            i.description, i.category, i.subcategory, i.source, self.timestamp, i.name
          )
          for i in frepple.operations()
          if i.name in primary_keys and not i.hidden and i.name != 'setup operation' and (not self.source or self.source == i.source)
        ])
      print('Exported operations in %.2f seconds' % (time() - starttime))


  def exportSubOperations(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting suboperations...")
      starttime = time()
      cursor.execute("SELECT operation_id, suboperation_id FROM suboperation")
      primary_keys = set([ i for i in cursor.fetchall() ])

      def subops():
        for i in frepple.operations():
          if not i.hidden and isinstance(i, (frepple.operation_split, frepple.operation_routing, frepple.operation_alternate)):
            for j in i.suboperations:
              yield j

      cursor.executemany(
        "insert into suboperation \
        (operation_id,suboperation_id,priority,effective_start,effective_end,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s,%s)",
        [
          (i.owner.name, i.operation.name, i.priority, i.effective_start, i.effective_end, i.source, self.timestamp)
          for i in subops()
          if (i.owner.name, i.operation.name) not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update suboperation \
         set priority=%s, effective_start=%s, effective_end=%s, source=%s, lastmodified=%s \
         where operation_id=%s and suboperation_id=%s",
        [
          (i.priority, i.effective_start, i.effective_end, i.source, self.timestamp, i.owner.name, i.operation.name)
          for i in subops()
          if (i.owner.name, i.operation.name) in primary_keys and (not self.source or self.source == i.source)
        ])
      print('Exported suboperations in %.2f seconds' % (time() - starttime))


  def exportFlows(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting flows...")
      starttime = time()
      cursor.execute("SELECT operation_id, thebuffer_id FROM flow")  # todo oper&buffer are not necesarily unique
      primary_keys = set([ i for i in cursor.fetchall() ])

      def flows():
        for o in frepple.operations():
          for i in o.flows:
            yield i

      cursor.executemany(
        '''insert into flow
        (operation_id,thebuffer_id,quantity,type,effective_start,effective_end,name,priority,
        search,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i.operation.name, i.buffer.name, round(i.quantity, settings.DECIMAL_PLACES),
            i.type[5:], str(i.effective_start), str(i.effective_end),
            i.name, i.priority, i.search != 'PRIORITY' and i.search or None, i.source, self.timestamp
          )
          for i in flows()
          if (i.operation.name, i.buffer.name) not in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update flow
         set quantity=%s, type=%s, effective_start=%s, effective_end=%s, name=%s,
         priority=%s, search=%s, source=%s, lastmodified=%s
         where operation_id=%s and thebuffer_id=%s''',
        [
          (
            round(i.quantity, settings.DECIMAL_PLACES),
            i.type[5:], str(i.effective_start), str(i.effective_end),
            i.name, i.priority, i.search != 'PRIORITY' and i.search or None, i.source,
            self.timestamp, i.operation.name, i.buffer.name,
          )
          for i in flows()
          if (i.operation.name, i.buffer.name) in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      print('Exported flows in %.2f seconds' % (time() - starttime))


  def exportLoads(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting loads...")
      starttime = time()
      cursor.execute("SELECT operation_id, resource_id FROM resourceload")  # todo oper&resource are not necesarily unique
      primary_keys = set([ i for i in cursor.fetchall() ])

      def loads():
        for o in frepple.operations():
          for i in o.loads:
            yield i

      cursor.executemany(
        '''insert into resourceload
        (operation_id,resource_id,quantity,setup,effective_start,effective_end,name,priority,
        search,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i.operation.name, i.resource.name, round(i.quantity, settings.DECIMAL_PLACES),
            i.setup, str(i.effective_start), str(i.effective_end),
            i.name, i.priority, i.search != 'PRIORITY' and i.search or None,
            i.source, self.timestamp
          )
          for i in loads()
          if (i.operation.name, i.resource.name) not in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update resourceload
         set quantity=%s, setup=%s, effective_start=%s, effective_end=%s, name=%s,
         priority=%s, search=%s, source=%s, lastmodified=%s
         where operation_id=%s and resource_id=%s''',
        [
          (
            round(i.quantity, settings.DECIMAL_PLACES),
            i.setup, str(i.effective_start), str(i.effective_end),
            i.name, i.priority, i.search != 'PRIORITY' and i.search or None,
            i.source, self.timestamp, i.operation.name, i.resource.name,
          )
          for i in loads()
          if (i.operation.name, i.resource.name) in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      print('Exported loads in %.2f seconds' % (time() - starttime))


  def exportBuffers(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting buffers...")
      starttime = time()
      cursor.execute("SELECT name FROM buffer")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into buffer
        (name,description,location_id,item_id,onhand,minimum,minimum_calendar_id,
         producing_id,type,leadtime,min_inventory,
         max_inventory,min_interval,max_interval,size_minimum,
         size_multiple,size_maximum,fence,
         carrying_cost,category,subcategory,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i.name, i.description, i.location and i.location.name or None,
            i.item and i.item.name or None,
            round(i.onhand, settings.DECIMAL_PLACES), round(i.minimum, settings.DECIMAL_PLACES),
            i.minimum_calendar and i.minimum_calendar.name or None,
            (not isinstance(i, frepple.buffer_procure) and i.producing) and i.producing.name or None,
            i.__class__.__name__[7:],
            isinstance(i, frepple.buffer_procure) and i.leadtime or None,
            isinstance(i, frepple.buffer_procure) and round(i.mininventory, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and round(i.maxinventory, settings.DECIMAL_PLACES) or None,
            i.mininterval,
            i.maxinterval < 99999999999 and i.maxinterval or None,
            isinstance(i, frepple.buffer_procure) and round(i.size_minimum, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and round(i.size_multiple, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and i.size_maximum < 99999999999 and round(i.size_maximum, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and i.fence or None,
            round(i.carrying_cost, settings.DECIMAL_PLACES), i.category, i.subcategory,
            i.source, self.timestamp
          )
          for i in frepple.buffers()
          if i.name not in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update buffer
         set description=%s, location_id=%s, item_id=%s, onhand=%s, minimum=%s, minimum_calendar_id=%s,
         producing_id=%s, type=%s, leadtime=%s, min_inventory=%s, max_inventory=%s, min_interval=%s,
         max_interval=%s, size_minimum=%s, size_multiple=%s, size_maximum=%s, fence=%s,
         carrying_cost=%s, category=%s, subcategory=%s, source=%s, lastmodified=%s
         where name=%s''',
        [
          (
            i.description, i.location and i.location.name or None, i.item and i.item.name or None,
            round(i.onhand, settings.DECIMAL_PLACES), round(i.minimum, settings.DECIMAL_PLACES),
            i.minimum_calendar and i.minimum_calendar.name or None,
            (not isinstance(i, frepple.buffer_procure) and i.producing) and i.producing.name or None,
            i.__class__.__name__[7:],
            isinstance(i, frepple.buffer_procure) and i.leadtime or None,
            isinstance(i, frepple.buffer_procure) and round(i.mininventory, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and round(i.maxinventory, settings.DECIMAL_PLACES) or None,
            (i.mininterval!=-1) and i.mininterval or None,
            i.maxinterval < 99999999999 and i.maxinterval or None,
            isinstance(i, frepple.buffer_procure) and round(i.size_minimum, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and round(i.size_multiple, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and i.size_maximum < 99999999999 and round(i.size_maximum, settings.DECIMAL_PLACES) or None,
            isinstance(i, frepple.buffer_procure) and i.fence or None,
            round(i.carrying_cost, settings.DECIMAL_PLACES), i.category, i.subcategory,
            i.source, self.timestamp, i.name
          )
          for i in frepple.buffers()
          if i.name in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update buffer set owner_id=%s where name=%s",
        [
          (i.owner.name, i.name)
          for i in frepple.buffers()
          if i.owner and not i.hidden
        ])
      print('Exported buffers in %.2f seconds' % (time() - starttime))


  def exportCustomers(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting customers...")
      starttime = time()
      cursor.execute("SELECT name FROM customer")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into customer \
        (name,description,category,subcategory,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s)",
        [
          (i.name, i.description, i.category, i.subcategory, i.source, self.timestamp)
          for i in frepple.customers()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update customer \
         set description=%s, category=%s, subcategory=%s, source=%s, lastmodified=%s \
         where name=%s",
        [
          (i.description, i.category, i.subcategory, i.source, self.timestamp, i.name)
          for i in frepple.customers()
          if i.name in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update customer set owner_id=%s where name=%s",
        [
          (i.owner.name, i.name)
          for i in frepple.customers()
          if i.owner and (not self.source or self.source == i.source)
        ])
      print('Exported customers in %.2f seconds' % (time() - starttime))


  def exportSuppliers(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting suppliers...")
      starttime = time()
      cursor.execute("SELECT name FROM supplier")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into supplier \
        (name,description,category,subcategory,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s)",
        [
          (i.name, i.description, i.category, i.subcategory, i.source, self.timestamp)
          for i in frepple.suppliers()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update supplier \
         set description=%s, category=%s, subcategory=%s, source=%s, lastmodified=%s \
         where name=%s",
        [
          (i.description, i.category, i.subcategory, i.source, self.timestamp, i.name)
          for i in frepple.suppliers()
          if i.name in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update supplier set owner_id=%s where name=%s",
        [
          (i.owner.name, i.name)
          for i in frepple.suppliers()
          if i.owner and (not self.source or self.source == i.source)
        ])
      print('Exported suppliers in %.2f seconds' % (time() - starttime))


  def exportItemSuppliers(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):

      def itemsuppliers():
        for o in frepple.suppliers():
          for i in o.itemsuppliers:
            yield i

      print("Exporting item suppliers...")
      starttime = time()
      default_start = datetime.datetime(1971, 1, 1)
      default_end = datetime.datetime(2030, 12, 31)
      cursor.execute("SELECT supplier_id, item_id, location_id, effective_start FROM itemsupplier")
      primary_keys = set([ (i[0], i[1], i[2], i[3] if i[3] else default_start) for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into itemsupplier \
        (supplier_id,item_id,location_id,leadtime,sizeminimum,sizemultiple, \
         cost,priority,effective_start,effective_end,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)",
        [
          (i.supplier.name, i.item.name, i.location.name if i.location else None,
           i.leadtime, i.size_minimum, i.size_multiple, i.cost, i.priority,
           i.effective_start if i.effective_start != default_start else None,
           i.effective_end if i.effective_end != default_end else None,
           i.source, self.timestamp)
          for i in itemsuppliers()
          if (i.supplier.name, i.item.name, i.location.name if i.location else None, i.effective_start) not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update itemsupplier \
         set leadtime=%s, sizeminimum=%s, sizemultiple=%s, \
         cost=%s, priority=%s, effective_end=%s, \
         source=%s, lastmodified=%s \
         where supplier_id=%s and item_id=%s and location_id=%s and effective_start=%s",
        [
          (i.leadtime, i.size_minimum, i.size_multiple, i.cost, i.priority,
           i.effective_end if i.effective_end != default_end else None,
           i.source, self.timestamp,
           i.supplier.name, i.item.name, i.location.name if i.location else None,
           i.effective_start)
          for i in itemsuppliers()
          if (i.supplier.name, i.item.name, i.location.name if i.location else None, i.effective_start) in primary_keys and i.effective_start != default_start and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update itemsupplier \
         set leadtime=%s, sizeminimum=%s, sizemultiple=%s, \
         cost=%s, priority=%s, effective_end=%s, \
         source=%s, lastmodified=%s \
         where supplier_id=%s and item_id=%s and location_id=%s and effective_start is null",
        [
          (i.leadtime, i.size_minimum, i.size_multiple, i.cost, i.priority,
           i.effective_end if i.effective_end != default_end else None,
           i.source, self.timestamp,
           i.supplier.name, i.item.name, i.location.name if i.location else None)
          for i in itemsuppliers()
          if (i.supplier.name, i.item.name, i.location.name if i.location else None, i.effective_start) in primary_keys and i.effective_start == default_start and (not self.source or self.source == i.source)
        ])
      print('Exported item suppliers in %.2f seconds' % (time() - starttime))


  def exportItemDistributions(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):

      def itemdistributions():
        for o in frepple.items():
          for i in o.itemdistributions:
            yield i

      print("Exporting item distributions...")
      starttime = time()
      default_start = datetime.datetime(1971, 1, 1)
      default_end = datetime.datetime(2030, 12, 31)
      cursor.execute("SELECT origin_id, item_id, location_id, effective_start FROM itemdistribution")
      primary_keys = set([ (i[0], i[1], i[2], i[3] if i[3] else default_start) for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into itemdistribution \
        (origin_id,item_id,location_id,leadtime,sizeminimum,sizemultiple, \
         cost,priority,effective_start,effective_end,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)",
        [
          (i.origin.name, i.item.name, i.destination.name if i.destination else None,
           i.leadtime, i.size_minimum, i.size_multiple, i.cost, i.priority,
           i.effective_start if i.effective_start != default_start else None,
           i.effective_end if i.effective_end != default_end else None,
           i.source, self.timestamp)
          for i in itemdistributions()
          if (i.origin.name, i.item.name, i.destination.name if i.destination else None, i.effective_start) not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update itemdistribution \
         set leadtime=%s, sizeminimum=%s, sizemultiple=%s, \
         cost=%s, priority=%s, effective_end=%s, \
         source=%s, lastmodified=%s \
         where origin_id=%s and item_id=%s and location_id=%s and effective_start=%s",
        [
          (i.leadtime, i.size_minimum, i.size_multiple, i.cost, i.priority,
           i.effective_end if i.effective_end != default_end else None,
           i.source, self.timestamp, i.origin.name, i.item.name,
           i.destination.name if i.destination else None,
           i.effective_start)
          for i in itemdistributions()
          if (i.origin.name, i.item.name, i.destination.name if i.destination else None, i.effective_start) in primary_keys and i.effective_start != default_start and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update itemdistribution \
         set leadtime=%s, sizeminimum=%s, sizemultiple=%s, \
         cost=%s, priority=%s, effective_end=%s, \
         source=%s, lastmodified=%s \
         where origin_id=%s and item_id=%s and location_id=%s and effective_start is null",
        [
          (i.leadtime, i.size_minimum, i.size_multiple, i.cost, i.priority,
           i.effective_end if i.effective_end != default_end else None,
           i.source, self.timestamp, i.origin.name, i.item.name,
           i.destination.name if i.destination else None)
          for i in itemdistributions()
          if (i.origin.name, i.item.name, i.destination.name if i.destination else None, i.effective_start) in primary_keys and i.effective_start == default_start and (not self.source or self.source == i.source)
        ])
      print('Exported item distributions in %.2f seconds' % (time() - starttime))


  def exportDemands(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting demands...")
      starttime = time()
      cursor.execute("SELECT name FROM demand")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into demand
        (name,due,quantity,priority,item_id,location_id,operation_id,customer_id,
         minshipment,maxlateness,category,subcategory,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i.name, str(i.due), round(i.quantity, settings.DECIMAL_PLACES), i.priority, i.item.name,
            i.location.name if i.location else None, i.operation.name if i.operation else None,
            i.customer.name if i.customer else None,
            round(i.minshipment, settings.DECIMAL_PLACES), round(i.maxlateness, settings.DECIMAL_PLACES),
            i.category, i.subcategory, i.source, self.timestamp
          )
          for i in frepple.demands()
          if i.name not in primary_keys and isinstance(i, frepple.demand_default) and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update demand
         set due=%s, quantity=%s, priority=%s, item_id=%s, location_id=%s,
         operation_id=%s, customer_id=%s, minshipment=%s, maxlateness=%s,
         category=%s, subcategory=%s, source=%s, lastmodified=%s
         where name=%s''',
        [
          (
            str(i.due), round(i.quantity, settings.DECIMAL_PLACES), i.priority,
            i.item.name, i.location.name if i.location else None,
            i.operation.name if i.operation else None,
            i.customer.name if i.customer else None,
            round(i.minshipment, settings.DECIMAL_PLACES),
            round(i.maxlateness, settings.DECIMAL_PLACES),
            i.category, i.subcategory, i.source, self.timestamp, i.name
          )
          for i in frepple.demands()
          if i.name in primary_keys and isinstance(i, frepple.demand_default) and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update demand set owner_id=%s where name=%s",
        [
          (i.owner.name, i.name)
          for i in frepple.demands()
          if i.owner and isinstance(i, frepple.demand_default) and (not self.source or self.source == i.source)
        ])
      print('Exported demands in %.2f seconds' % (time() - starttime))


  def exportOperationPlans(self, cursor):
    '''
    Only locked operationplans are exported. That because we assume that
    all of those were given as input.
    '''
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting operationplans...")
      starttime = time()
      cursor.execute("SELECT id FROM operationplan")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into operationplan
        (id,operation_id,quantity,startdate,enddate,status,source,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
         (
           i.id, i.operation.name, round(i.quantity, settings.DECIMAL_PLACES),
           str(i.start), str(i.end), i.status, i.source, self.timestamp
         )
         for i in frepple.operationplans()
         if i.locked and not i.operation.hidden and i.id not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update operationplan
         set operation_id=%s, quantity=%s, startdate=%s, enddate=%s, status=%s, source=%s, lastmodified=%s
         where id=%s''',
        [
         (
           i.operation.name, round(i.quantity, settings.DECIMAL_PLACES),
           str(i.start), str(i.end), i.status, i.source, self.timestamp, i.id
         )
         for i in frepple.operationplans()
         if i.locked and not i.operation.hidden and i.id in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update operationplan set owner_id=%s where id=%s",
        [
          (i.owner.id, i.id)
          for i in frepple.operationplans()
          if i.owner and not i.operation.hidden and i.locked and (not self.source or self.source == i.source)
        ])
      print('Exported operationplans in %.2f seconds' % (time() - starttime))


  def exportResources(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting resources...")
      starttime = time()
      cursor.execute("SELECT name FROM %s" % connections[self.database].ops.quote_name('resource'))
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into %s
        (name,description,maximum,maximum_calendar_id,location_id,type,cost,
         maxearly,setup,setupmatrix_id,category,subcategory,source,lastmodified)
        values(%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s)''' % connections[self.database].ops.quote_name('resource'),
        [
          (
            i.name, i.description, i.maximum, i.maximum_calendar and i.maximum_calendar.name or None,
            i.location and i.location.name or None, i.__class__.__name__[9:],
            round(i.cost, settings.DECIMAL_PLACES), round(i.maxearly, settings.DECIMAL_PLACES),
            i.setup, i.setupmatrix and i.setupmatrix.name or None,
            i.category, i.subcategory, i.source, self.timestamp
          )
          for i in frepple.resources()
          if i.name not in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update %s
         set description=%%s, maximum=%%s, maximum_calendar_id=%%s, location_id=%%s,
         type=%%s, cost=%%s, maxearly=%%s, setup=%%s, setupmatrix_id=%%s, category=%%s,
         subcategory=%%s, source=%%s, lastmodified=%%s
         where name=%%s''' % connections[self.database].ops.quote_name('resource'),
        [
          (
            i.description, i.maximum,
            i.maximum_calendar and i.maximum_calendar.name or None,
            i.location and i.location.name or None, i.__class__.__name__[9:],
            round(i.cost, settings.DECIMAL_PLACES),
            round(i.maxearly, settings.DECIMAL_PLACES),
            i.setup, i.setupmatrix and i.setupmatrix.name or None,
            i.category, i.subcategory, i.source, self.timestamp, i.name
          )
          for i in frepple.resources()
          if i.name in primary_keys and not i.hidden and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update %s set owner_id=%%s where name=%%s" % connections[self.database].ops.quote_name('resource'),
        [
          (i.owner.name, i.name)
          for i in frepple.resources()
          if i.owner and not i.hidden and (not self.source or self.source == i.source)
        ])
      print('Exported resources in %.2f seconds' % (time() - starttime))


  def exportSkills(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting skills...")
      starttime = time()
      cursor.execute("SELECT name FROM skill")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into skill (name,source,lastmodified) values(%s,%s,%s)''',
        [
          ( i.name, i.source, self.timestamp )
          for i in frepple.skills()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update skill set source=%s, lastmodified=%s where name=%s''',
        [
          (i.source, self.timestamp, i.name)
          for i in frepple.skills()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      print('Exported skills in %.2f seconds' % (time() - starttime))


  def exportResourceSkills(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting resource skills...")
      starttime = time()
      cursor.execute("SELECT resource_id, skill_id FROM resourceskill")  # todo resource&skill are not necesarily unique
      primary_keys = set([ i for i in cursor.fetchall() ])

      def res_skills():
        for s in frepple.skills():
          for r in s.resources:
            yield (r.effective_start, r.effective_end, r.priority, r.source, self.timestamp, r.name, s.name)

      cursor.executemany(
        '''insert into resourceskill
        (effective_start,effective_end,priority,source,lastmodified,resource_id,skill_id)
        values(%s,%s,%s,%s,%s,%s,%s)''',
        [
          i for i in res_skills()
          if i not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        '''update resourceskill
        set effective_start=%s, effective_end=%s, priority=%s, source=%s, lastmodified=%s
        where resource_id=%s and skill_id=%s''',
        [
          i for i in res_skills()
          if i not in primary_keys and (not self.source or self.source == i.source)
        ])
      print('Exported resource skills in %.2f seconds' % (time() - starttime))


  def exportSetupMatrices(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting setup matrices...")
      starttime = time()
      cursor.execute("SELECT name FROM setupmatrix")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into setupmatrix \
        (name,source,lastmodified) \
        values(%s,%s,%s)",
        [
          (i.name, i.source, self.timestamp)
          for i in frepple.setupmatrices()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update setupmatrix \
         set source=%s, lastmodified=%s \
         where name=%s",
        [
          (i.source, self.timestamp, i.name)
          for i in frepple.setupmatrices()
          if i.name in primary_keys and (not self.source or self.source == i.source)
        ])
      print('Exported setupmatrices in %.2f seconds' % (time() - starttime))


  def exportSetupMatricesRules(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting setup matrix rules...")
      starttime = time()
      cursor.execute("SELECT setupmatrix_id, priority FROM setuprule")
      primary_keys = set([ i for i in cursor.fetchall() ])

      def matrixrules():
        for m in frepple.setupmatrices():
          for i in m.rules:
            yield m, i

      cursor.executemany(
        "insert into setuprule \
        (setupmatrix_id,priority,fromsetup,tosetup,duration,cost,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s,%s,%s)",
        [
         (
           i[0].name, i[1].priority, i[1].fromsetup, i[1].tosetup, i[1].duration,
           round(i[1].cost, settings.DECIMAL_PLACES),
           i.source, self.timestamp
         )
         for i in matrixrules()
         if (i[0].name, i[1].priority) not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update setuprule \
         set fromsetup=%s, tosetup=%s, duration=%s, cost=%s, source=%s, lastmodified=%s \
         where setupmatrix_id=%s and priority=%s",
        [
          (
            i[1].fromsetup, i[1].tosetup, i[1].duration, round(i[1].cost, settings.DECIMAL_PLACES),
            i.source, self.timestamp, i[0].name, i[1].priority
          )
          for i[1] in matrixrules()
          if (i[0].name, i[1].priority) in primary_keys and (not self.source or self.source == i.source)
        ])
      print('Exported setup matrix rules in %.2f seconds' % (time() - starttime))


  def exportItems(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting items...")
      starttime = time()
      cursor.execute("SELECT name FROM item")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        "insert into item \
        (name,description,operation_id,price,category,subcategory,source,lastmodified) \
        values(%s,%s,%s,%s,%s,%s,%s,%s)",
        [
          (
            i.name, i.description, i.operation and i.operation.name or None,
            round(i.price, settings.DECIMAL_PLACES), i.category, i.subcategory,
            i.source, self.timestamp
          )
          for i in frepple.items()
          if i.name not in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update item \
         set description=%s, operation_id=%s, price=%s, category=%s, subcategory=%s, source=%s, lastmodified=%s \
         where name=%s",
        [
          (
            i.description, i.operation and i.operation.name or None,
            round(i.price, settings.DECIMAL_PLACES), i.category, i.subcategory,
            i.source, self.timestamp, i.name
          )
          for i in frepple.items()
          if i.name in primary_keys and (not self.source or self.source == i.source)
        ])
      cursor.executemany(
        "update item set owner_id=%s where name=%s",
        [
          (i.owner.name, i.name)
          for i in frepple.items()
          if i.owner and (not self.source or self.source == i.source)
        ])
      print('Exported items in %.2f seconds' % (time() - starttime))


  def exportParameters(self, cursor):
    with transaction.atomic(using=self.database, savepoint=False):
      if self.source:
        # Only complete export should save the current date
        return
      print("Exporting parameters...")
      starttime = time()
      # Update current date if the parameter already exists
      # If it doesn't exist, we want to continue using the system clock for the next run.
      cursor.execute(
        "UPDATE common_parameter SET value=%s, lastmodified=%s WHERE name='currentdate'",
        (frepple.settings.current.strftime("%Y-%m-%d %H:%M:%S"), self.timestamp)
        )
      print('Exported parameters in %.2f seconds' % (time() - starttime))


  def exportForecasts(self, cursor):
    # Detect whether the forecast module is available
    if not 'demand_forecast' in [ a[0] for a in inspect.getmembers(frepple) ]:
      return
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting forecast...")
      starttime = time()
      cursor.execute("SELECT name FROM forecast")
      primary_keys = set([ i[0] for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into forecast
        (name,customer_id,item_id,priority,operation_id,minshipment,
         calendar_id,discrete,maxlateness,category,subcategory,lastmodified)
        values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
        [
          (
            i.name, i.customer and i.customer.name or None, i.item.name, i.priority,
            i.operation and i.operation.name or None, round(i.minshipment, settings.DECIMAL_PLACES),
            i.calendar.name, i.discrete, round(i.maxlateness, settings.DECIMAL_PLACES),
            i.category, i.subcategory, self.timestamp
          )
          for i in frepple.demands()
          if i.name not in primary_keys and isinstance(i, frepple.demand_forecast)
        ])
      cursor.executemany(
        '''update forecast
         set customer_id=%s, item_id=%s, priority=%s, operation_id=%s, minshipment=%s,
         calendar_id=%s, discrete=%s,maxlateness=%s, category=%s, subcategory=%s, lastmodified=%s
         where name=%s''',
        [
          (
            i.customer and i.customer.name or None, i.item.name, i.priority,
            i.operation and i.operation.name or None, round(i.minshipment, settings.DECIMAL_PLACES),
            i.calendar.name, i.discrete, round(i.maxlateness, settings.DECIMAL_PLACES),
            i.category, i.subcategory, self.timestamp, i.name,
          )
          for i in frepple.demands()
          if i.name in primary_keys and isinstance(i, frepple.demand_forecast)
        ])
      print('Exported forecasts in %.2f seconds' % (time() - starttime))


  def exportForecastDemands(self, cursor):
    # Detect whether the forecast module is available
    if not 'demand_forecast' in [ a[0] for a in inspect.getmembers(frepple) ]:
      return
    with transaction.atomic(using=self.database, savepoint=False):
      print("Exporting forecast demands...")
      starttime = time()
      cursor.execute("SELECT forecast_id, startdate, enddate FROM forecastdemand")
      primary_keys = set([ i for i in cursor.fetchall() ])
      cursor.executemany(
        '''insert into forecastdemand
        (forecast_id,startdate,enddate,quantity,lastmodified)
        values(%s,%s,%s,%s,%s)''',
        [
         (
           i.owner.name, str(i.startdate.date()), str(i.enddate.date()),
           round(i.total, settings.DECIMAL_PLACES), self.timestamp
         )
         for i in frepple.demands()
         if isinstance(i, frepple.demand_forecastbucket) and (i.owner.name, i.startdate.date(), i.enddate.date()) not in primary_keys
        ])
      cursor.executemany(
        '''update forecastdemand
         set quantity=%s, lastmodified=%s
         where forecast_id=%s and startdate=%s and enddate=%s''',
        [
          (
            round(i.total, settings.DECIMAL_PLACES), self.timestamp,
            i.owner.name, str(i.startdate.date()), str(i.enddate.date()),
          )
          for i in frepple.demands()
          if isinstance(i, frepple.demand_forecastbucket) and (i.owner.name, i.startdate.date(), i.enddate.date()) in primary_keys
        ])
      print('Exported forecast demands in %.2f seconds' % (time() - starttime))


  def run(self):
    '''
    This function exports the data from the frePPLe memory into the database.
    '''
    # Make sure the debug flag is not set!
    # When it is set, the django database wrapper collects a list of all sql
    # statements executed and their timings. This consumes plenty of memory
    # and cpu time.
    tmp = settings.DEBUG
    settings.DEBUG = False
    self.timestamp = str(datetime.datetime.now())

    try:
      # Create a database connection
      cursor = connections[self.database].cursor()

      if False:
        # OPTION 1: Sequential export of each entity
        # The parallel export normally gives a better performance, but
        # you could still choose a sequential export.
        try:
          self.exportParameters(cursor)
          self.exportCalendars(cursor)
          self.exportCalendarBuckets(cursor)
          self.exportLocations(cursor)
          self.exportOperations(cursor)
          self.exportSubOperations(cursor)
          self.exportOperationPlans(cursor)
          self.exportItems(cursor)
          self.exportBuffers(cursor)
          self.exportFlows(cursor)
          self.exportSetupMatrices(cursor)
          self.exportSetupMatricesRules(cursor)
          self.exportResources(cursor)
          self.exportSkills(cursor)
          self.exportResourceSkills(cursor)
          self.exportLoads(cursor)
          self.exportCustomers(cursor)
          self.exportSuppliers(cursor)
          self.exportItemSuppliers(cursor)
          self.exportDemands(cursor)
          self.exportForecasts(cursor)
          self.exportForecastDemands(cursor)
        except:
          traceback.print_exc()

      else:
        # OPTION 2: Parallel export of entities in groups.
        # The groups are running in separate threads, and all functions in a group
        # are run in sequence.
        try:
          self.exportCalendars(cursor)
          self.exportLocations(cursor)
          self.exportOperations(cursor)
          self.exportItems(cursor)
          tasks = (
            DatabaseTask(self, self.exportCalendarBuckets, self.exportSubOperations, self.exportOperationPlans, self.exportParameters),
            DatabaseTask(self, self.exportBuffers, self.exportFlows, self.exportSuppliers, self.exportItemSuppliers, self.exportItemDistributions),
            DatabaseTask(self, self.exportSetupMatrices, self.exportSetupMatricesRules, self.exportResources, self.exportSkills, self.exportResourceSkills, self.exportLoads),
            DatabaseTask(self, self.exportCustomers, self.exportDemands, self.exportForecasts, self.exportForecastDemands),
            )
          # Start all threads
          for i in tasks:
            i.start()
          # Wait for all threads to finish
          for i in tasks:
            i.join()
        except Exception as e:
          print("Error exporting static model:", e)

      # Cleanup unused records
      if self.source:
        cursor.execute("delete from flow where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from buffer where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from demand where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from itemsupplier where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from itemdistribution where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from item where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from operationplan where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from suboperation where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from resourceload where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from resourceskill where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from operation where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from suboperation where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from resource where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from location where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from calendar where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from skill where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from setuprule where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from setupmatrix where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from customer where source = %s and lastmodified <> %s", (self.source, self.timestamp))
        cursor.execute("delete from supplier where source = %s and lastmodified <> %s", (self.source, self.timestamp))

      # Close the database connection
      cursor.close()

    finally:
      # Restore the previous setting
      settings.DEBUG = tmp


class DatabaseTask(Thread):
  '''
  An auxiliary class that allows us to run a function with its own
  database connection in its own thread.
  '''
  def __init__(self, xprt, *f):
    super(DatabaseTask, self).__init__()
    self.export = xprt
    self.functions = f

  def run(self):
    # Create a database connection
    cursor = connections[self.export.database].cursor()

    # Run the functions sequentially
    for f in self.functions:
      try:
        f(cursor)
      except:
        traceback.print_exc()

    # Close the connection
    cursor.close()
