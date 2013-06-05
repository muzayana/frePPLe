#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.contrib.auth.backends import ModelBackend
from django.core.validators import email_re

from freppledb.common.models import User


class EmailBackend(ModelBackend):
  '''
  This customized authentication allows logging in using either
  the user name or the user email address.
  '''
  def authenticate(self, username=None, password=None):
    if email_re.search(username):
      # The username looks like an email address
      try:
        user = User.objects.get(email=username)
        if user.check_password(password):
          return user
      except User.DoesNotExist:
        return None
    else:
      # The user name isn't an email address
      try:
        user = User.objects.get(username=username)
        if user.check_password(password):
          return user
      except User.DoesNotExist:
        return None
