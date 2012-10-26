# Create your views here.

from django.http import HttpResponse,Http404
from django.template import RequestContext
from django.shortcuts import render_to_response
from datalog.models import *

def summary(request):
    controllers=Controller.objects.all()
    return render_to_response('datalog/summary.html',
                              {'controllers':controllers,},
                              context_instance=RequestContext(request))
