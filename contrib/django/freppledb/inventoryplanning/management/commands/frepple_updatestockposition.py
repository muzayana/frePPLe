#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime
from optparse import make_option

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings
from django.db import DEFAULT_DB_ALIAS, connections

from freppledb.execute.models import Task
from freppledb.common.models import User, Parameter
from freppledb import VERSION


sql = '''
  with position as (
      select
        buffer_id,
        coalesce(max(item.price), 0) as price,
        max(buffer.onhand) as onhand,
        max(out_inventoryplanning.leadtime) as leadtime,
        coalesce(max(roq_cal.value), 1) reorderquantity,
        coalesce(max(ss_cal.value), 0) safetystock
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      inner join item
        on item.name = buffer.item_id
      left outer join calendarbucket roq_cal
        on roq_cal.calendar_id = 'ROQ for ' || out_inventoryplanning.buffer_id
        and roq_cal.startdate <= %%s and roq_cal.enddate > %%s
        and roq_cal.priority = (
           select min(priority) from calendarbucket
           where calendar_id = 'ROQ for ' || out_inventoryplanning.buffer_id
           and roq_cal.startdate <= %%s and roq_cal.enddate > %%s
           )
      left outer join calendarbucket ss_cal
        on ss_cal.calendar_id = 'SS for ' || out_inventoryplanning.buffer_id
        and ss_cal.startdate <= %%s and ss_cal.enddate > %%s
        and ss_cal.priority = (
           select min(priority) from calendarbucket
           where calendar_id = 'SS for ' || out_inventoryplanning.buffer_id
           and ss_cal.startdate <= %%s and ss_cal.enddate > %%s
           )
      %s
      group by buffer_id
    ),
    purchases as (
      select
        buffer_id,
        sum(case
          when purchase_order.status = 'proposed'
            then purchase_order.quantity
          else 0
          end) as proposed,
        sum(case
          when purchase_order.status = 'confirmed'
            then purchase_order.quantity
          else 0
          end) as open
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      left outer join purchase_order
        on purchase_order.item_id = buffer.item_id
        and purchase_order.location_id = buffer.location_id
        and purchase_order.enddate < %%s + out_inventoryplanning.leadtime
      %s
      group by buffer_id
    ),
    transfers as (
      select
      buffer_id,
      sum(case
        when distribution_order.status = 'proposed' and distribution_order.destination_id = buffer.location_id
          then distribution_order.quantity
        when distribution_order.status = 'proposed' and distribution_order.origin_id = buffer.location_id and consume_material = true
          then -distribution_order.quantity
        else 0
        end) as proposed,
      sum(case
        when distribution_order.status = 'confirmed' and distribution_order.destination_id = buffer.location_id
          then distribution_order.quantity
        when distribution_order.status = 'confirmed' and distribution_order.origin_id = buffer.location_id and consume_material = true
          then -distribution_order.quantity
        else 0
        end) as open
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      left outer join distribution_order
        on distribution_order.item_id = buffer.item_id
        and (distribution_order.destination_id = buffer.location_id
          or distribution_order.origin_id = buffer.location_id)
        and distribution_order.enddate < %%s + out_inventoryplanning.leadtime
      %s
      group by buffer_id
    ),
    demand as (
      select
        buffer_id,
        sum(case
          when demand.due < %%s
            then demand.quantity
          else 0
          end) as overdue,
        sum(case
          when demand.due >= %%s
            then demand.quantity
          else 0
          end) as open
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      left outer join demand
        on demand.item_id = buffer.item_id
        and demand.location_id = buffer.location_id
        and demand.due < %%s + out_inventoryplanning.leadtime
        and demand.status = 'open'
      %s
      group by buffer_id
    )
  update out_inventoryplanning
  set
    onhand = position.onhand,
    overduesalesorders = demand.overdue,
    opensalesorders = demand.open,
    proposedpurchases = purchases.proposed,
    openpurchases = purchases.open,
    proposedtransfers = transfers.proposed,
    opentransfers = transfers.open,
    reorderquantity = position.reorderquantity,
    safetystock = position.safetystock,
    onhandvalue = position.onhand * position.price,
    overduesalesordersvalue = demand.overdue * position.price,
    opensalesordersvalue = demand.open * position.price,
    proposedpurchasesvalue = purchases.proposed * position.price,
    openpurchasesvalue = purchases.open * position.price,
    proposedtransfersvalue = transfers.proposed * position.price,
    opentransfersvalue = transfers.open * position.price,
    reorderquantityvalue = position.reorderquantity * position.price,
    safetystockvalue = position.safetystock * position.price
  from position, transfers, purchases, demand
  where out_inventoryplanning.buffer_id = position.buffer_id
  and out_inventoryplanning.buffer_id = transfers.buffer_id
  and out_inventoryplanning.buffer_id = purchases.buffer_id
  and out_inventoryplanning.buffer_id = demand.buffer_id
  '''


class Command(BaseCommand):
  help = '''
  This command recomputes the inventory position of all buffers.

  It is basically evaluated as:
     inventory position =
       current inventory on hand =
       - all open orders due within the lead time
       - all net forecast due within the lead time
       - all dependent demand expected during the lead time
       + all purchase orders due to arrive within the lead time
       + all distribution orders due to arrive within the lead time
  '''
  option_list = BaseCommand.option_list + (
    make_option(
      '--user', dest='user', type='string',
      help='User running the command'
      ),
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS,
      help='Nominates a specific database to backup'
      ),
    make_option(
      '--task', dest='task', type='int',
      help='Task identifier (generated automatically if not provided)'
      ),
    )

  requires_system_checks = False

  def get_version(self):
    return VERSION

  def handle(self, **options):

    # Pick up the options
    if 'database' in options:
      database = options['database'] or DEFAULT_DB_ALIAS
    else:
      database = DEFAULT_DB_ALIAS
    if database not in settings.DATABASES:
      raise CommandError("No database settings known for '%s'" % database )
    if 'user' in options and options['user']:
      try:
        user = User.objects.all().using(database).get(username=options['user'])
      except:
        raise CommandError("User '%s' not found" % options['user'] )
    else:
      user = None

    now = datetime.now()
    task = None
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try:
          task = Task.objects.all().using(database).get(pk=options['task'])
        except:
          raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'backup database':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='evaluate inventory', submitted=now, started=now, status='0%', user=user)

      # Run the command
      updateStockPosition(database=database)

      # Task update
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
        task.save(using=database)


def updateStockPosition(item=None, database=DEFAULT_DB_ALIAS):
  # Get the current date
  try:
    current_date = datetime.strptime(
      Parameter.objects.using(database).get(name="currentdate").value,
      "%Y-%m-%d %H:%M:%S"
      )
  except:
    current_date = datetime.now()

  # Run update query
  cursor = connections[database].cursor()
  if item:
    cursor.execute(sql % (
      "where buffer.item_id = %s", "where buffer.item_id = %s",
      "where buffer.item_id = %s", "where buffer.item_id = %s"
      ), (
      current_date, current_date, current_date, current_date,
      current_date, current_date, current_date, current_date, item,
      current_date, item, current_date, item,
      current_date, current_date, current_date, item
      ))
  else:
    cursor.execute(sql % ('', '', '', ''), (
      current_date, current_date, current_date, current_date,
      current_date, current_date, current_date, current_date,
      current_date, current_date,
      current_date, current_date, current_date
      ))
  # TODO Loop over records to calculate the fill rate
