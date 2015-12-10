#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from time import time
from datetime import datetime

from django.db import DEFAULT_DB_ALIAS, connections, transaction

from freppledb.common.models import Parameter

import frepple


def exportResults(cursor, database):

  def buffers():
    for b in frepple.buffers():
      try:
        if b.ip_flag == True:
          yield b
      except:
        pass

  def calendars():
    cursor.execute("select name from calendar where source = 'Inventory planning'")
    cals = set([ i[0] for i in cursor.fetchall() ])
    for c in frepple.calendars():
      if c.source == 'Inventory planning' and c.name not in cals:
        yield c

  def buckets():
    for c in frepple.calendars():
      if c.source != 'Inventory planning':
        continue
      for i in c.buckets:
        if i.source == 'Inventory planning':
          yield i

  def int_to_time(i):
    hour = i // 3600
    i -= (hour * 3600)
    minute = i // 60
    i -= (minute * 60)
    second = i
    if hour >= 24:
      hour -= 24
    return "%s:%s:%s" % (hour, minute, second)

  with transaction.atomic(using=database, savepoint=False):
    print("Exporting inventory planning results...")
    timestamp = str(datetime.now())
    starttime = time()

    cursor.execute("delete from out_inventoryplanning")  # TODO avoid complete rebuild of the table
    cursor.executemany('''
      insert into out_inventoryplanning
      (buffer_id, leadtime, reorderquantity, reorderquantityvalue, calculatedsafetystock, safetystock, calculatedreorderquantity)
      values (%s,(%s||' second')::interval ,%s,%s,%s,%s,%s)
      ''',
      [
        (b.name, b.ip_leadtime, b.ip_roq, b.ip_roq * b.item.price, b.ip_calculated_ss, b.ip_ss, b.ip_calculated_roq)
        for b in buffers()
      ])

    cursor.execute('''
      delete from calendarbucket where source = 'Inventory planning'
    ''')
    cursor.executemany('''
      insert into calendar \
        (name,defaultvalue,source,lastmodified) \
      values(%s,%s,%s,%s)
      ''',
      [ (i.name, i.default, i.source, timestamp) for i in calendars() ]
      )
    cursor.executemany(
      '''insert into calendarbucket
      (calendar_id,startdate,enddate,priority,value,
       sunday,monday,tuesday,wednesday,thursday,friday,saturday,
       starttime,endtime,source,lastmodified)
      values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)''',
      [
        (
          i.calendar.name, str(i.start), str(i.end), i.priority,
          round(i.value, 4),
          (i.days & 1) and True or False, (i.days & 2) and True or False,
          (i.days & 4) and True or False, (i.days & 8) and True or False,
          (i.days & 16) and True or False, (i.days & 32) and True or False,
          (i.days & 64) and True or False,
          int_to_time(i.starttime), int_to_time(i.endtime - 1),
          i.source, timestamp
        )
        for i in buckets()
      ])

  # Keep the database healthy after this bulky data change
  cursor.execute('vacuum analyze out_inventoryplanning')
  cursor.execute('vacuum analyze calendarbucket')

  print('Exported inventory planning results in %.2f seconds' % (time() - starttime))


