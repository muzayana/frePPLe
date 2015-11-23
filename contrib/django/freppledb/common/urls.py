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
from django.views.generic.base import RedirectView

import freppledb.common.views
import freppledb.common.serializers
import freppledb.common.dashboard

from freppledb.common.api.views import APIIndexView


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
  (r'^data/auth/group/$', freppledb.common.views.GroupList.as_view()),
  (r'^data/common/user/$', freppledb.common.views.UserList.as_view()),
  (r'^data/common/bucket/$', freppledb.common.views.BucketList.as_view()),
  (r'^data/common/bucketdetail/$', freppledb.common.views.BucketDetailList.as_view()),
  (r'^data/common/parameter/$', freppledb.common.views.ParameterList.as_view()),
  (r'^data/common/comment/$', freppledb.common.views.CommentList.as_view()),
  (r'^comments/([^/]+)/([^/]+)/(.+)/$', freppledb.common.views.Comments),

  # Special case of the next line for user password changes in the user edit screen
  (r'detail/common/user/(?P<id>.+)/password/$', RedirectView.as_view(url="/data/common/user/%(id)s/password/")),

  # Detail URL for an object, which internally redirects to the view for the last opened tab
  (r'^detail/([^/]+)/([^/]+)/(.+)/$', freppledb.common.views.detail),

  # REST API framework
  (r'^api/common/bucket/$', freppledb.common.serializers.BucketAPI.as_view()),
  (r'^api/common/bucketdetail/$', freppledb.common.serializers.BucketDetailAPI.as_view()),
  (r'^api/common/parameter/$', freppledb.common.serializers.ParameterAPI.as_view()),
  (r'^api/common/comment/$', freppledb.common.serializers.CommentAPI.as_view()),
  (r'^api/common/bucket/(?P<pk>(.+))/$', freppledb.common.serializers.BucketdetailAPI.as_view()),
  (r'^api/common/bucketdetail/(?P<pk>(.+))/$', freppledb.common.serializers.BucketDetaildetailAPI.as_view()),
  (r'^api/common/parameter/(?P<pk>(.+))/$', freppledb.common.serializers.ParameterdetailAPI.as_view()),
  (r'^api/common/comment/(?P<pk>(.+))/$', freppledb.common.serializers.CommentdetailAPI.as_view()),
  (r'^api/$', APIIndexView),
)
