#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
from django.conf import settings
from django.core.management import call_command
from django.db import models, migrations


def loadParameters(apps, schema_editor):
  call_command('loaddata', "parameters.json", app_label="planningboard", verbosity=0)


class Migration(migrations.Migration):

    dependencies = [
        migrations.swappable_dependency(settings.AUTH_USER_MODEL),
    ]

    operations = [
        migrations.CreateModel(
            name='Chat',
            fields=[
                ('id', models.AutoField(primary_key=True, verbose_name='identifier', serialize=False)),
                ('message', models.TextField(verbose_name='message', max_length=3000)),
                ('lastmodified', models.DateTimeField(verbose_name='last modified', null=True, auto_now=True, db_index=True)),
                ('user', models.ForeignKey(related_name='chat', verbose_name='user', editable=False, to=settings.AUTH_USER_MODEL)),
            ],
            options={
                'ordering': ('id',),
                'verbose_name_plural': 'chat history',
                'db_table': 'planningboard_chat',
                'verbose_name': 'chat history',
            },
        ),
        migrations.RunPython(loadParameters),
    ]
