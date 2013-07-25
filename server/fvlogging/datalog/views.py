# Create your views here.

from django.http import HttpResponse,HttpResponseRedirect,Http404
from django.template import RequestContext
from django.shortcuts import render_to_response
from datalog.models import *
import datetime
import csv
import xml.etree.ElementTree as ET

def summary(request):
    controllers=Controller.objects.all()
    registers=Register.objects.filter(frontpage=True).order_by("description").all()
    return render_to_response('datalog/summary.html',
                              {'controllers':controllers,
                               'registers':registers},
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

def parsedatetime(d):
    """Convert a string to a datetime object, trying several possible
    format strings in order.  Returns the first match, or None.

    """
    formats=('%Y-%m-%d %H:%M:%S','%Y-%m-%d %H:%M','%Y-%m-%d')
    for f in formats:
        try:
            return datetime.datetime.strptime(d,f)
        except ValueError:
            pass
    return None

def graph(request):
    """Draw a graph.  Time will always be along the x axis.

    The request can have the following parameters:
    width - width of returned image, defaults to 800
    height - height of returned image, defaults to 600
    end - end date and time in ISO format, defaults to now
    start - start date and time in ISO format (yyyy-mm-dd hh:mm:ss)
    (start defaults to end-1 day)
      - or a time difference
    title - title of graph
    series - a data series to plot, as follows:
      controller:register:colour
      
    """
    width=int(request.GET.get('width',800))
    height=int(request.GET.get('height',600))
    leftmargin=int(request.GET.get('leftmargin',100))
    bottommargin=int(request.GET.get('bottommargin',100))
    floatmin=float(request.GET.get('floatmin',0.0))
    floatmax=float(request.GET.get('floatmax',30.0))
    end=None
    if request.GET.has_key('end'):
        end=parsedatetime(request.GET['end'])
    if end is None: end=datetime.datetime.now()
    start=None
    if request.GET.has_key('start'):
        start=parsedatetime(request.GET['start'])
        if start is None:
            try:
                start=end-datetime.timedelta(days=int(request.GET['start']))
            except ValueError:
                pass
    if start is None:
        start=end-datetime.timedelta(days=30)

    length=(end-start).total_seconds()
    series=request.GET.getlist('series')

    # Origin is in top-left by default.
    svg=ET.Element("svg",{'xmlns':'http://www.w3.org/2000/svg',
                          'version':'1.1',
                          'width':str(width),'height':str(height)})
    ET.SubElement(svg,"title").text="A Graph"
    ET.SubElement(svg,"desc").text="A longer description of the group"
    # Bounding box
    ET.SubElement(svg,"rect",x="0",y="0",width=str(width),height=str(height),
                  fill="none",stroke="blue")

    # Let's create a group with a transformed coordinate space such
    # that the origin is in the bottom-left, 100 pixels in and 100
    # pixels from the bottom.
    g=ET.SubElement(svg,"g",transform="translate(%d,%d) scale(1,-1)"%(
            leftmargin,height-bottommargin))
    
    # We now can draw axes, etc. from 0-(width-leftmargin) on X and 0-(height-bottommargin) on y
    graphwidth=width-leftmargin
    graphheight=height-bottommargin

    for s in series:
        controller,register,colour=s.split(":")
        controller=Controller.objects.get(ident=controller)
        register=Register.objects.get(controller=controller,name=register)
        # Retrieve all datapoints between start and end
        dt=DATATYPE_DICT[register.datatype]
        datapoints=dt.objects.filter(register=register,timestamp__lte=end,
                                     timestamp__gte=start).order_by('timestamp')
        now_drawing=False
        dl=[]
        for datum in datapoints:
            x=(datum.timestamp-start).total_seconds()*graphwidth/length
            if datum.data is None:
                now_drawing=False
            else:
                # Let's assume float for now
                y=(datum.data-floatmin)*graphheight/floatmax
                dl.append("%s %f %f"%('L' if now_drawing else 'M',x,y))
                now_drawing=True
        ET.SubElement(g,"path",stroke=colour,fill="none",d=" ".join(dl))

    r=HttpResponse(mimetype="image/svg+xml")
    ET.ElementTree(svg).write(r,encoding="UTF-8",xml_declaration=True)
    return r
