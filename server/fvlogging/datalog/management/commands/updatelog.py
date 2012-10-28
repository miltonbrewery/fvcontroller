# -*- coding: utf-8 -*-

from django.core.management.base import BaseCommand, CommandError
from datalog.models import Controller,Register

class Command(BaseCommand):
    def handle(self,*args,**options):
        for c in Controller.objects.all():
            # Check all the non-config registers
            for r in c.register_set.filter(config=False):
                r.value()
