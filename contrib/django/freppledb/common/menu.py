#
# Copyright (C) 2007-2012 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _
from django.conf import settings
from django.contrib.auth.models import Group

import freppledb.common.views
from freppledb.common.models import User, Bucket, BucketDetail, Parameter, Comment
from freppledb.menu import menu
from freppledb import VERSION


# Settings menu
menu.addItem(
  "admin", "parameter admin", url="/data/common/parameter/",
  report=freppledb.common.views.ParameterList, index=1100, model=Parameter
  )
menu.addItem(
  "admin", "bucket admin", url="/data/common/bucket/",
  report=freppledb.common.views.BucketList, index=1200, model=Bucket
  )
menu.addItem(
  "admin", "bucketdetail admin", url="/data/common/bucketdetail/",
  report=freppledb.common.views.BucketDetailList, index=1300, model=BucketDetail
  )
menu.addItem(
  "admin", "comment admin", url="/data/common/comment/",
  report=freppledb.common.views.CommentList, index=1400, model=Comment
  )

# User maintenance
menu.addItem("admin", "users", separator=True, index=2000)
menu.addItem(
  "admin", "user admin", url="/data/common/user/",
  report=freppledb.common.views.UserList, index=2100, model=User
  )
menu.addItem(
  "admin", "group admin", url="/data/auth/group/",
  report=freppledb.common.views.GroupList, index=2200, model=Group
  )

# Help menu
menu.addItem("help", "tour", javascript="tour.start('0,0,0')", label=_('Guided tour'), index=100)

versionnumber=VERSION.split('.', 2)
docurl="http://frepple.com/docs/"+versionnumber[0]+"."+versionnumber[1]+"/"
#. Translators: Translation included with Django
menu.addItem("help", "documentation", url=docurl, label=_('View documentation'), window=True, prefix=False, index=200)
menu.addItem("help", "API", url="/api/", label=_('REST API help'), window=True, prefix=False, index=300)
menu.addItem("help", "website", url="http://frepple.com", window=True, label=_('frePPLe website'), prefix=False, index=400)
