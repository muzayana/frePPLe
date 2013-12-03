# -*- encoding: utf-8 -*-
#
# Copyright (C) 2010-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from osv import fields
from osv import osv
from tools.translate import _


class frepple_setupmatrix(osv.osv):
    _name = "frepple.setupmatrix"
    _description = "Setup matrix"
    _columns = {
        'name': fields.char('Name', size=32, required=True),
        'active': fields.boolean('Active'),
        'note': fields.text('Description', help="Notes on this matrix"),   # WHY IS THIS FIELD NOT BEING ADDED?????
        'setuprule_lines': fields.one2many('frepple.setuprule', 'setupmatrix_id', 'Setup rules'),
    }
    _defaults = {
        'active': lambda *a: 1,
        'name': lambda x,y,z,c: x.pool.get('ir.sequence').get(y,z,'frepple.setupmatrix') or '',
    }

    def copy(self, cr, uid, id, default=None,context={}):
        if not default:
            default = {}
        default.update({
            'name': self.pool.get('ir.sequence').get(cr, uid, 'frepple.setupmatrix') or '',
        })
        return super(frepple_setupmatrix, self).copy(cr, uid, id, default, context)
frepple_setupmatrix()


class frepple_setuprule(osv.osv):
    _name = "frepple.setuprule"
    _description = "Setup rule"
    _columns = {
        'fromsetup': fields.char('From setup', size=32, required=True),
        'tosetup': fields.char('To setup', size=32, required=True),
        'active': fields.boolean('Active'),
        'setupmatrix_id': fields.many2one('frepple.setupmatrix', 'Setup Matrix', select=True, ondelete="cascade"),
        'duration': fields.float('Duration', required=True,
            help="Number of working hours taken for this conversion."),
        'cost': fields.float('Cost', required=True,
            help="Cost involved in this conversion."),
        'priority': fields.integer('Priority', required=True,
            help="The order of this rule in its matrix."),
    }
    _defaults = {
        'fromsetup': lambda *a: '*',
        'tosetup': lambda *a: '*',
        'active': lambda *a: 1,
        'duration': lambda *a: 0,
        'priority': lambda *a: 1,
        'cost': lambda *a: 0,
        'name': lambda x,y,z,c: x.pool.get('ir.sequence').get(y,z,'frepple.setuprule') or '',
    }
frepple_setuprule()


class frepple_product(osv.osv):
    _name = 'product.template'
    _inherit = 'product.template'
    _description = 'frePPLe extension of a product'
    _columns = {
        'frepple_field1': fields.char('Name', size=40),
    }
    _defaults = {
        'frepple_field1': lambda *a: "turbo",
    }
frepple_product()

# vim:expandtab:smartindent:tabstop=4:softtabstop=4:shiftwidth=4:
