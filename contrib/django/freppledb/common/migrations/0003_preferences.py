#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from django.db import models, migrations
from django.conf import settings
import freppledb.common.fields


class Migration(migrations.Migration):

    dependencies = [
        ('common', '0002_defaultuser'),
    ]

    operations = [
        migrations.CreateModel(
            name='UserPreference',
            fields=[
                ('id', models.AutoField(verbose_name='identifier', serialize=False, primary_key=True)),
                ('property', models.CharField(max_length=60)),
                ('value', freppledb.common.fields.JSONField(max_length=1000)),
                ('user', models.ForeignKey(verbose_name='user', editable=False, related_name='preferences', to=settings.AUTH_USER_MODEL)),
            ],
            options={
                'verbose_name': 'preference',
                'verbose_name_plural': 'preferences',
                'db_table': 'common_preference',
            },
        ),
        migrations.AlterUniqueTogether(
            name='userpreference',
            unique_together=set([('user', 'property')]),
        ),
    ]
