#
# Copyright (C) 2014 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from optparse import make_option
from datetime import datetime, timedelta, date
from time import time
from xml.etree.cElementTree import iterparse
from xml.etree.ElementTree import ParseError
import urllib
from io import StringIO

from django.core.management.base import BaseCommand, CommandError
from django.db import transaction, connections, DEFAULT_DB_ALIAS
from django.conf import settings

from freppledb.common.models import Parameter
from freppledb.execute.models import Task
from freppledb.openbravo.utils import get_data as get_data_base


class Command(BaseCommand):

  help = "Loads data from an Openbravo instance into the frePPLe database"

  option_list = BaseCommand.option_list + (
    make_option(
      '--user', dest='user', type='string',
      help='User running the command'
      ),
    make_option(
      '--delta', action='store', dest='delta', type="float", default='3650',
      help='Number of days for which we extract changed openbravo data'
      ),
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS, help='Nominates the frePPLe database to load'
      ),
    make_option(
      '--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'
      ),
    )

  requires_system_checks = False

  def handle(self, **options):

    # Pick up the options
    if 'verbosity' in options:
      self.verbosity = int(options['verbosity'] or '1')
    else:
      self.verbosity = 1
    if 'user' in options:
      user = options['user']
    else:
      user = ''
    if 'database' in options:
      self.database = options['database'] or DEFAULT_DB_ALIAS
    else:
      self.database = DEFAULT_DB_ALIAS
    if self.database not in settings.DATABASES.keys():
      raise CommandError("No database settings known for '%s'" % self.database )
    if 'delta' in options:
      self.delta = float(options['delta'] or '3650')
      self.incremental = (self.delta < 3650)
    else:
      self.delta = 3650
      self.incremental = False

    # Pick up configuration s
    self.openbravo_user = Parameter.getValue("openbravo.user", self.database)
    # Passwords in djangosettings file are preferably used
    self.openbravo_password = settings.OPENBRAVO_PASSWORDS.get(self.database)
    if not self.openbravo_password:
      self.openbravo_password = Parameter.getValue("openbravo.password", self.database)
    self.openbravo_host = Parameter.getValue("openbravo.host", self.database)
    self.openbravo_pagesize = int(Parameter.getValue("openbravo.pagesize", self.database, default='1000'))
    self.openbravo_dateformat = Parameter.getValue("openbravo.date_format", self.database, default='%Y-%m-%d')
    if not self.openbravo_user:
      raise CommandError("Missing or invalid  openbravo_user")
    if not self.openbravo_password:
      raise CommandError("Missing or invalid  openbravo_password")
    if not self.openbravo_host:
      raise CommandError("Missing or invalid  openbravo_host")

    # Make sure the debug flag is not set!
    # When it is set, the django database wrapper collects a list of all sql
    # statements executed and their timings. This consumes plenty of memory
    # and cpu time.
    tmp_debug = settings.DEBUG
    settings.DEBUG = False

    now = datetime.now()
    task = None
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try:
          task = Task.objects.all().using(self.database).get(pk=options['task'])
        except:
          raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'Openbravo import':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(
          name='Openbravo import', submitted=now, started=now, status='0%',
          user=user, arguments="--delta=%s" % self.delta
          )
      task.save(using=self.database)

      # Create a database connection to the frePPLe database
      cursor = connections[self.database].cursor()

      # Dictionaries for the mapping between openbravo ids and frepple names
      self.organizations = {}
      self.locations = {}
      self.organization_location = {}
      self.customers = {}
      self.suppliers = {}
      self.itemsupplier = {}
      self.items = {}
      self.locators = {}
      self.resources = {}
      self.date = datetime.now()
      self.delta = (date.today() - timedelta(days=self.delta)).strftime(self.openbravo_dateformat)

      # Pick up the current date
      try:
        cursor.execute("SELECT value FROM common_ where name='currentdate'")
        d = cursor.fetchone()
        self.current = datetime.strptime(d[0], "%Y-%m-%d %H:%M:%S")
      except:
        self.current = datetime.now()

      # Get the biggest operationplan identifier in the current plan
      cursor.execute('''
        select greatest(oper.max_id, pur.max_id, dist.max_id)
        from
          (select max(id) max_id from operationplan) oper,
          (select max(id) max_id from purchase_order) pur,
          (select max(id) max_id from distribution_order) dist
        ''')
      self.idcounter = cursor.fetchone()[0] or 1

      # Sequentially load all data
      self.importData(task, cursor)

      # Log success
      task.status = 'Done'
      task.finished = datetime.now()

    except Exception as e:
      if task:
        task.status = 'Failed'
        task.message = '%s' % e
        task.finished = datetime.now()
      raise e

    finally:
      if task:
        task.save(using=self.database)
      settings.DEBUG = tmp_debug

  def get_data(self, url, callback):
    firstResult = 0
    # Retrieve openbravo data page per page
    while True:
      # Send the request
      if '?' in url:
        url2 = "%s&firstResult=%d&maxResult=%d" % (url, firstResult, self.openbravo_pagesize)
      else:
        url2 = "%s?firstResult=%d&maxResult=%d" % (url, firstResult, self.openbravo_pagesize)
      if self.verbosity > 1:
        print('Request: ', url2)

      data = get_data_base(url2, self.openbravo_host, self.openbravo_user, self.openbravo_password).replace("&#0;","").replace("&#22;","").replace("\xcc","")
      if self.verbosity == 1:
        print('.', end="")
      elif self.verbosity > 2:
        print('Response content: ', data)
      conn = iterparse(StringIO(data), events=('start', 'end'))
      try:
        count = callback(conn)
      except ParseError:
        print ("Error parsing Openbravo XML document")
        count = self.openbravo_pagesize
      if count < self.openbravo_pagesize:
        # No more records to be expected
        if self.verbosity == 1:
          print('')
        return firstResult + count
      else:
        # Prepare for the next loop
        firstResult += self.openbravo_pagesize


  def importData(self, task, cursor):
    self.import_organizations(cursor)
    self.import_customers(cursor)
    task.status = '10%'
    self.import_suppliers(cursor)
    task.status = '15%'
    task.save(using=self.database)
    self.import_products(cursor)
    task.status = '20%'
    task.save(using=self.database)
    self.import_locations(cursor)
    task.status = '30%'
    task.save(using=self.database)
    self.import_salesorders(cursor)
    task.status = '40%'
    task.save(using=self.database)
    self.import_machines(cursor)
    task.status = '50%'
    task.save(using=self.database)
    self.import_onhand(cursor)
    task.status = '60%'
    task.save(using=self.database)
    self.import_itemsupplier(cursor)
    task.status = '70%'
    task.save(using=self.database)
    self.import_purchaseorders(cursor)
    exportPurchasingPlan = Parameter.getValue("openbravo.exportPurchasingPlan", self.database, default="false")
    if exportPurchasingPlan == "true":
      task.status = '75%'
      task.save(using=self.database)
      self.import_purchasingplan(cursor)
    task.status = '80%'
    task.save(using=self.database)
    self.import_productbom(cursor)
    task.status = '90%'
    task.save(using=self.database)
    task.status = '95%'
    task.save(using=self.database)
    self.import_workInProgress(cursor)


  # Load a mapping of Openbravo organizations to their search key.
  # The result is used as a lookup in other interface where the
  # organization needs to be translated into a human readable form.
  #
  # If an organization is not present in the mapping dictionary, its
  # sales orders and purchase orders aren't mapped into frePPLe. This
  # can be used as a simple filter for some data.
  def import_organizations(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'Organization':
          continue
        records += 1
        searchkey = elem.find("searchKey").text
        objectid = elem.get('id')
        self.organizations[objectid] = searchkey
        # Clean the XML hierarchy
        root.clear()
      return records

    try:
      starttime = time()
      if self.verbosity > 0:
        print("Importing organizations...")
      count = self.get_data("/openbravo/ws/dal/Organization?includeChildren=false", parse)
      if self.verbosity > 0:
        print("Loaded %d organizations in %.2f seconds" % (count, time() - starttime))
    except Exception as e:
      raise CommandError("Error importing organizations: %s" % e)


  # Importing customers
  #   - extracting recently changed BusinessPartner objects
  #   - meeting the criterion:
  #        - %active = True
  #        - %customer = True
  #   - mapped fields openbravo -> frePPLe customer
  #        - %searchKey %name -> name
  #        - %description -> description
  #        - %id -> source
  #        - 'openbravo' -> subcategory

  def import_customers(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'BusinessPartner':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        if not organization:
          continue
        searchkey = elem.find("searchKey").text
        name = elem.find("name").text
        unique_name = u'%s %s' % (searchkey, name)
        objectid = elem.get('id')
        description = elem.find("description").text
        if description: description = description[0:500]
        self.customers[objectid] = unique_name
        if unique_name in frepple_keys:
          update.append( (description, objectid, unique_name) )
        else:
          insert.append( (description, unique_name, objectid) )
          frepple_keys[unique_name] = objectid
        unused_keys.pop(unique_name, None)
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing customers...")

      # Get existing records
      cursor.execute("SELECT name, subcategory, source FROM customer")
      frepple_keys = {}
      for i in cursor.fetchall():
        if i[1] == 'openbravo':
          frepple_keys[i[0]] = i[2]
        else:
          frepple_keys[i[0]] = None
      unused_keys = frepple_keys.copy()

        # Retrieve businesspartners - customers
      insert = []
      update = []
      if self.incremental:
        query = urllib.parse.quote("customer=true and updated>'%s'" % self.delta)
      else:
        query = urllib.parse.quote("customer=true")
      self.get_data("/openbravo/ws/dal/BusinessPartner?where=%s&orderBy=name&includeChildren=false" % query, parse)

      # Create records
      cursor.executemany(
        "insert into customer \
          (description,name,source,subcategory,lastmodified) \
          values (%%s,%%s,%%s,'openbravo','%s')" % self.date,
        insert
        )

      # Update records
      cursor.executemany(
        "update customer \
          set description=%%s,source=%%s,subcategory='openbravo',lastmodified='%s' \
          where name=%%s" % self.date,
        update
        )

      # Delete records
      if not self.incremental:
        delete = [ (i,) for i, j in unused_keys.items() if j ]
        cursor.executemany(
          'update customer set owner_id=null where owner_id=%s',
          delete
          )
        cursor.executemany(
          'update demand set customer_id=null where customer_id=%s',
          delete
          )
        cursor.executemany(
          'delete from customer where name=%s',
          delete
          )

      if self.verbosity > 0:
        print("Inserted %d new customers" % len(insert))
        print("Updated %d existing customers" % len(update))
        if not self.incremental:
          print("Deleted %d customers" % len(delete))
        print("Imported customers in %.2f seconds" % (time() - starttime))


  # Importing suppliers
  #   - extracting recently changed BusinessPartner objects
  #   - meeting the criterion:
  #        - %active = True
  #        - %vendor = True
  #   - mapped fields openbravo -> frePPLe supplier
  #        - %searchKey %name -> name
  #        - %description -> description
  #        - %id -> source
  #        - 'openbravo' -> subcategory
  def import_suppliers(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'BusinessPartner':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        if not organization:
          continue
        searchkey = elem.find("searchKey").text
        name = elem.find("name").text
        unique_name = u'%s %s' % (searchkey, name)
        objectid = elem.get('id')
        description = elem.find("description").text
        if description: description = description[0:500]
        self.suppliers[objectid] = unique_name
        if unique_name in frepple_keys:
          update.append( (description, objectid, unique_name) )
        else:
          insert.append( (description, unique_name, objectid) )
          frepple_keys[unique_name] = objectid
        unused_keys.pop(unique_name, None)
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing suppliers...")

      # Get existing records
      cursor.execute("SELECT name, subcategory, source FROM supplier")
      frepple_keys = {}
      for i in cursor.fetchall():
        if i[1] == 'openbravo':
          frepple_keys[i[0]] = i[2]
        else:
          frepple_keys[i[0]] = None
      unused_keys = frepple_keys.copy()

      # Retrieve businesspartners - suppliers
      insert = []
      update = []
      if self.incremental:
        query = urllib.parse.quote("vendor=true and updated>'%s'" % self.delta)
      else:
        query = urllib.parse.quote("vendor=true")
      self.get_data("/openbravo/ws/dal/BusinessPartner?where=%s&orderBy=name&includeChildren=false" % query, parse)

      # Create records
      cursor.executemany(
        "insert into supplier \
          (description,name,source,subcategory,lastmodified) \
          values (%%s,%%s,%%s,'openbravo','%s')" % self.date,
        insert
        )

      # Update records
      cursor.executemany(
        "update supplier \
          set description=%%s,source=%%s,subcategory='openbravo',lastmodified='%s' \
          where name=%%s" % self.date,
        update
        )

      # Delete records
      if not self.incremental:
        delete = [ (i,) for i, j in unused_keys.items() if j ]
        cursor.executemany(
          'update supplier set owner_id=null where owner_id=%s',
          delete
          )
        cursor.executemany(
          'update purchase_order set supplier_id=null where supplier_id=%s',
          delete
          )
        cursor.executemany(
          'delete from itemsupplier where supplier_id=%s',
          delete
          )
        cursor.executemany(
          'delete from supplier where name=%s',
          delete
          )

      if self.verbosity > 0:
        print("Inserted %d new suppliers" % len(insert))
        print("Updated %d existing suppliers" % len(update))
        if not self.incremental:
          print("Deleted %d suppliers" % len(delete))
        print("Imported suppliers in %.2f seconds" % (time() - starttime))


  # Importing products
  #   - extracting recently changed Product objects
  #   - meeting the criterion:
  #        - %active = True
  #   - mapped fields openbravo -> frePPLe item
  #        - %searchKey %name -> name
  #        - %description -> description
  #        - %searchKey -> source
  #        - 'openbravo' -> subcategory

  def import_products(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'Product':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        if not organization:
          continue
        searchkey = elem.find("searchKey").text
        name = elem.find("name").text
        # A product name which consists of the searchkey and the name fields
        # is the default.
        # If you want a shorter item name, use the following lines instead:
        # unique_name = searchkey
        # description = name
        unique_name = u'%s %s' % (searchkey, name)
        description = elem.find("description").text
        if description: description = description[0:500]
        objectid = elem.get('id')
        self.items[objectid] = unique_name
        unused_keys.pop(unique_name, None)
        if unique_name in frepple_keys:
          update.append( (description, objectid, unique_name) )
        else:
          insert.append( (unique_name, description, objectid) )
          frepple_keys[unique_name] = objectid
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing products...")

      # Get existing items
      cursor.execute("SELECT name, subcategory, source FROM item")
      frepple_keys = {}
      for i in cursor.fetchall():
        if i[1] == 'openbravo':
          frepple_keys[i[0]] = i[2]
        else:
          frepple_keys[i[0]] = None
      unused_keys = frepple_keys.copy()

      # Get all items from Openbravo
      insert = []
      update = []
      delete = []
      if self.incremental:
        query = urllib.parse.quote("updated>'%s'" % self.delta)
        self.get_data("/openbravo/ws/dal/Product?where=%s&orderBy=name&includeChildren=false" % query, parse)
      else:
        self.get_data("/openbravo/ws/dal/Product?orderBy=name&includeChildren=false", parse)

      # Create new items
      cursor.executemany(
        "insert into item \
          (name,description,subcategory,source,lastmodified) \
          values (%%s,%%s,'openbravo',%%s,'%s')" % self.date,
        insert
        )

      # Update existing items
      cursor.executemany(
        "update item \
          set description=%%s, subcategory='openbravo', source=%%s, lastmodified='%s' \
          where name=%%s" % self.date,
        update
        )

      # Delete inactive items
      if not self.incremental:
        delete = [ (i,) for i, j in unused_keys.items() if j ]
        cursor.executemany("delete from demand where item_id=%s", delete)
        cursor.executemany(
          "delete from flow \
          where thebuffer_id in (select name from buffer where item_id=%s)",
          delete
          )
        cursor.executemany("delete from buffer where item_id=%s", delete)
        cursor.executemany("delete from item where name=%s", delete)

      if self.verbosity > 0:
        print("Inserted %d new products" % len(insert))
        print("Updated %d existing products" % len(update))
        if not self.incremental:
          print("Deleted %d products" % len(delete))
        print("Imported products in %.2f seconds" % (time() - starttime))


  # Importing locations
  #   - extracting warehouse objects
  #   - We're also trying to create a mapping between an organization and its main
  #     warehouse location. This mapping is used to map manufacturing work requirements
  #     to a location where it'll happen.
  #     Manufacturing work requirements are linked with an organization in Openbravo,
  #     not with a warehouse.
  #     The mapping organization - warehouse we're making here is not a strict and
  #     foolproof way.
  #   - meeting the criterion:
  #        - %active = True
  #   - mapped fields Openbravo -> frePPLe location
  #        - %searchKey %name -> name
  #        - %searchKey -> category
  #        - 'openbravo' -> source

  def import_locations(self, cursor):

    def parse1(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'Warehouse':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        if not organization:
          continue
        searchkey = elem.find("searchKey").text
        name = elem.find("name").text
        # A product name which consists of the searchkey field is the default.
        # If you want a longer more descriptive item name, use the following lines instead
        # unique_name = u'%s %s' % (searchkey, name)
        # description = elem.find("description").text[0:500]
        unique_name = searchkey
        description = name
        objectid = elem.get('id')
        self.locations[objectid] = unique_name
        locations.append( (description, objectid, unique_name) )
        self.locations[objectid] = unique_name
        unused_keys.pop(unique_name, None)
        if organization in self.organization_location:
          print(
            "Warning: Organization '%s' is already associated with '%s'. Ignoring association with '%s'"
            % (organization, self.organization_location[organization], unique_name)
            )
        else:
          self.organization_location[organization] = unique_name

        # Clean the XML hierarchy
        root.clear()
      return records

    def parse2(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'Locator':
          continue
        records += 1
        warehouse = elem.find("warehouse").get('id')
        objectid = elem.get('id')
        self.locators[objectid] = warehouse
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing locations...")

      # Get existing locations
      cursor.execute("SELECT name, subcategory, source FROM location")
      frepple_keys = {}
      for i in cursor.fetchall():
        if i[1] == 'openbravo':
          frepple_keys[i[0]] = i[2]
        else:
          frepple_keys[i[0]] = None
      unused_keys = frepple_keys.copy()

      # Get locations
      locations = []
      query = urllib.parse.quote("active=true")
      self.get_data("/openbravo/ws/dal/Warehouse?where=%s&orderBy=name&includeChildren=false" % query, parse1)

      # Remove deleted or inactive locations
      delete = [ (i,) for i, j in unused_keys.items() if j ]
      cursor.executemany(
        "update buffer \
        set location_id=null \
        where location_id=%s",
        delete
        )
      cursor.executemany(
        "update resource \
        set location_id=null \
        where location_id=%s",
        delete
        )
      cursor.executemany(
        "update location \
        set owner_id=null \
        where owner_id=%s",
        delete
        )

      # Create or update locations
      cursor.executemany(
        "insert into location \
          (description, source, subcategory, name, lastmodified) \
          values (%%s,%%s,'openbravo',%%s,'%s')" % self.date,
        [ i for i in locations if i[2] not in frepple_keys ]
        )
      cursor.executemany(
        "update location \
          set description=%%s, subcategory='openbravo', source=%%s, lastmodified='%s' \
          where name=%%s" % self.date,
        [ i for i in locations if i[2] not in frepple_keys ]
        )

      # Get a mapping of all locators to their warehouse
      self.get_data("/openbravo/ws/dal/Locator", parse2)

      if self.verbosity > 0:
        print("Processed %d locations" % len(locations))
        print("Deleted %d locations" % len(delete))
        print("Imported locations in %.2f seconds" % (time() - starttime))


  # Importing sales orders
  #   - Extracting recently changed orderline objects
  #   - meeting the criterion:
  #        - %salesOrder.salesTransaction = true
  #   - mapped fields openbravo -> frePPLe delivery operation
  #        - 'Ship %product_searchkey %product_name @ %warehouse' -> name
  #   - mapped fields openbravo -> frePPLe buffer
  #        - '%product_searchkey %product_name @ %warehouse' -> name
  #   - mapped fields openbravo -> frePPLe delivery flow
  #        - 'Ship %product_searchkey %product_name @ %warehouse' -> operation
  #        - '%product_searchkey %product_name @ %warehouse' -> buffer
  #        - quantity -> -1
  #        -  'start' -> type
  #   - mapped fields openbravo -> frePPLe demand
  #        - %organization_searchkey %salesorder.documentno %lineNo -> name
  #        - if (%orderedQuantity > %deliveredQuantity, %orderedQuantity - %deliveredQuantity, %orderedQuantity) -> quantity
  #        - if (%orderedQuantity > %deliveredQuantity, 'open', 'closed') -> status
  #        - %sol_product_id -> item
  #        - %businessParnter.searchKey %businessParnter.name -> customer
  #        - %scheduledDeliveryDate -> due
  #          Note that the field scheduledDeliveryDate on the order line is hidden by default.
  #          Only the field scheduledDeliveryDate on the order is visible by default.
  #        - 'openbravo' -> source
  #        - 1 -> priority
  # We assume that a lineNo is unique within an order. However, this is not enforced by Openbravo!

  def import_salesorders(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'OrderLine':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        product = self.items.get(elem.find("product").get('id'), None)
        warehouse = self.locations.get(elem.find("warehouse").get('id'), None)
        businessPartner = self.customers.get(elem.find("businessPartner").get('id'), None)
        if not warehouse or not product or not businessPartner or not organization:
          # Product, customer, location or organization are not known in frePPLe.
          # We assume that in that case you don't need to the demand either.
          root.clear()
          continue
        objectid = elem.get('id')
        documentno = elem.find("salesOrder").get('identifier').split()[0]
        lineNo = elem.find("lineNo").text
        unique_name = "%s %s %s" % (organization, documentno, lineNo)
        tmp = elem.find("scheduledDeliveryDate").text
        if tmp:
          scheduledDeliveryDate = datetime.strptime(tmp, '%Y-%m-%dT%H:%M:%S.%fZ')
        else:
          root.clear()
          continue
        tmp = elem.find("orderedQuantity").text
        orderedQuantity = tmp and float(tmp) or 0
        tmp = elem.find("deliveredQuantity").text
        deliveredQuantity = tmp and float(tmp) or 0
        closed = deliveredQuantity >= orderedQuantity   # TODO Not the right criterion
        operation = u'Ship %s @ %s' % (product, warehouse)
        deliveries.update([
          (product, warehouse, operation, u'%s @ %s' % (product, warehouse))
          ])
        if unique_name in frepple_keys:
          update.append((
            objectid, closed and orderedQuantity or orderedQuantity - deliveredQuantity,
            product, closed and 'closed' or 'open', scheduledDeliveryDate,
            businessPartner, operation, unique_name
            ))
        else:
          insert.append((
            objectid, closed and orderedQuantity or orderedQuantity - deliveredQuantity,
            product, closed and 'closed' or 'open', scheduledDeliveryDate,
            businessPartner, operation, unique_name
            ))
          frepple_keys.add(unique_name)
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      deliveries = set()
      if self.verbosity > 0:
        print("Importing sales orders...")

      # Get the list of known demands in frePPLe
      cursor.execute("SELECT name FROM demand")
      frepple_keys = set([ i[0] for i in cursor.fetchall() ])

      # Get the list of sales order lines
      insert = []
      update = []
      deliveries = set()
      if self.incremental:
        query = urllib.parse.quote("(updated>'%s' or salesOrder.updated>'%s') and salesOrder.salesTransaction=true and (salesOrder.documentStatus='CO' or salesOrder.documentStatus='CL')" % (self.delta, self.delta))
      else:
        query = urllib.parse.quote("salesOrder.salesTransaction=true and (salesOrder.documentStatus='CO' or salesOrder.documentStatus='CL')")
      self.get_data("/openbravo/ws/dal/OrderLine?where=%s&orderBy=salesOrder.creationDate&includeChildren=false" % query, parse)

      # Create or update delivery operations
      cursor.execute("SELECT name FROM operation where name like 'Ship %'")
      frepple_keys = set([ i[0] for i in cursor.fetchall()])
      cursor.executemany(
        "insert into operation \
          (name,location_id,subcategory,type,lastmodified) \
          values (%%s,%%s,'openbravo','fixed_time','%s')" % self.date,
        [ (i[2], i[1]) for i in deliveries if i[2] not in frepple_keys ])
      cursor.executemany(
        "update operation \
          set location_id=%%s, subcategory='openbravo', type='fixed_time', lastmodified='%s' where name=%%s" % self.date,
        [ (i[1], i[2]) for i in deliveries if i[2] in frepple_keys ])

      # Create or update delivery buffers
      cursor.execute("SELECT name FROM buffer")
      frepple_keys = set([ i[0] for i in cursor.fetchall()])
      cursor.executemany(
        "insert into buffer \
          (name,item_id,location_id,subcategory,lastmodified) \
          values(%%s,%%s,%%s,'openbravo','%s')" % self.date,
        [ (i[3], i[0], i[1]) for i in deliveries if i[3] not in frepple_keys ])
      cursor.executemany(
        "update buffer \
          set item_id=%%s, location_id=%%s, subcategory='openbravo', lastmodified='%s' where name=%%s" % self.date,
        [ (i[0], i[1], i[3]) for i in deliveries if i[3] in frepple_keys ])

      # Create or update flow on delivery operation
      cursor.execute("SELECT operation_id, thebuffer_id FROM flow")
      frepple_keys = set([ i for i in cursor.fetchall()])
      cursor.executemany(
        "insert into flow \
          (operation_id,thebuffer_id,quantity,type,source,lastmodified) \
          values(%%s,%%s,-1,'start','openbravo','%s')" % self.date,
        [ (i[2], i[3]) for i in deliveries if (i[2], i[3]) not in frepple_keys ])
      cursor.executemany(
        "update flow \
          set quantity=-1, type='start', source='openbravo', lastmodified='%s' where operation_id=%%s and thebuffer_id=%%s" % self.date,
        [ (i[2], i[3]) for i in deliveries if (i[2], i[3]) in frepple_keys ])

      # Create or update demands
      cursor.executemany(
        "insert into demand \
          (source, quantity, item_id, status, subcategory, due, customer_id, operation_id, name, priority, lastmodified) \
          values (%%s,%%s,%%s,%%s,'openbravo',%%s,%%s,%%s,%%s,0,'%s')" % self.date,
        insert
        )
      cursor.executemany(
        "update demand \
          set source=%%s, quantity=%%s, item_id=%%s, status=%%s, subcategory='openbravo', \
              due=%%s, customer_id=%%s, operation_id=%%s, lastmodified='%s' \
          where name=%%s" % self.date,
        update
        )

      if self.verbosity > 0:
        print("Created or updated %d delivery operations" % len(deliveries))
        print("Inserted %d new sales order lines" % len(insert))
        print("Updated %d existing sales order lines" % len(update))
        print("Imported sales orders in %.2f seconds" % (time() - starttime))


  # Importing machines
  #   - extracting recently changed ManufacturingMachine objects
  #   - meeting the criterion:
  #        - %active = True
  #   - mapped fields openbravo -> frePPLe resource
  #        - %id %name -> name
  #        - %cost_hour -> cost
  #        - capacity_per_cycle -> maximum
  #        - 'openbravo' -> subcategory
  #   - Manufacturing machines are associated with any location in Openbravo.
  #     You should assign a location in frePPLe to assure that the user interface
  #     working.
  #

  def import_machines(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'ManufacturingMachine':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        if not organization:
          continue
        unique_name = elem.get('identifier')
        objectid = elem.get('id')
        if unique_name in frepple_keys:
          update.append( (objectid, unique_name) )
        else:
          insert.append( (unique_name, objectid) )
          frepple_keys.add(unique_name)
        unused_keys.discard(unique_name)
        self.resources[objectid] = unique_name
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing machines...")
      cursor.execute("SELECT name, subcategory, source FROM resource")
      frepple_keys = set()
      for i in cursor.fetchall():
        if i[1] == 'openbravo':
          self.resources[i[2]] = i[0]
        frepple_keys.add(i[0])
      unused_keys = frepple_keys.copy()
      insert = []
      update = []
      self.get_data("/openbravo/ws/dal/ManufacturingMachine?orderBy=name&includeChildren=false", parse)

      cursor.executemany(
        "insert into resource \
          (name,source,subcategory,lastmodified) \
          values (%%s,%%s,'openbravo','%s')" % self.date,
        insert
        )
      cursor.executemany(
        "update resource \
          set source=%%s,subcategory='openbravo',lastmodified='%s' \
          where name=%%s" % self.date,
        update
        )
      delete = [ (i,) for i in unused_keys ]
      cursor.executemany('delete from resourceskill where resource_id=%s', delete)
      cursor.executemany('delete from resourceload where resource_id=%s', delete)
      cursor.executemany('update resource set owner_id where owner_id=%s', delete)
      cursor.executemany('delete from resource where name=%s', delete)
      if self.verbosity > 0:
        print("Inserted %d new machines" % len(insert))
        print("Updated %d existing machines" % len(update))
        print("Deleted %d machines" % len(delete))
        print("Imported machines in %.2f seconds" % (time() - starttime))


  # Importing onhand
  #   - extracting all MaterialMgmtStorageDetail objects
  #   - No filtering for latest changes, ie always complete extract
  #   - meeting the criterion:
  #        - %qty > 0
  #        - Location already mapped in frePPLe
  #        - Product already mapped in frePPLe
  #   - mapped fields openbravo -> frePPLe buffer
  #        - %product @ %locator.warehouse -> name
  #        - %product -> item_id
  #        - %locator.warehouse -> location_id
  #        - %qty -> onhand
  #        - 'openbravo' -> subcategory

  def import_onhand(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'MaterialMgmtStorageDetail':
          continue
        records += 1
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        if not organization:
          continue
        onhand = elem.find("quantityOnHand").text
        locator = elem.find("storageBin").get('id')
        product = elem.find("product").get('id')
        item = self.items.get(product, None)
        location = self.locations.get(self.locators[locator], None)
        if not item or not location:
          root.clear()
          continue
        buffer_name = "%s @ %s" % (item, location)
        if buffer_name in frepple_buffers:
          if frepple_buffers[buffer_name] == 'openbravo':
            # Existing buffer marked as openbravo buffer
            increment.append( (onhand, buffer_name) )
          else:
            # Existing buffer marked as a non-openbravo buffer
            update.append( (onhand, buffer_name) )
            frepple_buffers[buffer_name] = 'openbravo'
        elif item and location:
          # New buffer
          insert.append( (buffer_name, self.items[product], self.locations[self.locators[locator]], onhand) )
          frepple_buffers[buffer_name] = 'openbravo'
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing onhand...")

      # Get the list of all current frepple records
      cursor.execute("SELECT name, subcategory FROM buffer")
      frepple_buffers = {}
      for i in cursor.fetchall():
        frepple_buffers[i[0]] = i[1]

      # Reset stock levels in all openbravo buffers
      cursor.execute("UPDATE buffer SET onhand=0, lastmodified='%s' WHERE subcategory='openbravo'" % self.date )

      # Get all stock values. NO incremental load here!
      insert = []
      increment = []
      update = []
      query = urllib.parse.quote("quantityOnHand>0")
      self.get_data("/openbravo/ws/dal/MaterialMgmtStorageDetail?where=%s" % query, parse)

      cursor.executemany(
        "insert into buffer \
          (name,item_id,location_id,onhand,subcategory,lastmodified) \
          values(%%s,%%s,%%s,%%s,'openbravo','%s')" % self.date,
        insert)
      cursor.executemany(
        "update buffer \
          set onhand=%%s, subcategory='openbravo', lastmodified='%s' \
          where name=%%s" % self.date,
        update
        )
      cursor.executemany(
        "update buffer \
          set onhand=%s+onhand \
          where name=%s",
        increment
        )
      if self.verbosity > 0:
        print("Inserted onhand for %d new buffers" % len(insert))
        print("Updated onhand for %d existing buffers" % len(update))
        print("Incremented onhand %d times for existing buffers" % len(increment))
        print("Imported onhand in %.2f seconds" % (time() - starttime))


  # Load itemsupplier with data
  #   - exctracting data from approvedvendor records
  #   - meeting the criterion:
  #        - %discontinued = false
  #        - %currentVendor = true
  #   - mapped fields openbravo -> frePPLe itemsupplier
  #        - 'product id' ->item_id
  #        - 'businessPartner id' -> supplier_id
  #        - 'purchasingLeadTime' -> leadtime
  #        - 'minimumOrderQty' ->sizeminimum
  #        - 'quantityPerPackage' -> sizemultiple
  #        - 'listPrice' -> cost
  #        - 'currentVendor' -> priority (1 if true, 2 otherwise)
  #        - 'creationDate'  -> effective_start
  #        - 'endDate' -> effective_end
  #        - 'organization id' -> location_id
  #        - 'openbravo' -> source

  def import_itemsupplier(self, cursor):

    def parse(conn):
      global prevproduct
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'ApprovedVendor':
          continue

        records += 1
        objectid = elem.get('id')
        item_id = self.items.get(elem.find('product').get('id'), None)
        if not item_id:
          root.clear()
          continue
        supplier_id = elem.find('businessPartner').get('identifier')[:300]
        leadtime = elem.find("purchasingLeadTime").text
        sizeminimum = elem.find("minimumOrderQty").text
        sizemultiple = elem.find("quantityPerPackage").text
        cost = elem.find("listPrice").text
        vendoritemname = elem.find("VendorProductNo").text

        priority = elem.find("currentVendor").text
        if priority:
          priority = 1
        else:
          priority = 2

        effective_end = elem.find("discontinuedBy").text
        if effective_end:
          effective_end = datetime.strptime(effective_end, '%Y-%m-%dT%H:%M:%S.%fZ')

        organization = self.organizations.get(elem.find("organization").get("id"), None)
        location_id=""         #openbravo does not have location at this moment
        source="openbravo"

        key = (item_id, supplier_id, None)
        unused_keys.discard(key)
        if key in frepple_keys:
          update.append( (vendoritemname, source, leadtime, sizeminimum, sizemultiple, cost, priority, effective_end, item_id, location_id, supplier_id) )
        else:
          insert.append( (vendoritemname, source, leadtime, sizeminimum, sizemultiple, cost, priority, effective_end, item_id, location_id, supplier_id) )

        # Clean the XML hierarchy
        root.clear()
      return records

    global prevproduct
    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing approved vendors...")
        cursor.execute("SELECT DISTINCT item_id, supplier_id, location_id FROM itemsupplier")
        frepple_keys=set()
      for i in cursor.fetchall():
            frepple_keys.add( ( i[0], i[1], i[2] ) )
      unused_keys = frepple_keys.copy()

      insert = []
      update = []
      if self.incremental:
        query = urllib.parse.quote("active=true and discontinued=false and updated>'%s'" % self.delta)
      else:
        query = urllib.parse.quote("active=true and discontinued=false")
      prevproduct = None
      self.get_data("/openbravo/ws/dal/ApprovedVendor?where=%s&orderBy=product&includeChildren=false" % query, parse)

      if not self.incremental:
        cursor.executemany(
            "delete from itemsupplier \
            where item_id=%s and supplier_id=%s and location_id=%s",
            [ i for i in unused_keys ]
          )

      # Create or update purchasing operations
      cursor.executemany(
          "insert into itemsupplier \
            (vendoritemname, source, leadtime, sizeminimum, sizemultiple, cost, priority, effective_end, item_id, location_id, supplier_id, lastmodified) \
            values (%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,'%%s',%s)" % self.date,
          insert
        )
      cursor.executemany(
          "update itemsupplier \
            set vendoritemname=%%s, source=%%s, leadtime=%%s, sizeminimum=%%s, sizemultiple=%%s, cost=%%s, priority=%%s, effective_end=%%s,\
             lastmodified='%s' where item_id=%%s and location_id=%%s and supplier_id=%%s" % self.date,
          update
        )

      if self.verbosity > 0:
          print("Inserted %d new approved vendors" % len(insert))
          print("Updated %d existing approved vendors" % len(update))
          if not self.incremental:
            print("Deleted %d approved vendors" % len(unused_keys))
          print("Populated approved vendors in %.2f seconds" % (time() - starttime))


  # Load open purchase orders
  #   - extracting recently changed orderline objects
  #   - meeting the criterion:
  #        - %product already exists in frePPLe
  #        - %warehouse already exists in frePPLe
  #   - mapped fields openbravo -> frePPLe buffer
  #        - %product @ %warehouse -> name
  #        - %warehouse -> location
  #        - %product -> item
  #        - 'openbravo' -> subcategory
  #   - mapped fields openbravo -> frePPLe operation
  #        - 'Purchase ' %product ' @ ' %warehouse -> name
  #        - 'fixed_time' -> type
  #        - 'openbravo' -> subcategory
  #   - mapped fields openbravo -> frePPLe flow
  #        - 'Purchase ' %product ' @ ' %warehouse -> operation
  #        - %product ' @ ' %warehouse -> buffer
  #        - 1 -> quantity
  #        - 'end' -> type
  #   - mapped fields openbravo -> frePPLe operationplan
  #        - %documentNo -> identifier
  #        - 'Purchase ' %product ' @ ' %warehouse -> operation
  #        - %orderedQuantity - %deliveredQuantity -> quantity
  #        - %creationDate -> startdate
  #        - %scheduledDeliveryDate -> enddate
  #        - 'openbravo' -> source

  def import_purchaseorders(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'OrderLine':
          continue
        records += 1
        product = self.items.get(elem.find("product").get('id'), None)
        warehouse = self.locations.get(elem.find("warehouse").get('id'), None)
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        scheduledDeliveryDate = elem.find("scheduledDeliveryDate").text
        if not warehouse or not product or not organization or not scheduledDeliveryDate:
          # Product, location or organization are not known in frePPLe.
          # Or there is no scheduled delivery date.
          # We assume that in that case you don't need to the purchase order either.
          root.clear()
          continue
        objectid = elem.get('id')
        scheduledDeliveryDate = datetime.strptime(scheduledDeliveryDate, '%Y-%m-%dT%H:%M:%S.%fZ')
        creationDate = datetime.strptime(elem.find("creationDate").text, '%Y-%m-%dT%H:%M:%S.%fZ')
        orderedQuantity = float(elem.find("orderedQuantity").text or 0)
        deliveredQuantity = float(elem.find("deliveredQuantity").text or 0)
        operation = u'Purchase %s @ %s' % (product, warehouse)
        deliveries.update([
          (product, warehouse, operation, u'%s @ %s' % (product, warehouse))
          ])
        if objectid in frepple_keys:
          if deliveredQuantity >= orderedQuantity:   # TODO Not the right criterion
            delete.append( (objectid,) )
          else:
            update.append((
              operation, orderedQuantity - deliveredQuantity,
              creationDate, scheduledDeliveryDate, objectid
              ))
        else:
          self.idcounter += 1
          insert.append((
            self.idcounter, operation, orderedQuantity - deliveredQuantity,
            creationDate, scheduledDeliveryDate, objectid
            ))
          frepple_keys.add(objectid)
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing purchase orders...")

      # Find all known operationplans in frePPLe
      cursor.execute("SELECT source \
         FROM operationplan \
         where source is not null \
           and operation_id like 'Purchase %'")
      frepple_keys = set([ i[0] for i in cursor.fetchall()])

      # Get the list of all open purchase orders
      insert = []
      update = []
      delete = []
      deliveries = set()
      query = urllib.parse.quote("updated>'%s' and salesOrder.salesTransaction=false and salesOrder.documentType.name<>'RTV Order'" % self.delta)
      self.get_data("/openbravo/ws/dal/OrderLine?where=%s&orderBy=salesOrder.creationDate&includeChildren=false" % query, parse)

      # Create or update procurement operations
      cursor.execute("SELECT name FROM operation where name like 'Purchase %'")
      frepple_keys = set([ i[0] for i in cursor.fetchall()])
      cursor.executemany(
        "insert into operation \
          (name,location_id,subcategory,type,lastmodified) \
          values (%%s,%%s,'openbravo','fixed_time','%s')" % self.date,
        [ (i[2], i[1]) for i in deliveries if i[2] not in frepple_keys ])
      cursor.executemany(
        "update operation \
          set location_id=%%s, subcategory='openbravo', type='fixed_time', lastmodified='%s' where name=%%s" % self.date,
        [ (i[1], i[2]) for i in deliveries if i[2] in frepple_keys ])

      # Create or update purchasing buffers
      cursor.execute("SELECT name FROM buffer")
      frepple_keys = set([ i[0] for i in cursor.fetchall()])
      cursor.executemany(
        "insert into buffer \
          (name,item_id,location_id,subcategory,lastmodified) \
          values(%%s,%%s,%%s,'openbravo','%s')" % self.date,
        [ (i[3], i[0], i[1]) for i in deliveries if i[3] not in frepple_keys ])
      cursor.executemany(
        "update buffer \
          set item_id=%%s, location_id=%%s, subcategory='openbravo', lastmodified='%s' where name=%%s" % self.date,
        [ (i[0], i[1], i[3]) for i in deliveries if i[3] in frepple_keys ])

      # Create or update flow on purchasing operation
      cursor.execute("SELECT operation_id, thebuffer_id FROM flow")
      frepple_keys = set([ i for i in cursor.fetchall()])
      cursor.executemany(
        "insert into flow \
          (operation_id,thebuffer_id,quantity,type,source,lastmodified) \
          values(%%s,%%s,1,'end','openbravo','%s')" % self.date,
        [ (i[2], i[3]) for i in deliveries if (i[2], i[3]) not in frepple_keys ])
      cursor.executemany(
        "update flow \
          set quantity=1, type='end', source='openbravo', lastmodified='%s' where operation_id=%%s and thebuffer_id=%%s" % self.date,
        [ (i[2], i[3]) for i in deliveries if (i[2], i[3]) in frepple_keys ])

      # Create purchasing operationplans
      cursor.executemany(
        "insert into operationplan \
            (id,operation_id,quantity,startdate,enddate,status,source,lastmodified) \
            values(%%s,%%s,%%s,%%s,%%s,'confirmed',%%s,'%s')" % self.date,
        insert
        )
      cursor.executemany(
        "update operationplan \
            set operation_id=%%s, quantity=%%s, startdate=%%s, enddate=%%s, status='confirmed', lastmodified='%s' \
          where source=%%s" % self.date,
        update)
      cursor.executemany(
        "delete from operationplan where source=%s",
        delete)

      if self.verbosity > 0:
        print("Inserted %d purchase order lines" % len(insert))
        print("Updated %d purchase order lines" % len(update))
        print("Deleted %d purchase order lines" % len(delete))
        print("Imported purchase orders in %.2f seconds" % (time() - starttime))


  def import_purchasingplan(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end':
          continue
        elif elem.tag == 'MRPPurchasingRun':
          # Clean the XML hierarchy
          root.clear()
          continue
        elif elem.tag != 'MRPPurchasingRunLine':
          continue
        records += 1
        product = self.items.get(elem.find("product").get('id'), None)
        # warehouse = self.locations.get(elem.find("warehouse").get('id'), None)
        warehouse = 'Main location'   # TODO: purchasing plan has no concept of the location
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        plannedDate = datetime.strptime(elem.find("plannedDate"), '%Y-%m-%dT%H:%M:%S.%fZ')
        plannedOrderDate = datetime.strptime(elem.find("plannedDate"), '%Y-%m-%dT%H:%M:%S.%fZ')
        businessPartner = self.suppliers(elem.find("businessPartner").get("id"), None)
        name = elem.find("purchasingPlan").get('identifier')
        if not warehouse or not product or not organization or not plannedDate:
          # Product, location or organization are not known in frePPLe.
          # Or there is no scheduled delivery date.
          # We assume that in that case you don't need to the purchase order either.
          continue
        objectid = elem.get('id')
        requiredQuantity = float(elem.find("requiredQuantity").text or 0)
        transactionType = elem.find("transactionType").text
        if transactionType == 'PO' and businessPartner:
          # Purchase order
          unused_keys_po.discard(objectid)
          if objectid in frepple_keys_po:
            update_po.append((
              name, product, warehouse, businessPartner, requiredQuantity,
              plannedDate, plannedOrderDate, objectid
              ))
          else:
            self.idcounter += 1
            insert_po.append((
              self.idcounter, name, product, warehouse, businessPartner,
              requiredQuantity, plannedDate, plannedOrderDate, objectid
              ))
            frepple_keys_po.add(objectid)
        elif transactionType == 'MS':
          # Distribution order
          unused_keys_do.discard(objectid)
          if objectid in frepple_keys_do:
            update_do.append((
              name, product, warehouse, requiredQuantity,
              plannedDate, objectid
              ))
          else:
            self.idcounter += 1
            insert_do.append((
              self.idcounter, name, product, warehouse, requiredQuantity,
              plannedDate, objectid
              ))
            frepple_keys_do.add(objectid)
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing purchasing plan...")

      # Collect existing purchase orders and distribution orders
      cursor.execute("select source from purchase_order where status='approved'")
      frepple_keys_po = set([ i for i in cursor.fetchall()])
      unused_keys_po = frepple_keys_po.copy()
      cursor.execute("select source from distribution_order where status='approved'")
      frepple_keys_do = set([ i for i in cursor.fetchall()])
      unused_keys_do = frepple_keys_do.copy()

      # Process the input
      insert_po = []
      update_po = []
      insert_do = []
      update_do = []
      query = urllib.parse.quote("name like 'FREPPLE%' and description like 'Incremental export triggered by %'" % self.delta)
      self.get_data("/openbravo/ws/dal/MRPPurchasingRun?where=%s" % query, parse)

      # Create or update purchase orders
      cursor.executemany(
        "insert into purchase_order \
            (id,reference,item_id,location_id,supplier_id,quantity,enddate,startdate,status,lastmodified) \
            values(%%s,%%s,%%s,%%s,%%s,%%s,%%s,%%s,'confirmed''%s')" % self.date,
        insert_po
        )
      cursor.executemany(
        "update purchase_order \
            set reference=%%s, item_id=%%s, location_id=%%s, supplier_id=%%s, quantity=%%s, \
            enddate=%%s, startdate=%%s, status='approved', lastmodified='%s' \
          where source=%%s" % self.date,
        update_po
        )
      cursor.executemany(
        "delete from purchase_order where source=%s",
        unused_keys_po
        )

      # Create or update distribution orders
      cursor.executemany(
        "insert into distribution_order \
            (id,reference,item_id,destination_id,quantity,enddate,source,status,consume_material,lastmodified) \
            values(%%s,%%s,%%s,%%s,%%s,%%s,%%s,'approved',false,'%s')" % self.date,
        insert_do
        )
      cursor.executemany(
        "update distribution_order \
            set reference=%%s, item_id=%%s, destination_id=%%s, quantity=%%s, enddate=%s, status='approved', \
            consume_material=false, lastmodified='%s'  \
          where source=%%s" % self.date,
        update_do
        )
      cursor.executemany(
        "delete from distribution_order where source=%s",
        unused_keys_do
        )

      if self.verbosity > 0:
        print("Inserted %d approved purchasing plan lines" % (len(insert_po) + len(insert_do)))
        print("Updated %d approved purchasing plan lines" % (len(update_po) + len(update_do)))
        print("Deleted %d approved purchasing plan lines" % (len(unused_keys_po) + len(unused_keys_do)))
        print("Imported approved purchasing plan lines in %.2f seconds" % (time() - starttime))


  # Load work in progress operationplans
  #   - extracting manufacturingWorkRequirement objects
  #   - meeting the criterion:
  #        - %operation already exists in frePPLe
  #   - mapped fields openbravo -> frePPLe operationplan
  #        - %product @ %warehouse -> name
  #        - %warehouse -> location
  #        - %product -> item
  #        - 'openbravo' -> subcategory

  def import_workInProgress(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'ManufacturingWorkRequirement':
          continue
        records += 1
        objectid = elem.get('id')
        quantity = float(elem.find("quantity").text)
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        location = self.organization_location.get(organization, None)
        if not location:
          continue
        processPlan = frepple_operations.get( (elem.find("processPlan").get('id'), location), None)
        if not processPlan:
          continue
        startingDate = datetime.strptime(elem.find("startingDate").text, '%Y-%m-%dT%H:%M:%S.%fZ')
        endingDate = datetime.strptime(elem.find("endingDate").text, '%Y-%m-%dT%H:%M:%S.%fZ')
        if objectid in frepple_keys:
          update.append( (processPlan, quantity, startingDate, endingDate, objectid) )
        else:
          self.idcounter += 1
          insert.append( (self.idcounter, processPlan, quantity, startingDate, endingDate, objectid) )
        unused_keys.discard(objectid)
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing manufacturing work requirement ...")

      # Find all known operationplans in frePPLe
      cursor.execute("SELECT source \
         FROM operationplan \
         where source is not null \
           and operation_id like 'Processplan %'")
      frepple_keys = set([ i[0] for i in cursor.fetchall()])
      unused_keys = frepple_keys.copy()

      # Create index of all operations
      cursor.execute("SELECT name, source, location_id \
        FROM operation \
        WHERE subcategory='openbravo' \
          and source is not null")
      frepple_operations = { (i[1], i[2]): i[0] for i in cursor.fetchall() }

      # Get the list of all open work requirements
      insert = []
      update = []
      query = urllib.parse.quote("closed=false")
      self.get_data("/openbravo/ws/dal/ManufacturingWorkRequirement?where=%s" % query, parse)

      # Delete closed/canceled/deleted work requirements
      deleted = [ (i,) for i in unused_keys ]
      cursor.executemany("delete from operationplan where source=%s", deleted)

      # Create or update operationplans
      cursor.executemany(
        "insert into operationplan \
          (id,operation_id,quantity,startdate,enddate,locked,source,lastmodified) \
          values(%%s,%%s,%%s,%%s,%%s,'1',%%s,'%s')" % self.date,
        insert
        )
      cursor.executemany(
        "update operationplan \
          set operation_id=%%s, quantity=%%s, startdate=%%s, enddate=%%s, locked='1', lastmodified='%s' \
          where source=%%s" % self.date,
        update
        )

      if self.verbosity > 0:
        print("Inserted %d operationplans" % len(insert))
        print("Updated %d operationplans" % len(update))
        print("Deleted %d operationplans" % len(deleted))
        print("Imported manufacturing work requirements in %.2f seconds" % (time() - starttime))


  # Importing productboms
  #   - extracting productBOM object for all Products with
  #

  def import_productbom(self, cursor):

    def parse(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'ProductBOM':
          continue
        records += 1
        bomquantity = float(elem.find("bOMQuantity").text)
        organization = self.organizations.get(elem.find("organization").get("id"), None)
        product = self.items.get(elem.find("product").get("id"), None)
        bomproduct = self.items.get(elem.find("bOMProduct").get("id"), None)
        if not product or not organization or not bomproduct or product not in frepple_buffers:
          # Rejecting uninteresting records
          root.clear()
          continue
        for name, loc in frepple_buffers[product]:
          operation = "Product BOM %s @ %s" % (product, loc)
          buf = "%s @ %s" % (bomproduct, loc)
          operations.add( (operation, loc, name) )
          if buf not in frepple_keys:
            buffers.add( (buf, bomproduct, loc) )
          flows[ (operation, name, 'end') ] = 1
          t = (operation, buf, 'start')
          if t in flows:
            flows[t] -= bomquantity
          else:
            flows[t] = -bomquantity
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing product boms...")

      # Reset the current operations
      cursor.execute("DELETE FROM operationplan where operation_id like 'Product BOM %'")  # TODO allow incremental load!
      cursor.execute("DELETE FROM suboperation where operation_id like 'Product BOM %'")
      cursor.execute("DELETE FROM resourceload where operation_id like 'Product BOM %'")
      cursor.execute("DELETE FROM flow where operation_id like 'Product BOM %'")
      cursor.execute("UPDATE buffer SET producing_id=NULL where subcategory='openbravo' and producing_id like 'Product BOM %'")
      cursor.execute("DELETE FROM operation where name like 'Product BOM %'")

      # Get the list of all frePPLe buffers
      cursor.execute("SELECT name, item_id, location_id FROM buffer")
      frepple_buffers = {}
      frepple_keys = set()
      for i in cursor.fetchall():
        if i[1] in frepple_buffers:
          frepple_buffers[i[1]].append( (i[0], i[2]) )
        else:
          frepple_buffers[i[1]] = [ (i[0], i[2]) ]
        frepple_keys.add(i[0])

      # Loop over all productboms
      operations = set()
      buffers = set()
      flows = {}
      query = urllib.parse.quote("product.billOfMaterials=true")
      self.get_data("/openbravo/ws/dal/ProductBOM?where=%s&includeChildren=false" % query, parse)

      # Execute now on the database
      cursor.executemany(
        "insert into operation \
          (name,location_id,subcategory,type,duration,lastmodified) \
          values(%%s,%%s,'openbravo','fixed_time',0,'%s')" % self.date,
        [ (i[0], i[1]) for i in operations ]
        )
      cursor.executemany(
        "update buffer set producing_id=%%s, lastmodified='%s' where name=%%s" % self.date,
        [ (i[0], i[2]) for i in operations ]
        )
      cursor.executemany(
        "insert into buffer \
          (name,item_id,location_id,subcategory,lastmodified) \
          values(%%s,%%s,%%s,'openbravo','%s')" % self.date,
        buffers
        )
      cursor.executemany(
        "insert into flow \
          (operation_id,thebuffer_id,type,quantity,source,lastmodified) \
          values(%%s,%%s,%%s,%%s,'openbravo','%s')" % self.date,
        [ (i[0], i[1], i[2], j) for i, j in flows.items() ]
        )

      if self.verbosity > 0:
        print("Inserted %d operations" % len(operations))
        print("Created %d buffers" % len(buffers))
        print("Inserted %d flows" % len(flows))
        print("Imported product boms in %.2f seconds" % (time() - starttime))


  # Importing processplans
  #   - extracting ManufacturingProcessPlan objects for all
  #     Products with production=true and processplan <> null
  #   - Not supported yet:
  #        - date effectivity with start date in the future
  #        - multiple processplans simultaneously effective
  #        - phantom boms
  #        - subproducts
  #        - routings
  #

  def import_processplan(self, cursor):

    def parse1(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'ManufacturingProcessPlan':
          continue
        records += 1
        processplans[elem.get('id')] = elem
      return records

    def parse2(conn):
      records = 0
      root = None
      for event, elem in conn:
        if not root:
          root = elem
          continue
        if event != 'end' or elem.tag != 'Product':
          continue
        records += 1
        product = self.items.get(elem.get('id'), None)
        if not product or product not in frepple_buffers:
          # TODO A produced item which appears in a BOM but has no sales orders, purchase orders or onhand will not show up.
          # If WIP exists on a routing, it could thus happen that the operation was not created.
          # A buffer in the middle of a BOM may thus be missing.
          root.clear()
          continue   # Not interested if item isn't mapped to frePPLe

        # Pick up the processplan of the product
        processplan = elem.find("processPlan").get('id')
        root2 = processplans[processplan]

        # Create routing operation for all frePPLe buffers of this product
        # We create a routing operation in the right location
        for name, loc in frepple_buffers[product]:
          tmp0 = root2.find('manufacturingVersionList')
          if not tmp0:
            continue
          for pp_version in tmp0.findall('ManufacturingVersion'):
            endingDate = datetime.strptime(pp_version.find("endingDate").text, '%Y-%m-%dT%H:%M:%S.%fZ')
            if endingDate < self.current:
              continue  # We have passed the validity date of this version
            documentNo = pp_version.find('documentNo').text
            routing_name = "Processplan %s - %s - %s" % (name, documentNo, loc)
            if routing_name in frepple_operations:
              continue  # We apparantly already added it
            frepple_operations.add(routing_name)
            operations.append( (routing_name, loc, 'routing', None, processplan) )
            #flows[ (routing_name, name, 'end') ] = 1
            buffers_update.append( (routing_name, name) )
            tmp1 = pp_version.find('manufacturingOperationList')
            if tmp1:
              steps = set()
              for pp_operation in tmp1.findall('ManufacturingOperation'):
                objectid = pp_operation.get('id')
                sequenceNumber = int(pp_operation.find('sequenceNumber').text)
                if sequenceNumber in steps:
                  print("Warning: duplicate sequence number %s in processplan %s" % (sequenceNumber, routing_name))
                  while sequenceNumber in steps:
                    sequenceNumber += 1
                steps.add(sequenceNumber)
                costCenterUseTime = float(pp_operation.find('costCenterUseTime').text) * 3600
                step_name = "%s - %s" % (routing_name, sequenceNumber)
                operations.append( (step_name, loc, 'fixed_time', costCenterUseTime, objectid) )
                suboperations.append( (routing_name, step_name, sequenceNumber) )
                tmp2 = pp_operation.find('manufacturingOperationProductList')
                if tmp2:
                  for ff_operationproduct in tmp2.findall('ManufacturingOperationProduct'):
                    quantity = float(ff_operationproduct.find('quantity').text)
                    productionType = ff_operationproduct.find('productionType').text
                    opproduct = self.items.get(ff_operationproduct.find('product').get('id'), None)
                    if not opproduct:
                      continue  # Unknown product
                    # Find the buffer
                    opbuffer = None
                    if opproduct in frepple_buffers:
                      for bname, bloc in frepple_buffers[opproduct]:
                        if bloc == loc:
                          opbuffer = bname
                          break
                    if not opbuffer:
                      opbuffer = "%s @ %s" % (opproduct, loc)
                      buffers_create.append( (opbuffer, opproduct, loc) )
                      if opproduct not in frepple_buffers:
                        frepple_buffers[opproduct] = [ (opbuffer, loc) ]
                      else:
                        frepple_buffers[opproduct].append( (opbuffer, loc) )
                    if productionType == '-':
                      flow_key = (step_name, opbuffer, 'start')
                      if flow_key in flows:
                        flows[flow_key] -= quantity
                      else:
                        flows[flow_key] = -quantity
                    else:
                      flow_key = (step_name, opbuffer, 'end')
                      if flow_key in flows:
                        flows[flow_key] += quantity
                      else:
                        flows[flow_key] = quantity
                tmp4 = pp_operation.find('manufacturingOperationMachineList')
                if tmp4:
                  for ff_operationmachine in tmp4.findall('ManufacturingOperationMachine'):
                    machine = self.resources.get(ff_operationmachine.find('machine').get('id'), None)
                    if not machine:
                      continue  # Unknown machine
                    loads.append( (step_name, machine, 1) )
        # Clean the XML hierarchy
        root.clear()
      return records

    with transaction.atomic(using=self.database, savepoint=False):
      starttime = time()
      if self.verbosity > 0:
        print("Importing processplans...")

      # Reset the current operations
      cursor.execute("DELETE FROM operationplan where operation_id like 'Processplan %'")  # TODO allow incremental load!
      cursor.execute("DELETE FROM suboperation where operation_id like 'Processplan %'")
      cursor.execute("DELETE FROM resourceload where operation_id like 'Processplan %'")
      cursor.execute("DELETE FROM flow where operation_id like 'Processplan %'")
      cursor.execute("UPDATE buffer SET producing_id=NULL where subcategory='openbravo' and producing_id like 'Processplan %'")
      cursor.execute("DELETE FROM operation where name like 'Processplan %'")

      # Pick up existing operations in frePPLe
      cursor.execute("SELECT name FROM operation")
      frepple_operations = set([i[0] for i in cursor.fetchall()])

      # Get the list of all frePPLe buffers
      cursor.execute("SELECT name, item_id, location_id FROM buffer")
      frepple_buffers = {}
      for i in cursor.fetchall():
        if i[1] in frepple_buffers:
          frepple_buffers[i[1]].append( (i[0], i[2]) )
        else:
          frepple_buffers[i[1]] = [ (i[0], i[2]) ]

      # Get a dictionary with all process plans
      processplans = {}
      self.get_data("/openbravo/ws/dal/ManufacturingProcessPlan?includeChildren=true", parse1)

      # Loop over all produced products
      query = urllib.parse.quote("production=true and processPlan is not null")
      operations = []
      suboperations = []
      buffers_create = []
      buffers_update = []
      flows = {}
      loads = []
      self.get_data("/openbravo/ws/dal/Product?where=%s&orderBy=name&includeChildren=false" % query, parse2)

      # TODO use "decrease" and "rejected" fields on steps to compute the yield
      # TODO multiple processplans for the same item -> alternate operation

      # Execute now on the database
      cursor.executemany(
        "insert into operation \
          (name,location_id,subcategory,type,duration,source,lastmodified) \
          values(%%s,%%s,'openbravo',%%s,%%s,%%s,'%s')" % self.date,
        operations
        )
      cursor.executemany(
        "insert into suboperation \
          (operation_id,suboperation_id,priority,source,lastmodified) \
          values(%%s,%%s,%%s,'openbravo','%s')" % self.date,
        suboperations
        )
      cursor.executemany(
        "update buffer set producing_id=%%s, lastmodified='%s' where name=%%s" % self.date,
        buffers_update
        )
      cursor.executemany(
        "insert into buffer \
          (name,item_id,location_id,subcategory,lastmodified) \
          values(%%s,%%s,%%s,'openbravo','%s')" % self.date,
        buffers_create
        )
      cursor.executemany(
        "insert into flow \
          (operation_id,thebuffer_id,type,quantity,source,lastmodified) \
          values(%%s,%%s,%%s,%%s,'openbravo','%s')" % self.date,
        [ (i[0], i[1], i[2], j) for i, j in flows.items() ]
        )
      cursor.executemany(
        "insert into resourceload \
          (operation_id,resource_id,quantity,source,lastmodified) \
          values(%%s,%%s,%%s,'openbravo','%s')" % self.date,
        loads
        )

      if self.verbosity > 0:
        print("Inserted %d operations" % len(operations))
        print("Inserted %d suboperations" % len(suboperations))
        print("Updated %d buffers" % len(buffers_update))
        print("Created %d buffers" % len(buffers_create))
        print("Inserted %d flows" % len(flows))
        print("Inserted %d loads" % len(loads))
        print("Imported processplans in %.2f seconds" % (time() - starttime))
