#
# Copyright (C) 2014 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from django.db import migrations


class AttributeMigration(migrations.Migration):
  '''
  This migration subclass allows a migration in application X to change
  a model defined in application Y.
  This is useful to extend models in application Y with custom fields.

  By default we are extending the 'input' app. You can set extends_app_label
  in your migration subclass.
  '''

  # Application in which we are extending the models.
  extends_app_label = 'input'

  def __init__(self, name, app_label):
    # Make the migration believe that it's running in the "input" app.
    # This is required to make changes to models from that app.
    super(AttributeMigration, self).__init__(name, self.extends_app_label)
    self.my_app_label = app_label
    self.app_label = self.extends_app_label

  def apply(self, project_state, schema_editor, collect_sql=False):
    super(AttributeMigration, self).apply(project_state, schema_editor, collect_sql)
    # After applying the changes, we register the changes as a migration
    # that is owned by the current app, rather the "extends_app_label" app.
    self.app_label = self.my_app_label

  def unapply(self, project_state, schema_editor, collect_sql=False):
    super(AttributeMigration, self).unapply(project_state, schema_editor, collect_sql)
    # After unapplying the changes, we make django believe that this migration
    # is owned by the current app, rather the "extends_app_label" app it extends.
    self.app_label = self.my_app_label
