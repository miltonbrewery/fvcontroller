from django.http import HttpResponse
from django.http import HttpResponseRedirect
from django.http import Http404
from django.template import RequestContext
from django.shortcuts import render
from django import forms
from django.urls import reverse
from datalog.models import *
import datetime
import csv
import xml.etree.ElementTree as ET

def summary(request):
    controllers = Controller.objects.all()
    registers = Register.objects.filter(frontpage=True)\
                                .order_by("description")\
                                .all()
    return render(request, 'datalog/summary.html',
                  context={'controllers': controllers,
                           'registers': registers})

def detail(request, name, config=False):
    try:
        controller = Controller.objects.get(ident=name)
    except Controller.DoesNotExist:
        raise Http404
    registers = controller.register_set.filter(config=config)
    extraseries = Register.objects.exclude(graphcolour_all="")

    if request.method == 'POST':
        # Go through the registers looking for ones that have a value
        # set in the request, and update as appropriate.
        for r in registers:
            if r.name in request.POST and request.POST[r.name]:
                r.set(request.POST[r.name])
        return HttpResponseRedirect(
            reverse('datalog-controller-config' if config
                    else 'datalog-controller',
                    args=(name,)))

    return render(request, 'datalog/detail.html',
                  context={'controller': controller,
                           'registers': registers,
                           'extraseries': extraseries,
                           'config': config,
                  })

class GraphPeriodForm(forms.Form):
    start = forms.DateTimeField()
    end = forms.DateTimeField()
    def clean(self):
        s = None
        e = None
        try:
            s = self.cleaned_data['start']
            e = self.cleaned_data['end']
        except:
            pass
        if s and e and e <= s:
            raise forms.ValidationError("Start must be before end")
        return self.cleaned_data

def detailgraph(request, name, start=None, end=None):
    try:
        controller = Controller.objects.get(ident=name)
    except Controller.DoesNotExist:
        raise Http404
    extraseries = Register.objects.exclude(graphcolour_all="")
    if start == None or end == None:
        end = datetime.datetime.now().replace(microsecond=0)
        start = end - datetime.timedelta(days=7)
    else:
        start = parsedatetime(start)
        end = parsedatetime(end)

    if request.method == 'POST':
        form = GraphPeriodForm(request.POST)
        if form.is_valid():
            cd = form.cleaned_data
            start = cd['start']
            end = cd['end']
            return HttpResponseRedirect(
                reverse('datalog-detailgraph-period',
                        args=(name, str(start), str(end))))
    else:
        form = GraphPeriodForm(initial={
            'start': start,
            'end': end,
        })

    period = end - start

    return render(request, 'datalog/detailgraph.html',
                  context={'controller': controller,
                           'extraseries': extraseries,
                           'form': form,
                           'start': str(start),
                           'end': str(end),
                           'period': period,
                           'back': str(start - period),
                           'forward': str(end + period),
                  })

def series_csv(request, name, register):
    try:
        controller=Controller.objects.get(ident=name)
    except Controller.DoesNotExist:
        raise Http404
    try:
        register = controller.register_set.get(name=register)
    except Register.DoesNotExist:
        raise Http404
    dt = DATATYPE_DICT[register.datatype]
    series = dt.objects.filter(register=register).order_by('timestamp')
    r = HttpResponse(content_type="text/csv")
    r['Content-Disposition'] = 'attachment; filename=%s-%s.csv' % (
        controller.ident, register.name.replace('/', '-'))
    c = csv.writer(r)
    for d in series:
        c.writerow((d.timestamp.strftime("%Y-%m-%d %H:%M:%S"), d.data))
    return r

