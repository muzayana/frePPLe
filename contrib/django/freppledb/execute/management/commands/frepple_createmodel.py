#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
import random
from optparse import make_option
from datetime import timedelta, datetime, date

from django.conf import settings
from django.core.management.base import BaseCommand, CommandError
from django.core import management
from django.db import connections, DEFAULT_DB_ALIAS, transaction
from django.db.models import Min, Max

from freppledb.common.models import Parameter, Bucket, BucketDetail
from freppledb.input.models import Operation, Buffer, Resource, Location, Calendar
from freppledb.input.models import CalendarBucket, Customer, Demand, Flow
from freppledb.input.models import Load, Item
from freppledb.execute.models import Task
from freppledb.common.models import User
from freppledb import VERSION

try:
  from freppledb.forecast.models import Forecast
  has_forecast = True
except:
  has_forecast = False


class Command(BaseCommand):

  help = '''
      This command is a simple, generic model generator.
      It is meant as a quick way to produce models for tests on performance,
      memory size, database size...

      The auto-generated supply network looks schematically as follows:
        [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
        [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
        [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
        [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
        [ Operation -> buffer ] ...   [ -> Operation -> buffer ]  [ Delivery ]
            ...                                  ...
      Each row represents a cluster.
      The operation+buffer are repeated as many times as the depth of the supply
      path parameter.
      In each cluster a single item is defined, and a parametrizable number of
      demands is placed on the cluster.

      The script uses random numbers. These are reproducible (ie different runs
      will produce the same random number sequence), but not portable (ie runs
      on a different platform or version can give different results).
    '''

  option_list = BaseCommand.option_list + (
    make_option(
      '--user', dest='user', type='string',
      help='User running the command'
      ),
    make_option(
      '--cluster', dest='cluster', type="int",
      help='Number of end items', default=100
      ),
    make_option(
      '--demand', dest='demand', type="int",
      help='Demands per end item', default=30),
    make_option(
      '--forecast_per_item', dest='forecast_per_item', type="int",
      help='Monthly forecast per end item', default=30
      ),
    make_option(
      '--level', dest='level', type="int",
      help='Depth of bill-of-material', default=5
      ),
    make_option(
      '--resource', dest='resource', type="int",
      help='Number of resources', default=60
      ),
    make_option(
      '--resource_size', dest='resource_size', type="int",
      help='Size of each resource', default=5
      ),
    make_option(
      '--components', dest='components', type="int",
      help='Total number of components', default=200
      ),
    make_option(
      '--components_per', dest='components_per', type="int",
      help='Number of components per end item', default=4
      ),
    make_option(
      '--deliver_lt', dest='deliver_lt', type="int",
      help='Average delivery lead time of orders', default=30
      ),
    make_option(
      '--procure_lt', dest='procure_lt', type="int",
      help='Average procurement lead time', default=40
      ),
    make_option(
      '--currentdate', dest='currentdate', type="string",
      help='Current date of the plan in YYYY-MM-DD format'
      ),
    make_option(
      '--database', action='store', dest='database',
      default=DEFAULT_DB_ALIAS,
      help='Nominates a specific database to populate'
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
    # Make sure the debug flag is not set!
    # When it is set, the django database wrapper collects a list of all sql
    # statements executed and their timings. This consumes plenty of memory
    # and cpu time.
    tmp_debug = settings.DEBUG
    settings.DEBUG = False

    # Pick up the options
    if 'verbosity' in options:
      verbosity = int(options['verbosity'])
    else:
      verbosity = 1
    if 'cluster' in options:
      cluster = int(options['cluster'])
    else:
      cluster = 100
    if 'demand' in options:
      demand = int(options['demand'])
    else:
      demand = 30
    if 'forecast_per_item' in options:
      forecast_per_item = int(options['forecast_per_item'])
    else:
      forecast_per_item = 50
    if 'level' in options:
      level = int(options['level'])
    else:
      level = 5
    if 'resource' in options:
      resource = int(options['resource'])
    else:
      resource = 60
    if 'resource_size' in options:
      resource_size = int(options['resource_size'])
    else:
      resource_size = 5
    if 'components' in options:
      components = int(options['components'])
    else:
      components = 200
    if 'components_per' in options:
      components_per = int(options['components_per'])
    else:
      components_per = 5
    if components == 0:
      components_per = 0
    if 'deliver_lt' in options:
      deliver_lt = int(options['deliver_lt'])
    else:
      deliver_lt = 30
    if 'procure_lt' in options:
      procure_lt = int(options['procure_lt'])
    else:
      procure_lt = 40
    if 'currentdate' in options:
      currentdate = options['currentdate'] or datetime.strftime(date.today(), '%Y-%m-%d')
    else:
      currentdate = datetime.strftime(date.today(), '%Y-%m-%d')
    if 'database' in options:
      database = options['database'] or DEFAULT_DB_ALIAS
    else:
      database = DEFAULT_DB_ALIAS
    if not database in settings.DATABASES:
      raise CommandError("No database settings known for '%s'" % database )
    if 'user' in options and options['user']:
      try:
        user = User.objects.all().using(database).get(username=options['user'])
      except:
        raise CommandError("User '%s' not found" % options['user'] )
    else:
      user = None

    random.seed(100)  # Initialize random seed to get reproducible results

    now = datetime.now()
    task = None
    try:
      # Initialize the task
      if 'task' in options and options['task']:
        try:
          task = Task.objects.all().using(database).get(pk=options['task'])
        except:
          raise CommandError("Task identifier not found")
        if task.started or task.finished or task.status != "Waiting" or task.name != 'generate model':
          raise CommandError("Invalid task identifier")
        task.status = '0%'
        task.started = now
      else:
        task = Task(name='generate model', submitted=now, started=now, status='0%', user=user)
      task.arguments = "--cluster=%s --demand=%s --forecast_per_item=%s --level=%s --resource=%s " \
        "--resource_size=%s --components=%s --components_per=%s --deliver_lt=%s --procure_lt=%s" % (
          cluster, demand, forecast_per_item, level, resource,
          resource_size, components, components_per, deliver_lt, procure_lt
        )
      task.save(using=database)

      # Pick up the startdate
      try:
        startdate = datetime.strptime(currentdate, '%Y-%m-%d')
      except:
        raise CommandError("current date is not matching format YYYY-MM-DD")

      # Check whether the database is empty
      if Buffer.objects.using(database).count() > 0 or Item.objects.using(database).count() > 0:
        raise CommandError("Database must be empty before creating a model")

      # Plan start date
      if verbosity > 0:
        print("Updating current date...")
      param = Parameter.objects.using(database).create(name="currentdate")
      param.value = datetime.strftime(startdate, "%Y-%m-%d %H:%M:%S")
      param.save(using=database)

      # Parameters
      Parameter.objects.using(database).create(
        name='loading_time_units', value='days',
        description='Time units to be used for the resource report: hours, days, weeks'
        ).save(using=database)
      Parameter.objects.using(database).create(
        name='plan.loglevel', value='0',
        description='Controls the verbosity of the planning log file. Accepted values are 0(silent - default), 1 and 2 (verbose)'
        ).save(using=database)
      if has_forecast:
        Parameter.objects.using(database).create(
          name='forecast.Croston_initialAlfa', value='0.1',
          description='Initial parameter for the Croston forecast method.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Croston_maxAlfa', value='1',
          description='Maximum parameter for the Croston forecast method.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Croston_minAlfa', value='0.03',
          description='Minimum parameter for the Croston forecast method.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Croston_minIntermittence', value='0.33',
          description='Minimum intermittence (defined as the percentage of zero demand buckets) before the Croston method is applied.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_dampenTrend', value='0.8',
          description='Dampening factor applied to the trend in future periods.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_initialAlfa', value='0.2',
          description='Initial smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_initialGamma', value='0.2',
          description='Initial trend smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_maxAlfa', value='1',
          description='Maximum smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_maxGamma', value='1',
          description='Maximum trend smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_minAlfa', value='0.02',
          description='Minimum smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DoubleExponential_minGamma', value='0.05',
          description='Minimum trend smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.DueAtEndOfBucket', value='1',
          description='By setting this flag to true, the forecast will be due at the end of the forecast bucket.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Iterations', value='15',
          description='Specifies the maximum number of iterations allowed for a forecast method to tune its parameters.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.loglevel', value='0',
          description='Verbosity of the forecast solver'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.MovingAverage_order', value='5',
          description='This parameter controls the number of buckets to be averaged by the moving average forecast method.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Net_CustomerThenItemHierarchy', value='1',
          description='This flag allows us to control whether we first search the customer hierarchy and then the item hierarchy, or the other way around.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Net_MatchUsingDeliveryOperation', value='1',
          description='Specifies whether or not a demand and a forecast require to have the same delivery operation to be a match.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Net_NetEarly', value='0',
          description='Defines how much time before the due date of an order we are allowed to search for a forecast bucket to net from.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Net_NetLate', value='0',
          description='Defines how much time after the due date of an order we are allowed to search for a forecast bucket to net from.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Outlier_maxDeviation', value='4',
          description='Multiple of the standard deviation used to detect outliers'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_dampenTrend', value='0.8',
          description='Dampening factor applied to the trend in future periods.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_gamma', value='0.05',
          description='Value of the seasonal parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_initialAlfa', value='0.2',
          description='Initial value for the constant parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_initialBeta', value='0.2',
          description='Initial value for the trend parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_maxAlfa', value='1',
          description='Maximum value for the constant parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_maxBeta', value='1',
          description='Maximum value for the trend parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_maxPeriod', value='14',
          description='Maximum seasonal cycle to be checked.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_minAlfa', value='0.02',
          description='Minimum value for the constant parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_minBeta', value='0.2',
          description='Initial value for the trend parameter'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Seasonal_minPeriod', value='2',
          description='Minimum seasonal cycle to be checked.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.SingleExponential_initialAlfa', value='0.2',
          description='Initial smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.SingleExponential_maxAlfa', value='1',
          description='Maximum smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.SingleExponential_minAlfa', value='0.03',
          description='Minimum smoothing constant.'
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.Skip', value='0',
          description="Specifies the number of time series values used to initialize the forecasting method. The forecast error in these bucket isn't counted."
          ).save(using=database)
        Parameter.objects.using(database).create(
          name='forecast.SmapeAlfa', value='0.95',
          description='Specifies how the sMAPE forecast error is weighted for different time buckets.'
          ).save(using=database)

      # Planning horizon
      # minimum 10 daily buckets, weekly buckets till 40 days after current
      if verbosity > 0:
        print("Updating buckets...")
      management.call_command('frepple_createbuckets', user=user, database=database)
      if verbosity > 0:
        print("Updating horizon telescope...")
      updateTelescope(10, 40, 730, database)
      task.status = '2%'
      task.save(using=database)

      # Weeks calendar
      if verbosity > 0:
        print("Creating weeks calendar...")
      with transaction.atomic(using=database):
        weeks = Calendar.objects.using(database).create(name="Weeks", defaultvalue=0)
        for i in BucketDetail.objects.using(database).filter(bucket="week").all():
          CalendarBucket(
            startdate=i.startdate, enddate=i.enddate, value=1, calendar=weeks
            ).save(using=database)
        task.status = '4%'
        task.save(using=database)

      # Working days calendar
      if verbosity > 0:
        print("Creating working days...")
      with transaction.atomic(using=database):
        workingdays = Calendar.objects.using(database).create(name="Working Days", defaultvalue=0)
        minmax = BucketDetail.objects.using(database).filter(bucket="week").aggregate(Min('startdate'), Max('startdate'))
        CalendarBucket(
          startdate=minmax['startdate__min'], enddate=minmax['startdate__max'],
          value=1, calendar=workingdays, priority=1, saturday=False, sunday=False
          ).save(using=database)
        task.status = '6%'
        task.save(using=database)

      # Create a random list of categories to choose from
      categories = [
        'cat A', 'cat B', 'cat C', 'cat D', 'cat E', 'cat F', 'cat G'
        ]

      # Create customers
      if verbosity > 0:
        print("Creating customers...")
      with transaction.atomic(using=database):
        cust = []
        for i in range(100):
          c = Customer.objects.using(database).create(name='Cust %03d' % i)
          cust.append(c)
        task.status = '8%'
        task.save(using=database)

      # Create resources and their calendars
      if verbosity > 0:
        print("Creating resources and calendars...")
      with transaction.atomic(using=database):
        res = []
        for i in range(resource):
          loc = Location(name='Loc %05d' % int(random.uniform(1, cluster)))
          loc.save(using=database)
          cal = Calendar(name='capacity for res %03d' % i, category='capacity', defaultvalue=0)
          bkt = CalendarBucket(startdate=startdate, value=resource_size, calendar=cal)
          cal.save(using=database)
          bkt.save(using=database)
          r = Resource.objects.using(database).create(
            name='Res %03d' % i, maximum_calendar=cal, location=loc
            )
          res.append(r)
        task.status = '10%'
        task.save(using=database)
        random.shuffle(res)

      # Create the components
      if verbosity > 0:
        print("Creating raw materials...")
      with transaction.atomic(using=database):
        comps = []
        comploc = Location.objects.using(database).create(name='Procured materials')
        for i in range(components):
          it = Item.objects.using(database).create(
            name='Component %04d' % i,
            category='Procured',
            price=str(round(random.uniform(0, 100)))
            )
          ld = abs(round(random.normalvariate(procure_lt, procure_lt / 3)))
          c = Buffer.objects.using(database).create(
            name='Component %04d' % i,
            location=comploc,
            category='Procured',
            item=it,
            type='procure',
            min_inventory=20,
            max_inventory=100,
            size_multiple=10,
            leadtime=str(ld * 86400),
            onhand=str(round(forecast_per_item * random.uniform(1, 3) * ld / 30)),
            )
          comps.append(c)
        task.status = '12%'
        task.save(using=database)

      # Loop over all clusters
      durations = [ 86400, 86400 * 2, 86400 * 3, 86400 * 5, 86400 * 6 ]
      progress = 88.0 / cluster
      for i in range(cluster):
        with transaction.atomic(using=database):
          if verbosity > 0:
            print("Creating supply chain for end item %d..." % i)

          # location
          loc = Location.objects.using(database).get_or_create(name='Loc %05d' % i)[0]
          loc.available = workingdays
          loc.save(using=database)

          # Item and delivery operation
          oper = Operation.objects.using(database).create(name='Del %05d' % i, sizemultiple=1, location=loc)
          it = Item.objects.using(database).create(
            name='Itm %05d' % i,
            operation=oper,
            category=random.choice(categories),
            price=str(round(random.uniform(100, 200)))
            )

          # Level 0 buffer
          buf = Buffer.objects.using(database).create(
            name='Buf %05d L00' % i,
            item=it,
            location=loc,
            category='00'
            )
          Flow.objects.using(database).create(operation=oper, thebuffer=buf, quantity=-1)

          # Demand
          for j in range(demand):
            Demand.objects.using(database).create(
              name='Dmd %05d %05d' % (i, j),
              item=it,
              quantity=int(random.uniform(1, 6)),
              # Exponential distribution of due dates, with an average of deliver_lt days.
              due=startdate + timedelta(days=round(random.expovariate(float(1) / deliver_lt / 24)) / 24),
              # Orders have higher priority than forecast
              priority=random.choice([1, 2]),
              customer=random.choice(cust),
              category=random.choice(categories)
              )

          # Create upstream operations and buffers
          ops = []
          for k in range(level):
            if k == 1 and res:
              # Create a resource load for operations on level 1
              oper = Operation.objects.using(database).create(
                name='Oper %05d L%02d' % (i, k),
                type='time_per',
                location=loc,
                duration_per=86400,
                sizemultiple=1,
                )
              if resource < cluster and i < resource:
                # When there are more cluster than resources, we try to assure
                # that each resource is loaded by at least 1 operation.
                Load.objects.using(database).create(resource=res[i], operation=oper)
              else:
                Load.objects.using(database).create(resource=random.choice(res), operation=oper)
            else:
              oper = Operation.objects.using(database).create(
                name='Oper %05d L%02d' % (i, k),
                duration=random.choice(durations),
                sizemultiple=1,
                location=loc,
                )
            ops.append(oper)
            buf.producing = oper
            # Some inventory in random buffers
            if random.uniform(0, 1) > 0.8:
              buf.onhand = int(random.uniform(5, 20))
            buf.save(using=database)
            Flow(operation=oper, thebuffer=buf, quantity=1, type="end").save(using=database)
            if k != level - 1:
              # Consume from the next level in the bill of material
              buf = Buffer.objects.using(database).create(
                name='Buf %05d L%02d' % (i, k + 1),
                item=it,
                location=loc,
                category='%02d' % (k + 1)
                )
              Flow.objects.using(database).create(operation=oper, thebuffer=buf, quantity=-1)

          # Consume raw materials / components
          c = []
          for j in range(components_per):
            o = random.choice(ops)
            b = random.choice(comps)
            while (o, b) in c:
              # A flow with the same operation and buffer already exists
              o = random.choice(ops)
              b = random.choice(comps)
            c.append( (o, b) )
            Flow.objects.using(database).create(
              operation=o, thebuffer=b,
              quantity=random.choice([-1, -1, -1, -2, -3])
              )

          # Commit the current cluster
          task.status = '%d%%' % (12 + progress * (i + 1))
          task.save(using=database)

      if has_forecast:
        for i in range(cluster):
          # Forecast
          fcst = Forecast.objects.using(database).create(
            name='Forecast item %05d' % i,
            calendar=weeks,
            item=it,
            customer=random.choice(cust),
            maxlateness=60 * 86400,  # Forecast can only be planned 2 months late
            priority=3,  # Low priority: prefer planning orders over forecast
            discrete=True
            )

          # This method will take care of distributing a forecast quantity over the entire
          # horizon, respecting the bucket weights.
          fcst.setTotal(startdate, startdate + timedelta(365), forecast_per_item * 12)

      # Task update
      task.status = 'Done'
      task.finished = datetime.now()

    except Exception as e:
      if task:
        task.status = 'Failed'
        task.message = '%s' % e
        task.finished = datetime.now()
        task.save(using=database)
      raise e

    finally:
      if task:
        task.save(using=database)
      settings.DEBUG = tmp_debug


def updateTelescope(min_day_horizon=10, min_week_horizon=40, min_month_horizon=730, database=DEFAULT_DB_ALIAS):
  '''
  Update for the telescopic horizon.
  The first argument specifies the minimum number of daily buckets. Additional
  daily buckets will be appended till we come to a monday. At that date weekly
  buckets are starting.
  The second argument specifies the minimum horizon with weeks before the
  monthly buckets. The last weekly bucket can be a partial one: starting on
  monday and ending on the first day of the next calendar month.
  '''

  # Make sure the debug flag is not set!
  # When it is set, the django database wrapper collects a list of all sql
  # statements executed and their timings. This consumes plenty of memory
  # and cpu time.
  tmp_debug = settings.DEBUG
  settings.DEBUG = False
  try:
    with transaction.atomic(using=database, savepoint=False):
      
      # Delete previous contents
      connections[database].cursor().execute(
        "delete from common_bucketdetail where bucket_id = 'telescope'"
        )
  
      # Create bucket
      try:
        b = Bucket.objects.using(database).get(name='telescope')
      except Bucket.DoesNotExist:
        b = Bucket(name='telescope', description='Time buckets with decreasing granularity')
      b.save(using=database)
  
      # Create bucket for all dates in the past
      startdate = datetime.strptime(Parameter.objects.using(database).get(name="currentdate").value, "%Y-%m-%d %H:%M:%S")
      curdate = startdate
      BucketDetail(
        bucket=b,
        name='past',
        startdate=datetime(2000, 1, 1),
        enddate=curdate,
        ).save(using=database)
  
      # Create daily buckets
      limit = curdate + timedelta(min_day_horizon)
      while curdate < limit or curdate.strftime("%w") != '0':
        BucketDetail(
          bucket=b,
          name=str(curdate.date()),
          startdate=curdate,
          enddate=curdate + timedelta(1)
          ).save(using=database)
        curdate = curdate + timedelta(1)
  
      # Create weekly buckets
      limit = startdate + timedelta(min_week_horizon)
      stop = False
      while not stop:
        enddate = curdate + timedelta(7)
        if curdate > limit and curdate.month != enddate.month:
          stop = True
          enddate = datetime(enddate.year, enddate.month, 1)
        BucketDetail(
          bucket=b,
          name=curdate.strftime("%y W%W"),
          startdate=curdate,
          enddate=enddate
          ).save(using=database)
        curdate = enddate
  
      # Create monthly buckets
      limit = startdate + timedelta(min_month_horizon)
      while curdate < limit:
        enddate = curdate + timedelta(32)
        enddate = datetime(enddate.year, enddate.month, 1)
        BucketDetail(
          bucket=b,
          name=curdate.strftime("%b %y"),
          startdate=curdate,
          enddate=enddate
          ).save(using=database)
        curdate = enddate

  finally:
    settings.DEBUG = tmp_debug
