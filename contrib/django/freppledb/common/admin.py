#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime

from django.contrib.auth.models import Group
from django.contrib.auth.admin import UserAdmin, GroupAdmin
from django.contrib.auth.forms import UserCreationForm
from django import forms
from django.forms.util import ErrorList
from django.utils.translation import ugettext_lazy as _

from freppledb.common.models import User, Parameter, Comment, Bucket, BucketDetail
from freppledb.common.adminforms import MultiDBModelAdmin, MultiDBTabularInline
from freppledb.admin import admin_site


# Registering the User admin for our custom model.
# See http://stackoverflow.com/questions/16953302/django-custom-user-model-in-admin-relation-auth-user-does-not-exists
# to understand the need for this extra customization.
class MyUserCreationForm(UserCreationForm):
  def clean_username(self):
    username = self.cleaned_data["username"]
    try:
      User.objects.get(username=username)
    except User.DoesNotExist:
      return username
    raise forms.ValidationError(self.error_messages['duplicate_username'])

  class Meta(UserCreationForm.Meta):
    model = User


class MyUserAdmin(UserAdmin):
    save_on_top = True
    add_form = MyUserCreationForm

admin_site.register(Group, GroupAdmin)
admin_site.register(User, MyUserAdmin)


class ParameterForm(forms.ModelForm):
  class Meta:
    model = Parameter
    fields = ('name', 'value', 'description')

  def clean(self):
    cleaned_data = self.cleaned_data
    name = cleaned_data.get("name")
    value = cleaned_data.get("value")
    # Currentdate parameter must be a date+time value
    if name == "currentdate":
      try:
        datetime.strptime(value, "%Y-%m-%d %H:%M:%S")
      except:
        self._errors["value"] = ErrorList([_("Invalid date: expecting YYYY-MM-DD HH:MM:SS")])
        del cleaned_data["value"]
    return cleaned_data


class Parameter_admin(MultiDBModelAdmin):
  model = Parameter
  save_on_top = True
  form = ParameterForm
  exclude = ('source',)
admin_site.register(Parameter, Parameter_admin)


class Comment_admin(MultiDBModelAdmin):
  model = Comment
  save_on_top = True
admin_site.register(Comment, Comment_admin)


class BucketDetail_inline(MultiDBTabularInline):
  model = BucketDetail
  max_num = 10
  extra = 3
  exclude = ('source',)


class BucketDetail_admin(MultiDBModelAdmin):
  model = BucketDetail
  save_on_top = True
  exclude = ('source',)
admin_site.register(BucketDetail, BucketDetail_admin)


class Bucket_admin(MultiDBModelAdmin):
  model = Bucket
  save_on_top = True
  inlines = [ BucketDetail_inline, ]
  exclude = ('source',)
admin_site.register(Bucket, Bucket_admin)