def parsedatetime(d):
    """Convert a string to a datetime object

    Tries several possible format strings in order.  Returns the first
    match, or None.
    """
    formats = ('%Y-%m-%d %H:%M:%S', '%Y-%m-%d %H:%M', '%Y-%m-%d')
    for f in formats:
        try:
            return datetime.datetime.strptime(d, f)
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
    width = int(request.GET.get('width', 800))
    height = int(request.GET.get('height', 600))
    leftmargin = int(request.GET.get('leftmargin', 100))
    bottommargin = int(request.GET.get('bottommargin', 100))
    floatmin = float(request.GET.get('floatmin', 0.0))
    floatmax = float(request.GET.get('floatmax', 30.0))
    end = None
    if 'end' in request.GET:
        end = parsedatetime(request.GET['end'])
    if end is None:
        end = datetime.datetime.now()
    start = None
    if 'start' in request.GET:
        start = parsedatetime(request.GET['start'])
        if start is None:
            try:
                start = end - datetime.timedelta(days=int(request.GET['start']))
            except ValueError:
                pass
    if start is None:
        start = end - datetime.timedelta(days=30)

    length = (end - start).total_seconds()
    series = request.GET.getlist('series')

    # Origin is in top-left by default.
    svg = ET.Element("svg", {'xmlns': 'http://www.w3.org/2000/svg',
                             'version': '1.1',
                             'width': str(width),
                             'height': str(height)})
    ET.SubElement(svg, "title").text = "A Graph"
    ET.SubElement(svg, "desc").text = "A longer description of the group"
    # Bounding box
    ET.SubElement(svg, "rect", x="0", y="0",
                  width=str(width), height=str(height),
                  fill="none", stroke="blue")

    # Let's create a group with a transformed coordinate space such
    # that the origin is in the bottom-left
    g = ET.SubElement(svg, "g", transform="translate(%d,%d) scale(1,-1)" % (
        leftmargin, height - bottommargin))
    
    # We now can draw axes, etc. from 0 - (width - leftmargin) on X and 0 - (height - bottommargin) on y
    graphwidth = width - leftmargin
    graphheight = height - bottommargin

    # Draw horizontal scale lines for temperature
    y = 0.0
    while y < floatmax:
        scaley = (y - floatmin) * graphheight / floatmax
        ET.SubElement(g, "line", x1="0", x2=str(width),
                      y1=str(scaley), y2=str(scaley),
                      stroke="lightgrey")
        y = y + 1.0
    y = 0.0
    while y < floatmax:
        scaley = (y - floatmin) * graphheight / floatmax
        ET.SubElement(g, "line", x1="0", x2=str(width),
                      y1=str(scaley), y2=str(scaley),
                      stroke="darkgrey")
        y = y + 5.0

    for s in series:
        controller, register, colour = s.split(":")
        controller = Controller.objects.get(ident=controller)
        register = Register.objects.get(controller=controller, name=register)
        # Retrieve all datapoints between start and end
        dt = DATATYPE_DICT[register.datatype]
        # We want to find the timestamps of the first datapoint before
        # start (if there is one) and the first datapoint after end
        # (if there is one), and adjust start and end to include
        # these.  This will avoid the graph having blank sections at
        # the left and right hand sides.
        sdp = dt.objects.filter(register=register, timestamp__lt=start)\
                        .order_by('-timestamp')[:1]
        s_start = sdp[0].timestamp if len(sdp) == 1 else start
        edp = dt.objects.filter(register=register,timestamp__gt=end)\
                        .order_by('timestamp')[:1]
        s_end = edp[0].timestamp if len(edp) == 1 else end
        datapoints = dt.objects.filter(register=register, timestamp__lte=s_end,
                                       timestamp__gte=s_start)\
                               .order_by('timestamp')
        now_drawing = False
        dl = []
        for datum in datapoints:
            x = (datum.timestamp - start).total_seconds() * graphwidth / length
            if datum.data is None:
                now_drawing = False
            else:
                # Let's assume float for now
                y = (datum.data - floatmin) * graphheight / floatmax
                dl.append("%s %f %f" % ('L' if now_drawing else 'M', x, y))
                now_drawing = True
        ET.SubElement(g, "path", stroke=colour, fill="none", d=" ".join(dl))

    r = HttpResponse(content_type="image/svg+xml")
    ET.ElementTree(svg).write(r, encoding="UTF-8", xml_declaration=True)
    return r
