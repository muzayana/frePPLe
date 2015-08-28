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
        ('common', '0001_initial'),
    ]

    operations = [
        migrations.CreateModel(
            name='Task',
            fields=[
                ('id', models.AutoField(verbose_name='identifier', primary_key=True, editable=False, serialize=False)),
                ('name', models.CharField(max_length=20, db_index=True, verbose_name='name', editable=False)),
                ('submitted', models.DateTimeField(verbose_name='submitted', editable=False)),
                ('started', models.DateTimeField(null=True, blank=True, verbose_name='started', editable=False)),
                ('finished', models.DateTimeField(null=True, blank=True, verbose_name='submitted', editable=False)),
                ('arguments', models.TextField(null=True, max_length=200, verbose_name='arguments', editable=False)),
                ('status', models.CharField(max_length=20, verbose_name='status', editable=False)),
                ('message', models.TextField(null=True, max_length=200, verbose_name='message', editable=False)),
                ('user', models.ForeignKey(blank=True, verbose_name='user', editable=False, null=True, to=settings.AUTH_USER_MODEL)),
            ],
            options={
                'db_table': 'execute_log',
                'verbose_name': 'task',
                'verbose_name_plural': 'tasks',
            },
        ),
    ]
