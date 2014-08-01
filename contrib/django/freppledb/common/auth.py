#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.contrib.auth import authenticate, login
from django.contrib.auth.backends import ModelBackend
from django.core.validators import validate_email
from django.core.exceptions import ValidationError
from django.http import HttpResponse

from freppledb.common.models import User


class EmailBackend(ModelBackend):
  '''
  This customized authentication allows logging in using either
  the user name or the user email address.
  '''
  def authenticate(self, username=None, password=None):
    try:
      validate_email(username)
      # The user name looks like an email address
      user = User.objects.get(email=username)
      if user.check_password(password):
        return user
    except User.DoesNotExist:
      return None
    except ValidationError:
      # The user name isn't an email address
      try:
        user = User.objects.get(username=username)
        if user.check_password(password):
          return user
      except User.DoesNotExist:
        return None


    def get_user(self, user_id):
      '''
      This is identical to django.contrib.auth.backends.ModelBackend.get_user
      with a small performance optimization.
      '''
      try:
        return User.objects.get(pk=user_id)
      except User.DoesNotExist:
        return None


def basicauthentication(allow_logged_in=True, perm=None, realm="frepple"):
  '''
  A decorator that requires a user to be logged in. If they are not
  logged in the request is examined for a 'authorization' header.

  If the header is present it is tested for basic authentication and
  the user is logged in with the provided credentials.

  If the header is not present a http 401 is sent back to the
  requestor to provide credentials.

  This code is inspired on and shamelessly copied from:
    https://djangosnippets.org/snippets/243/
  '''
  def view_decorator(view):
    def wrapper(request, *args, **kwargs):
      ok = False
      try:
        if allow_logged_in:
          u = getattr(request, 'user', None)
          if u and u.is_authenticated() and (not perm or u.has_perm(perm)):
            ok = True
        auth_header = request.META.get('HTTP_AUTHORIZATION', None)
        if auth_header:
          # Missing the header
          authmeth, auth = auth_header.split(' ', 1)
          if authmeth.lower() == 'basic':
            # Only basic authentication
            auth = auth.strip().decode('base64')
            user, password = auth.split(':', 1)
            user = authenticate(username=user, password=password)
            if user and user.is_active:
              # Active
              login(request, user)
              request.user = user
              ok = True
      except:
        # Everything going wrong in the above will get the 401-unauthorized
        # reply. Any exception is silently ignored.
        pass
      if ok:
        # All clear
        return view(request, *args, **kwargs)
      else:
        # Send a 401-unauthorized response with a header prompting for a
        # username and password.
        resp = HttpResponse("Missing or incorrect authorization header", status=401)
        resp['WWW-Authenticate'] = 'Basic realm="%s"' % realm
        return resp
    return wrapper
  return view_decorator
