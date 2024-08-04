# -*- coding: utf-8 -*-

from django.core.management.base import BaseCommand
from datalog.models import Controller
import django.utils.timezone


class Command(BaseCommand):
    def handle(self, *args, **options):
        now = django.utils.timezone.now()
        for c in Controller.objects.all():
            # Check all the non-config registers
            for r in c.register_set.filter(config=False):
                if r.future_time and r.future_time <= now:
                    r.set(r.future_value)
                    r.future_time = None
                    r.future_value = None
                    r.save()
                else:
                    r.value()
