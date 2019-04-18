from django.conf.urls import url, include
import datalog.views

urlpatterns = [
    # Main index page
    url(r'^$', datalog.views.summary),
    url(r'^detail/(?P<name>\w+)/$', datalog.views.detail,
        name="datalog-controller"),
    url(r'^detail/(?P<name>\w+)/config/$', datalog.views.detail,
        {'config':True}, name="datalog-controller-config"),
    url(r'^detail/(?P<name>\w+)/graph/$', datalog.views.detailgraph,
        name="datalog-detailgraph"),
    url(r'^detail/(?P<name>\w+)/graph/(?P<start>\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d)--(?P<end>\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d)$',
        datalog.views.detailgraph, name="datalog-detailgraph-period"),
    url(r'^series/(?P<name>\w+)-(?P<register>\w+/?\w*).csv$',
        datalog.views.series_csv, name="datalog-csvfile"),
    url(r'^graph.svg', datalog.views.graph, name="datalog-graph"),
]
