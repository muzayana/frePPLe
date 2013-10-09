#
# Copyright (C) 2007-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from freppledb.menu import menu
from freppledb.forecast.views import OverviewReport

# Adding reports. We use an index value to keep the same order of the entries in all languages.
menu.addItem("reports", "forecast report", url="/forecast/", report=OverviewReport, index=250)
