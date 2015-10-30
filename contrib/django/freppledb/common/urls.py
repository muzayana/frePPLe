#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.conf.urls import patterns

import freppledb.common.views
import freppledb.common.dashboard


# Automatically add these URLs when the application is installed
autodiscover = True

urlpatterns = patterns(
  # Prefix
  '',

  # User preferences
  (r'^preferences/$', freppledb.common.views.preferences),

  # Horizon updates
  (r'^horizon/$', freppledb.common.views.horizon),

  # Report settings
  (r'^settings/$', freppledb.common.views.saveSettings),

  # Dashboard widgets
  (r'^widget/(.+)/', freppledb.common.dashboard.Dashboard.dispatch),

  # Model list reports, which override standard admin screens
  (r'^admin/auth/group/$', freppledb.common.views.GroupList.as_view()),
  (r'^admin/common/user/$', freppledb.common.views.UserList.as_view()),
  (r'^admin/common/bucket/$', freppledb.common.views.BucketList.as_view()),
  (r'^admin/common/bucketdetail/$', freppledb.common.views.BucketDetailList.as_view()),
  (r'^admin/common/parameter/$', freppledb.common.views.ParameterList.as_view()),
  (r'^comments/([^/]+)/([^/]+)/(.+)/$', freppledb.common.views.Comments),
  (r'^admin/common/comment/$', freppledb.common.views.CommentList.as_view()),

  (r'^detail/([^/]+)/([^/]+)/(.+)/$', freppledb.common.views.detail),
)
