# -*- coding: utf-8 -*-

from django.core.management.base import BaseCommand
from datalog.models import Controller, Register

modes = 4

# Name, description, datatype, unit, readonly, max_interval, config
regs = [
    ('t0', 'Temperature', 'F', '°C', True, 30, False),
    ('v0', 'Valve state', 'S', '', True, 30, False),
    ('mode', 'Mode', 'S', '', False, 30, False),
    ('alarm', 'Alarm', 'S', '', True, 30, False),
    ('set/lo', 'Low set point', 'F', '°C', False, 30, False),
    ('set/hi', 'High set point', 'F', '°C', False, 30, False),
    ('alarm/lo', 'Alarm low set point', 'F', '°C', False, 30, False),
    ('alarm/hi', 'Alarm high set point', 'F', '°C', False, 30, False),
    ('jog/lo', 'Stuck valve low set point', 'F', '°C', False, 30, False),
    ('jog/hi', 'Stuck valve high set point', 'F', '°C', False, 30, False),
    ('ver', 'Firmware version', 'S', '', True, 300, True),
    ('flashcnt', 'Firmware reflash count', 'I', '', True, 300, True),
    ('bl', 'Backlight timeout', 'I', 'cs', False, 300, True),
    ('bl/alarm', 'Alarm flash period', 'I', 'cs', False, 300, True),
    ('jog/flip', 'How long to blip valve to unstick it', 'I', 'cs',
     False, 300, True),
    ('jog/wait', 'Time between attempts to unstick valve', 'I', 'cs',
     False, 300, True),
    ('t0/id', 'Probe ID', 'S', '', False, 300, True),
]

moderegs = [
    ('m{mnum}/name', 'Mode {mnum} name'),
    ('m{mnum}/lo', 'Mode {mnum} low set point'),
    ('m{mnum}/hi', 'Mode {mnum} high set point'),
    ('m{mnum}/a/lo', 'Mode {mnum} alarm low set point'),
    ('m{mnum}/a/hi', 'Mode {mnum} alarm high set point'),
    ('m{mnum}/j/lo', 'Mode {mnum} stuck valve low set point'),
    ('m{mnum}/j/hi', 'Mode {mnum} stuck valve high set point'),
]

allregs = regs

for mnum in range(0, modes):
    for name, desc in moderegs:
        allregs.append((name.format(mnum=mnum), desc.format(mnum=mnum),
                        'S', '', False, 300, True))


class Command(BaseCommand):
    args = 'controllername'

    def handle(self, *args, **options):
        if len(args) < 1:
            print("Error: needs a controller name")
            return
        cname = args[0]

        try:
            controller = Controller.objects.get(ident=cname)
        except Controller.DoesNotExist:
            controller = Controller(
                ident=cname, description=cname,
                address="localhost", port=1576, active=True)
            controller.save()

        for name, desc, datatype, unit, readonly, timeout, config in allregs:
            try:
                reg = Register.objects.get(controller=controller, name=name)
                print("%s exists" % name)
            except Register.DoesNotExist:
                print("%s does not exist; creating..." % name)
                reg = Register(
                    controller=controller, name=name, description=desc,
                    datatype=datatype, unit=unit, readonly=readonly,
                    max_interval=timeout, config=config,
                    frontpage=False)
                reg.save()
