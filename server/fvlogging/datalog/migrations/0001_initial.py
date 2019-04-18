# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='Controller',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('ident', models.CharField(max_length=8)),
                ('description', models.TextField()),
                ('address', models.TextField()),
                ('port', models.IntegerField()),
                ('active', models.BooleanField()),
            ],
        ),
        migrations.CreateModel(
            name='FloatDatum',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('timestamp', models.DateTimeField()),
                ('data', models.FloatField(null=True)),
            ],
            options={
                'abstract': False,
            },
        ),
        migrations.CreateModel(
            name='IntegerDatum',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('timestamp', models.DateTimeField()),
                ('data', models.IntegerField(null=True)),
            ],
            options={
                'abstract': False,
            },
        ),
        migrations.CreateModel(
            name='Note',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('timestamp', models.DateTimeField()),
                ('data', models.TextField()),
                ('controller', models.ForeignKey(to='datalog.Controller',
                                                 on_delete=models.CASCADE)),
            ],
            options={
                'ordering': ['-timestamp'],
            },
        ),
        migrations.CreateModel(
            name='NoteType',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('desc', models.TextField()),
            ],
        ),
        migrations.CreateModel(
            name='Register',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('name', models.CharField(max_length=8)),
                ('description', models.TextField()),
                ('datatype', models.CharField(max_length=1, choices=[('S', 'StringDatum'), ('F', 'FloatDatum'), ('I', 'IntegerDatum')])),
                ('unit', models.CharField(max_length=10, null=True, blank=True)),
                ('readonly', models.BooleanField(help_text='Can value not be set on controller?')),
                ('max_interval', models.IntegerField()),
                ('config', models.BooleanField()),
                ('frontpage', models.BooleanField(help_text='Show this register on the site front page?')),
                ('graphcolour', models.CharField(help_text="Colour of trace on controller's default graph, or blank to leave out", max_length=20, blank=True)),
                ('graphcolour_all', models.CharField(help_text='Colour of trace on all graphs, or blank to leave out', max_length=20, blank=True)),
                ('controller', models.ForeignKey(to='datalog.Controller',
                                                 on_delete=models.PROTECT)),
            ],
        ),
        migrations.CreateModel(
            name='StringDatum',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('timestamp', models.DateTimeField()),
                ('data', models.TextField(null=True)),
                ('register', models.ForeignKey(to='datalog.Register',
                                               on_delete=models.PROTECT)),
            ],
            options={
                'abstract': False,
            },
        ),
        migrations.AddField(
            model_name='note',
            name='type',
            field=models.ForeignKey(to='datalog.NoteType', on_delete=models.CASCADE),
        ),
        migrations.AddField(
            model_name='integerdatum',
            name='register',
            field=models.ForeignKey(to='datalog.Register', on_delete=models.PROTECT),
        ),
        migrations.AddField(
            model_name='floatdatum',
            name='register',
            field=models.ForeignKey(to='datalog.Register', on_delete=models.PROTECT),
        ),
    ]
