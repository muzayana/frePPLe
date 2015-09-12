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


class Migration(migrations.Migration):

    dependencies = [
        migrations.swappable_dependency(settings.AUTH_USER_MODEL),
    ]

    operations = [
        migrations.CreateModel(
            name='Task',
            fields=[
                ('id', models.AutoField(editable=False, primary_key=True, verbose_name='identifier', serialize=False)),
                ('name', models.CharField(editable=False, verbose_name='name', db_index=True, max_length=20)),
                ('submitted', models.DateTimeField(editable=False, verbose_name='submitted')),
                ('started', models.DateTimeField(null=True, editable=False, verbose_name='started', blank=True)),
                ('finished', models.DateTimeField(null=True, editable=False, verbose_name='submitted', blank=True)),
                ('arguments', models.TextField(null=True, editable=False, verbose_name='arguments', max_length=200)),
                ('status', models.CharField(editable=False, verbose_name='status', max_length=20)),
                ('message', models.TextField(null=True, editable=False, verbose_name='message', max_length=200)),
                ('user', models.ForeignKey(verbose_name='user', null=True, editable=False, blank=True, to=settings.AUTH_USER_MODEL)),
            ],
            options={
                'db_table': 'execute_log',
                'verbose_name': 'task',
                'verbose_name_plural': 'tasks',
            },
        ),
    ]
