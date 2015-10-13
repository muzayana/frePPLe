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
        coalesce(max(item.price), 0) price,
        coalesce(max(buffer.onhand), 0) onhand,
        coalesce(max(out_inventoryplanning.leadtime), interval '0 day') leadtime,
        coalesce(max(roq_cal.value), 1) reorderquantity,
        coalesce(max(ss_cal.value), 0) safetystock
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      left outer join item
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
        coalesce(sum(case
          when purchase_order.status = 'proposed'
            then purchase_order.quantity
          else 0
          end),0) as proposed,
        coalesce(sum(case
          when purchase_order.status = 'confirmed'
            then purchase_order.quantity
          else 0
          end),0) as open
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      left outer join purchase_order
        on purchase_order.item_id = buffer.item_id
        and purchase_order.location_id = buffer.location_id
        and purchase_order.enddate <= %%s + out_inventoryplanning.leadtime + interval '7 day'
      %s
      group by buffer_id
    ),
    transfers as (
      select
      buffer_id,
      coalesce(sum(case
        when distribution_order.status = 'proposed' and distribution_order.destination_id = buffer.location_id
          then distribution_order.quantity
        when distribution_order.status = 'proposed' and distribution_order.origin_id = buffer.location_id and consume_material = true
          then -distribution_order.quantity
        else 0
        end),0) as proposed,
      coalesce(sum(case
        when distribution_order.status = 'confirmed' and distribution_order.destination_id = buffer.location_id
          then distribution_order.quantity
        when distribution_order.status = 'confirmed' and distribution_order.origin_id = buffer.location_id and consume_material = true
          then -distribution_order.quantity
        else 0
        end),0) as open
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
    salesorders as (
      select
        buffer_id,
        coalesce(sum(case
          when demand.due < %%s
            then demand.quantity
          end),0) as overdue,
        coalesce(sum(case
          when demand.due >= %%s
            then demand.quantity
          end),0) as open
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
    ),
    forecast as (
      select
        buffer_id,
        coalesce(round(sum(
          forecasttotal
          * (extract(epoch from (least(%%s + out_inventoryplanning.leadtime, forecastplan.enddate) - forecastplan.startdate)))
          / (extract(epoch from out_inventoryplanning.leadtime))
          )),0) as total,
        coalesce(round(sum(
          forecastnet
          * (extract(epoch from (least(%%s + out_inventoryplanning.leadtime, forecastplan.enddate) - forecastplan.startdate)))
          / (extract(epoch from out_inventoryplanning.leadtime))
          )),0) as net
      from out_inventoryplanning
      inner join buffer
        on buffer.name = out_inventoryplanning.buffer_id
      left outer join forecast
        on buffer.item_id = forecast.item_id
        and buffer.location_id = forecast.location_id
      left outer join forecastplan
        on forecastplan.forecast_id = forecast.name
        and forecastplan.startdate >= %%s
        and forecastplan.startdate < %%s + out_inventoryplanning.leadtime
      %s
      group by buffer_id
      ),
    depdemand as (
      select
        out_inventoryplanning.buffer_id buffer_id,
        coalesce(-sum(
          case when operationplan.id is not null or distribution_order.id is not null or purchase_order.id is not null
          then out_flowplan.quantity end
          ),0) total
      from out_inventoryplanning
      left outer join out_flowplan
        on out_inventoryplanning.buffer_id = out_flowplan.thebuffer
        and out_flowplan.quantity < 0
        and out_flowplan.flowdate < %%s + out_inventoryplanning.leadtime
      left outer join operationplan
        on out_flowplan.operationplan_id = operationplan.id
      left outer join distribution_order
        on out_flowplan.operationplan_id = distribution_order.id
      left outer join purchase_order
        on out_flowplan.operationplan_id = purchase_order.id
      %s
      group by buffer_id
    )
  update out_inventoryplanning
  set
    onhand = position.onhand,
    localforecast = coalesce(forecast.total, 0),
    dependentdemand = coalesce(depdemand.total, 0),
    overduesalesorders = coalesce(salesorders.overdue, 0),
    opensalesorders = coalesce(salesorders.open, 0),
    proposedpurchases = coalesce(purchases.proposed, 0),
    openpurchases = coalesce(purchases.open, 0),
    proposedtransfers = coalesce(transfers.proposed, 0),
    opentransfers = coalesce(transfers.open, 0),
    reorderquantity = coalesce(position.reorderquantity, 0),
    safetystock = coalesce(position.safetystock, 0),
    onhandvalue = coalesce(position.onhand * position.price, 0),
    localforecastvalue = coalesce(forecast.total * position.price, 0),
    dependentdemandvalue = coalesce(depdemand.total * position.price, 0),
    overduesalesordersvalue = coalesce(salesorders.overdue * position.price, 0),
    opensalesordersvalue = coalesce(salesorders.open * position.price, 0),
    proposedpurchasesvalue = coalesce(purchases.proposed * position.price, 0),
    openpurchasesvalue = coalesce(purchases.open * position.price, 0),
    proposedtransfersvalue = coalesce(transfers.proposed * position.price, 0),
    opentransfersvalue = coalesce(transfers.open * position.price, 0),
    reorderquantityvalue = coalesce(position.reorderquantity * position.price, 0),
    safetystockvalue = coalesce(position.safetystock * position.price, 0)
  from position, transfers, purchases, salesorders, forecast, depdemand
  where out_inventoryplanning.buffer_id = position.buffer_id
  and out_inventoryplanning.buffer_id = transfers.buffer_id
  and out_inventoryplanning.buffer_id = purchases.buffer_id
  and out_inventoryplanning.buffer_id = salesorders.buffer_id
  and out_inventoryplanning.buffer_id = forecast.buffer_id
  and out_inventoryplanning.buffer_id = depdemand.buffer_id
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
  # TODO minor issue the current date is passed as a time zone unaware object.
  cursor = connections[database].cursor()
  if item:
    cursor.execute(sql % (
      "where buffer.item_id = %s", "where buffer.item_id = %s",
      "where buffer.item_id = %s", "where buffer.item_id = %s",
      "where buffer.item_id = %s", "and buffer.item_id = %s"
      ), (
      current_date, current_date, current_date, current_date,
      current_date, current_date, current_date, current_date, item,
      current_date, item, current_date, item,
      current_date, current_date, current_date, item,
      current_date, current_date, current_date, current_date, item,
      current_date, item
      ))
  else:
    cursor.execute(sql % ('', '', '', '', '', ''), (
      current_date, current_date, current_date, current_date,
      current_date, current_date, current_date, current_date,
      current_date, current_date,
      current_date, current_date, current_date,
      current_date, current_date, current_date, current_date,
      current_date
      ))
  # TODO Loop over records to calculate the fill rate
