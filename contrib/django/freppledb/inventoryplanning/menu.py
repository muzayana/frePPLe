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
from freppledb.inventoryplanning.views import InventoryPlanningList, DRP

# Adding reports. We use an index value to keep the same order of the entries in all languages.
menu.addItem("distribution", "drp", url="/inventoryplanning/drp/", report=DRP, index=150)
menu.addItem("inventory", "inventory planning parameters", url="/data/inventoryplanning/inventoryplanning/", report=InventoryPlanningList, index=1300)
