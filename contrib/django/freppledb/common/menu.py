#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.utils.translation import ugettext_lazy as _
from django.conf import settings

from freppledb.menu import menu
from freppledb.admin import admin_site

# Admin menu
menu.addItem("admin", "admin_site", admin=admin_site, index=200)

# User menu
menu.addItem("user", "logout", url="/admin/logout/", label=_('Log out'), prefix=False, index=100)
menu.addItem("user", "preferences", url="/preferences/", label=_('Preferences'), index=200)
menu.addItem("user", "change password", url="/admin/password_change/", label=_('Change password'), index=300)

# Help menu
menu.addItem("help", "tour", javascript="tour.start('0,0,0')", label=_('Guided tour'), index=100)
menu.addItem("help", "documentation", url="%sdoc/index.html" % settings.STATIC_URL, label=_('Documentation'), window=True, prefix=False, index=300)
menu.addItem("help", "website", url="http://frepple.com", window=True, label=_('frePPLe website'), prefix=False, index=400)
