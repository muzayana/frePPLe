#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import json

from django.conf import settings
from django.contrib.admin.utils import unquote
from django.contrib.admin.views.decorators import staff_member_required
from django.contrib.contenttypes.models import ContentType
from django.db.models import Q
from django.http import Http404
from django.http.response import StreamingHttpResponse, HttpResponse, HttpResponseServerError
from django.utils.decorators import method_decorator
from django.utils.translation import ugettext_lazy as _
from django.views.generic import View

from freppledb.common.report import GridFieldText, GridReport
from freppledb.common.report import GridFieldLastModified, GridFieldChoice
from freppledb.common.report import GridFieldNumber, GridFieldBool

from freppledb.inventoryplanning.models import InventoryPlanning
from freppledb.input.models import Buffer, Item, Location
from freppledb.common.models import Comment


import logging
logger = logging.getLogger(__name__)


class InventoryPlanningList(GridReport):
  '''
  A list report to show inventory planning parameters.
  '''
  template = 'inventoryplanning/inventoryplanninglist.html'
  title = _("inventory planning parameters")
  basequeryset = InventoryPlanning.objects.all()
  model = InventoryPlanning
  frozenColumns = 1

  rows = (
    GridFieldText('buffer', title=_('buffer'), key=True, formatter='buffer'),
    GridFieldNumber('roq_min_qty', title=_('ROQ minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_max_qty', title=_('ROQ maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_multiple_qty', title=_('ROQ multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_min_poc', title=_('ROQ minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_max_poc', title=_('ROQ maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('leadtime_deviation', title=_('lead time deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('demand_deviation', title=_('demand deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldChoice('demand_distribution', title=_('demand distribution'),
      choices=InventoryPlanning.distributions, extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('service_level', title=_('service level'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_qty', title=_('safety stock minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_max_qty', title=_('safety stock maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_multiple_qty', title=_('safety stock multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_poc', title=_('safety stock minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_max_poc', title=_('safety stock maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldBool('nostock', title=_("Do not stock")),
    GridFieldText('source', title=_('source')),
    GridFieldLastModified('lastmodified'),
    )


class DRP(GridReport):
  template = 'inventoryplanning/drp.html'
  title = _("Distribution planning")
  basequeryset = InventoryPlanning.objects.all()
  model = InventoryPlanning
  height = 150
  frozenColumns = 1
  multiselect = False
  editable = False

  rows = (
    GridFieldText('buffer', title=_('buffer'), key=True, formatter='buffer'),
    GridFieldBool('nostock', title=_("Do not stock")),
    GridFieldNumber('roq_min_qty', title=_('ROQ minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_max_qty', title=_('ROQ maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_multiple_qty', title=_('ROQ multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_min_poc', title=_('ROQ minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('roq_max_poc', title=_('ROQ maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('leadtime_deviation', title=_('lead time deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('demand_deviation', title=_('demand deviation'), extra="formatoptions:{defaultValue:''}"),
    GridFieldChoice('demand_distribution', title=_('demand distribution'),
      choices=InventoryPlanning.distributions, extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('service_level', title=_('service level'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_qty', title=_('safety stock minimum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_max_qty', title=_('safety stock maximum quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_multiple_qty', title=_('safety stock multiple quantity'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_min_poc', title=_('safety stock minimum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldNumber('ss_max_poc', title=_('safety stock maximum period of cover'), extra="formatoptions:{defaultValue:''}"),
    GridFieldText('source', title=_('source')),
    GridFieldLastModified('lastmodified'),
    )


class DRPitemlocation(View):

  def getData(self, request, itemlocation):
    # This query retrieves all data for a certain itemlocation.
    # Advantage is that all data are sent to the user's browser in a single response,
    # and the user can navigate them without

    # Retrieve forecast data
    print("retrieving for " + itemlocation)
    yield '{"type":"itemlocation", "name":' +  json.dumps(itemlocation) + ","

    ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)

    yield '"parameters":' + json.dumps({
      "ss_multiple_qty": str(ip.ss_multiple_qty) if ip.ss_multiple_qty is not None else None,
      "ss_min_qty": str(ip.ss_min_qty) if ip.ss_multiple_qty is not None else None,
      "roq_min_poc": str(ip.roq_min_poc) if ip.ss_multiple_qty is not None else None,
      "ss_min_poc": str(ip.ss_min_poc) if ip.ss_multiple_qty is not None else None,
      "roq_min_qty": str(ip.roq_min_qty) if ip.ss_multiple_qty is not None else None,
      "demand_distribution": str(ip.demand_distribution) if ip.ss_multiple_qty is not None else None,
      "ss_max_qty": str(ip.ss_max_qty) if ip.ss_multiple_qty is not None else None,
      "leadtime_deviation": str(ip.leadtime_deviation) if ip.ss_multiple_qty is not None else None,
      "roq_max_qty": str(ip.roq_max_qty) if ip.roq_max_qty is not None else None,
      "roq_multiple_qty": str(ip.roq_multiple_qty) if ip.roq_multiple_qty is not None else None,
      "nostock": str(ip.nostock) if ip.nostock is not None else None,
      "roq_max_poc": str(ip.roq_max_poc) if ip.roq_max_poc is not None else None,
      "service_level": str(ip.service_level) if ip.service_level is not None else None,
      "demand_deviation": str(ip.demand_deviation) if ip.demand_deviation is not None else None,
      "ss_max_poc": str(ip.ss_max_poc) if ip.ss_max_poc is not None else None
      })
    yield ","
    # Retrieve inventory plan
    yield '"plan":' + json.dumps({"test": "val", "koko": 1})

    # Retrieve transactions

    # Retrieve comments
    buffer_type = ContentType.objects.get_for_model(Buffer)
    item_type = ContentType.objects.get_for_model(Item)
    location_type = ContentType.objects.get_for_model(Location)
    comments = Comment.objects.using(request.database).filter(
      Q(content_type=buffer_type.id, object_pk=ip.buffer.name)
      | Q(content_type=item_type.id, object_pk=ip.buffer.item.name if ip.buffer.item else None)
      | Q(content_type=location_type.id, object_pk=ip.buffer.location.name if ip.buffer.location else None)
      ).order_by('-lastmodified')
    yield ',"comments":['
    first = True
    for i in comments:
      if first:
        first = False
      else:
        yield ","
      if i.content_type == buffer_type:
        t = "itemlocation"
      elif i.content_type == item_type:
        t = "item"
      else:
        t = "location"
      yield json.dumps({
        "user": "%s (%s)" % (i.user.username, i.user.get_full_name()),
        "lastmodified": str(i.lastmodified),
        "comment": i.comment,
        "type": t
        })
    yield "]}"

    # Retrieve history: lazy?


  @method_decorator(staff_member_required)
  def get(self, request, arg):
    # Only accept ajax requests on this URL
    if not request.is_ajax():
      raise Http404('Only ajax requests allowed')

    # Verify permissions TODO

    # Unescape special characters in the argument, which is encoded django-admin style.
    itemlocation = unquote(arg)

    # Stream back the response
    response = StreamingHttpResponse(
      content_type='application/json; charset=%s' % settings.DEFAULT_CHARSET,
      streaming_content=self.getData(request, itemlocation)
      )
    response['Cache-Control'] = "no-cache, no-store"
    return response


  @method_decorator(staff_member_required)
  def post(self, request, arg):
    # Only accept ajax requests on this URL
    if not request.is_ajax():
      raise Http404('Only ajax requests allowed')

    try:
      # Unescape special characters in the argument, which is encoded django-admin style.
      itemlocation = unquote(arg)

      # Look up the relevant object
      ip = InventoryPlanning.objects.using(request.database).get(pk=itemlocation)

      # Retrieve the posted data
      data = json.JSONDecoder().decode(request.read().decode(request.encoding or settings.DEFAULT_CHARSET))

      print("posted:", itemlocation, data)

      # Retrieve the comment
      if 'commenttype' in data and 'comment' in data:
        if data['commenttype'] == 'item' and ip.buffer.item:
          Comment(
            content_object=ip.buffer.item,
            user=request.user,
            comment=data['comment']
            ).save(using=request.database)
        elif data['commenttype'] == 'location' and ip.buffer.location:
          Comment(
            content_object=ip.buffer.location,
            user=request.user,
            comment=data['comment']
            ).save(using=request.database)
        elif data['commenttype'] == 'itemlocation':
          Comment(
            content_object=ip.buffer,
            user=request.user,
            comment=data['comment']
            ).save(using=request.database)
        else:
          raise Exception("Invalid comment data")

      return HttpResponse(content="OK")

    except Exception as e:
      logger.error("Error saving DRP updates: %s" % e)
      return HttpResponseServerError('Error saving updates')


class DRPitem(DRPitemlocation):
  def getData(self, request, itemlocation):
    # This query retrieves all data for a certain itemlocation.
    # Advantage is that all data are sent to the user's browser in a single response,
    # and the user can navigate them without

    # Retrieve forecast data
    print("retrieving for " + itemlocation)
    yield '{"type":"item", "name":' +  json.dumps(itemlocation) + ","
    yield '"forecast":' + json.dumps([itemlocation, {"test2": "valzezeze", "koko2": 1222}])
    yield ","
    # Retrieve inventory plan
    yield '"plan":' + json.dumps({"test": "val", "koko": 1})
    yield "}"
    # Retrieve transactions
    # Retrieve comments
    # Retrieve history: lazy?


class DRPlocation(DRPitemlocation):
  def getData(self, request, itemlocation):
    # This query retrieves all data for a certain itemlocation.
    # Advantage is that all data are sent to the user's browser in a single response,
    # and the user can navigate them without

    # Retrieve forecast data
    print("retrieving for " + itemlocation)
    yield '{"type":"location", "name":' +  json.dumps(itemlocation) + ","
    yield '"forecast":' + json.dumps([itemlocation, {"test2": "valzezeze", "koko2": 1222}])
    yield ","
    # Retrieve inventory plan
    yield '"plan":' + json.dumps({"test": "val", "koko": 1})
    yield "}"
    # Retrieve transactions
    # Retrieve comments
    # Retrieve history: lazy?

