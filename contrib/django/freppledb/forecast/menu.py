#
# Copyright (C) 2007-2012 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from freppledb.menu import menu
from freppledb.forecast.models import Forecast, ForecastDemand
from freppledb.forecast.views import OverviewReport, ForecastList, ForecastDemandList

# Adding reports. We use an index value to keep the same order of the entries in all languages.
menu.addItem("sales", "forecast report", url="/forecast/", report=OverviewReport, index=110)
menu.addItem("sales", "forecast", url="/data/forecast/forecast/", report=ForecastList, index=1210, model=Forecast)
menu.addItem("sales", "forecast demand", url="/data/forecast/forecastdemand/", report=ForecastDemandList, index=1220, model=ForecastDemand)

#/data/inventoryplanning/inventoryplanning/">Inventory planning parameters</a></li>
