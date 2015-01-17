#
# Copyright (C) 2014 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

import hashlib
import time

from django.conf import settings
from django.contrib.admin.views.decorators import staff_member_required
from django.shortcuts import render
from django.utils.translation import ugettext_lazy as _
from django.views.generic import TemplateView
from django.views.decorators.cache import never_cache

from freppledb.common.models import Parameter


# Validity duration of a login token
PLANBOARD_SESSION_DURATION = 3600


@staff_member_required
@never_cache
def Board(request):
  t = round(time.time()) + PLANBOARD_SESSION_DURATION
  message = "%s%s%s%s" % (request.user.username, request.user.id, t, settings.SECRET_KEY)
  return render(request, 'planningboard/index.html', {
    "time": t,
    "token": hashlib.sha256(message.encode('utf-8')).hexdigest(),
    "port": int(Parameter.getValue("planningboard.port", request.database, 8001)),
    "title": _("Planning board"),
    })
