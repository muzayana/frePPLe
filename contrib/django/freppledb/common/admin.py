#
# Copyright (C) 2007-2013 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime

from django import forms
from django.contrib.auth.admin import UserAdmin
from django.contrib.auth.models import Group
from django.forms.utils import ErrorList
from django.utils.translation import ugettext_lazy as _

from freppledb.common.models import User, Parameter, Comment, Bucket, BucketDetail
from freppledb.common.adminforms import MultiDBModelAdmin, MultiDBTabularInline
from freppledb.admin import admin_site


class MyUserAdmin(UserAdmin, MultiDBModelAdmin):
  '''
  This class is a frePPLe specific override of its standard
  Django base class.
  '''
  save_on_top = True

  change_user_password_template = 'auth/change_password.html'

  fieldsets = (
    (None, {'fields': ('username', 'password')}),
    (_('Personal info'), {'fields': ('first_name', 'last_name', 'email')}),
    (_('Permissions in this scenario'), {'fields': ('is_active', 'is_superuser',
                                   'groups', 'user_permissions')}),
    (_('Important dates'), {'fields': ('last_login', 'date_joined')}),
  )

  def get_readonly_fields(self, request, obj=None):
    if obj:
      return self.readonly_fields + ('last_login', 'date_joined')
    return self.readonly_fields

  def has_delete_permission(self, request, obj=None):
    # Users can't be deleted. Just mark them as inactive instead
    return False

admin_site.register(User, MyUserAdmin)


class MyGroupAdmin(MultiDBModelAdmin):
  pass
MyGroupAdmin.tabs = [
    {"name": 'edit', "label": _("edit"), "view":  MyGroupAdmin.change_view, "permission": ''},
    {"name": 'comments', "label": _("comments"), "view": MyGroupAdmin.comment_view, "permission": ''},
    {"name": 'history', "label": _("history"), "view": MyGroupAdmin.history_view, "permission": ''},
  ]
admin_site.register(Group, MyGroupAdmin)


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
Parameter_admin.tabs = [
    {"name": 'edit', "label": _("edit"), "view":  Parameter_admin.change_view, "permission": ''},
    {"name": 'comments', "label": _("comments"), "view": Parameter_admin.comment_view, "permission": ''},
    {"name": 'history', "label": _("history"), "view": Parameter_admin.history_view, "permission": ''},
  ]
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
Parameter_admin.tabs = [
    {"name": 'edit', "label": _("edit"), "view":  BucketDetail_admin.change_view, "permission": ''},
    {"name": 'comments', "label": _("comments"), "view": BucketDetail_admin.comment_view, "permission": ''},
    {"name": 'history', "label": _("history"), "view": BucketDetail_admin.history_view, "permission": ''},
  ]
admin_site.register(BucketDetail, BucketDetail_admin)


class Bucket_admin(MultiDBModelAdmin):
  model = Bucket
  save_on_top = True
  inlines = [ BucketDetail_inline, ]
  exclude = ('source',)
Bucket_admin.tabs = [
    {"name": 'edit', "label": _("edit"), "view":  Bucket_admin.change_view, "permission": ''},
    {"name": 'comments', "label": _("comments"), "view": Bucket_admin.comment_view, "permission": ''},
    {"name": 'history', "label": _("history"), "view": Bucket_admin.history_view, "permission": ''},
  ]
admin_site.register(Bucket, Bucket_admin)
