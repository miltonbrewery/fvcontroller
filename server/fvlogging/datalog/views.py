# Create your views here.

from django.http import HttpResponse,HttpResponseRedirect,Http404
from django.template import RequestContext
from django.shortcuts import render_to_response
from datalog.models import *
import csv

def summary(request):
    controllers=Controller.objects.all()
    return render_to_response('datalog/summary.html',
                              {'controllers':controllers,},
                              context_instance=RequestContext(request))

def detail(request,name,config=False):
    try:
        controller=Controller.objects.get(ident=name)
    except:
        raise Http404
    registers=controller.register_set.filter(config=config)
    if request.method=='POST':
        # Go through the registers looking for ones that have a value
        # set in the request, and update as appropriate.
        for r in registers:
            if request.POST.has_key(r.name) and request.POST[r.name]:
                r.set(request.POST[r.name])
        return HttpResponseRedirect("")
    return render_to_response('datalog/detail.html',
                              {'controller':controller,
                               'registers':registers,
                               'config':config,},
                              context_instance=RequestContext(request))

def series_csv(request,name,register):
    try:
        controller=Controller.objects.get(ident=name)
    except:
        raise Http404
    try:
        register=controller.register_set.get(name=register)
    except:
        raise Http404
    dt=DATATYPE_DICT[register.datatype]
    series=dt.objects.filter(register=register).order_by('timestamp')
    r=HttpResponse(mimetype="text/csv")
    r['Content-Disposition']='attachment; filename=%s-%s.csv'%(
        controller.ident,register.name.replace('/','-'))
    c=csv.writer(r)
    for d in series:
        c.writerow((d.timestamp.strftime("%Y-%m-%d %H:%M:%S"),d.data))
    return r
