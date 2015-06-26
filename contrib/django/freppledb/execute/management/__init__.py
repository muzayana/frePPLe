#
# Copyright (C) 2009-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db.models import signals

from freppledb.common.management import removeDefaultPermissions
from freppledb.execute import models as execute_models


signals.post_syncdb.connect(removeDefaultPermissions, execute_models)