def createInventoryPlan(database=DEFAULT_DB_ALIAS):

  try:
    ip_calendar = frepple.calendar(
      name=Parameter.getValue('inventoryplanning.calendar', database),
      action='C'
      )
  except:
    print("Warning: Parameter inventoryplanning.calendar not configured.")
    print("Warning: All inventory planning related calculations will be skipped.")
    return

  print("Start inventory planning at", datetime.now().strftime("%H:%M:%S"))

  cursor = connections[database].cursor()

  # Step 1.
  # Create inventoryplanning records for all buffers, where they don't exist yet.
  # TODO Only create for the lowest level item+location combinations
  cursor.execute('''
    insert into inventoryplanning
      (buffer_id, nostock, lastmodified)
    select name, false, current_timestamp from buffer
    except
    select buffer_id, false, current_timestamp from inventoryplanning
    ''')

  # Step 2.
  # Calculate the demand deviation from the independent demand
  # Copy the standard deviation from the forecast table.
  # Assumption: there is a 1-to-1 relation between a forecast and the buffer
  cursor.execute('''
    update inventoryplanning
    set demand_deviation = dep_stddev.out_deviation
    from (
      select buffer.name, out_deviation
      from buffer
      inner join forecast
      on forecast.item_id = buffer.item_id
      and forecast.location_id = buffer.location_id
      ) dep_stddev
    where inventoryplanning.buffer_id = dep_stddev.name
    ''')
  cursor.execute('vacuum analyze inventoryplanning')
  # # Alternative method: compute the standard deviation directly
  # try:
  #   current_date = datetime.strptime(
  #     Parameter.objects.using(database).get(name="currentdate").value,
  #     "%Y-%m-%d %H:%M:%S"
  #     )
  # except:
  #   current_date = datetime.now()
  # cursor.execute('''
  #   update inventoryplanning
  #   set demand_deviation = coalesce(dep_stddev.stdev, 0)
  #   from (
  #     select buffer.name buffer_id, stddev_samp(indep_demand) stdev
  #     from buffer
  #     left outer join (
  #       select
  #         item_id, location_id, startdate,
  #         coalesce(sum(forecastplan.orderstotal),0) + coalesce(sum(forecastplan.ordersadjustment),0) as indep_demand
  #       from forecast
  #       inner join location
  #         on forecast.location_id = location.name
  #       inner join item
  #         on forecast.item_id = item.name
  #       inner join forecastplan
  #         on forecastplan.forecast_id = forecast.name
  #       left outer join (
  #         select forecast_id, min(startdate) period
  #         from forecastplan
  #         where orderstotal > 0 or ordersadjustment > 0
  #         group by forecast_id
  #         ) first_hit
  #       on forecastplan.forecast_id = first_hit.forecast_id
  #       where enddate <= timestamp '%s'
  #       and startdate >= first_hit.period
  #       group by item_id, location_id, startdate
  #       ) dmd
  #       on dmd.item_id = buffer.item_id
  #       and dmd.location_id = buffer.location_id
  #       group by buffer.name
  #    ) dep_stddev
  #    where inventoryplanning.buffer_id = dep_stddev.buffer_id
  #    ''' % (current_date,) )

  # Step 3.
  # Load inventory planning parameters
  starttime = time()
  cursor.execute('''
    select
      buffer_id, roq_min_qty, roq_max_qty, roq_multiple_qty,
      roq_min_poc, roq_max_poc, leadtime_deviation, demand_deviation,
      demand_distribution, service_level, ss_min_qty, ss_max_qty,
      ss_multiple_qty, ss_min_poc, ss_max_poc, nostock, roq_type, ss_type
    from inventoryplanning
    ''')
  cnt = 0
  for i in cursor.fetchall():
    cnt += 1
    buf = frepple.buffer(name=i[0])
    buf.ip_flag = True
    if i[1]:
      buf.roq_min_qty = i[1]
    if i[2]:
      buf.roq_max_qty = i[2]
    if i[3]:
      buf.roq_multiple_qty = i[3]
    if i[4]:
      buf.roq_min_poc = i[4]
    if i[5]:
      buf.roq_max_poc = i[5]
    if i[6]:
      buf.leadtime_deviation = i[6]
    if i[7]:
      buf.demand_deviation = i[7]
    if i[8]:
      buf.demand_distribution = i[8]
    if i[9]:
      buf.service_level = i[9]
    if i[10]:
      buf.ss_min_qty = i[10]
    if i[11]:
      buf.ss_max_qty = i[11]
    if i[12]:
      buf.ss_multiple_qty = i[12]
    if i[13]:
      buf.ss_min_poc = i[13]
    if i[14]:
      buf.ss_max_poc = i[14]
    if i[15]:
      buf.nostock = i[15]
    if i[16]:
      buf.roq_type = i[16]
    else:
      # The engine uses "combined" as default. We apply a different default here!
      buf.roq_type = "calculated"
    if i[17]:
      buf.ss_type = i[17]
    else:
      # The engine uses "combined" as default. We apply a different default here!
      buf.ss_type = "calculated"
  print('Loaded %d inventory planning parameters in %.2f seconds' % (cnt, time() - starttime))

  # Step 4.
  # Run the inventory planning solver to compute the safety stock and reorder quantities
  frepple.solver_inventoryplanning(
    calendar=ip_calendar,
    horizon_start=int(Parameter.getValue('inventoryplanning.horizon_start', database, 0)),
    horizon_end=int(Parameter.getValue('inventoryplanning.horizon_end', database, 365)),
    holding_cost=float(Parameter.getValue('inventoryplanning.holding_cost', database, 0.05)),
    fixed_order_cost=float(Parameter.getValue('inventoryplanning.fixed_order_cost', database, 20)),
    loglevel=int(Parameter.getValue('inventoryplanning.loglevel', database, 0)),
    service_level_on_average_inventory=(Parameter.getValue("inventoryplanning.service_level_on_average_inventory", database, "true") == "true")
    ).solve()

  # Step 5.
  # Export all results into the database
  exportResults(cursor, database)

  print("Finished inventory planning at", datetime.now().strftime("%H:%M:%S"))


if __name__ == "__main__":
  print("CONFIGURATION ERROR:")
  print("  Don't use inventoryplanning/commands.py as the main planning script")  # TODO
  print("  Change the order of the INSTALLED_APPS in your djangosettings.py configuration file")