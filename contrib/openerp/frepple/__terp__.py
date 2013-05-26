# -*- encoding: utf-8 -*-
#
# Copyright (C) 2010-2012 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

{
    "name" : "Advanced Planning and Scheduling Module",
    "version" : "1.0",
    "author" : "frePPLe",
    "website" : "http://www.frepple.com",
    "category" : "Generic Modules/Production",
    "depends" : ["mrp","sale_order_dates"],
    "license" : "Other OSI approved license",
    "description": """
    This module performs constrained planning scheduling with frePPLe.
    """,
    "init_xml": [],
    "update_xml": [
      'frepple_data.xml',
      'frepple_view.xml',
      'ir.model.access.csv',
      ],
    "demo_xml": [],
    "installable": True,
    "active": False,
}

# vim:expandtab:smartindent:tabstop=4:softtabstop=4:shiftwidth=4:
