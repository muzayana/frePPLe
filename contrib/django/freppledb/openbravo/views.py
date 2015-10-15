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

from django.contrib.auth.decorators import login_required
from django.views.decorators.csrf import csrf_protect
from django.http import HttpResponse


@login_required
@csrf_protect
def Upload(request):

  data = json.loads(request.body.decode('utf-8'))
  
  print(data)
#  return HttpResponse(content="OK", status = 200)
  return HttpResponse(content="Not implemented yet...", status = 402)
