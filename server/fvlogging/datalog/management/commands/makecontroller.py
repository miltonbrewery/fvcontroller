# -*- coding: utf-8 -*-

from django.core.management.base import BaseCommand, CommandError
from datalog.models import Controller,Register

# Name, description, datatype, unit, readonly, max_interval, config
regs=[
    ('t0','Temperature','F',u'°C',True,30,False),
    ('v0','Valve state','S','',True,30,False),
    ('mode','Mode','S','',False,30,False),
    ('set/lo','Low set point','F',u'°C',False,30,False),
    ('set/hi','High set point','F',u'°C',False,30,False),
    ('ver','Firmware version','S','',True,300,True),
    ('flashcnt','Firmware reflash count','I','',True,300,True),
    ('bl','Backlight timeout','I','cs',False,300,True),
    ('t0/id','Probe ID','S','',False,300,True),
    ('m0/name','Mode 0 name','S','',False,300,True),
    ('m0/lo','Mode 0 low set point','S','',False,300,True),
    ('m0/hi','Mode 0 high set point','S','',False,300,True),
    ('m1/name','Mode 1 name','S','',False,300,True),
    ('m1/lo','Mode 1 low set point','S','',False,300,True),
    ('m1/hi','Mode 1 high set point','S','',False,300,True),
    ('m2/name','Mode 2 name','S','',False,300,True),
    ('m2/lo','Mode 2 low set point','S','',False,300,True),
    ('m2/hi','Mode 2 high set point','S','',False,300,True),
    ('m3/name','Mode 3 name','S','',False,300,True),
    ('m3/lo','Mode 3 low set point','S','',False,300,True),
    ('m3/hi','Mode 3 high set point','S','',False,300,True),
]

class Command(BaseCommand):
    args='controllername'
    def handle(self,*args,**options):
        if len(args)<1:
            print "Error: needs a controller name"
            return
        cname=args[0]

        try:
            controller=Controller.objects.get(ident=cname)
        except:
            controller=Controller(ident=cname,description=cname,
                                  address="localhost",port=1576,active=True)
            controller.save()
        for name,desc,datatype,unit,readonly,timeout,config in regs:
            try:
                reg=Register.objects.get(controller=controller,name=name)
            except Register.DoesNotExist:
                print "Creating %s..."%name
                reg=Register(controller=controller,name=name,description=desc,
                             datatype=datatype,unit=unit,readonly=readonly,
                             max_interval=timeout,config=config)
                reg.save()
