from django.conf.urls import patterns, url, include

urlpatterns=patterns(
    'datalog.views',

    # Main index page
    (r'^$','summary'),
    url(r'^detail/(?P<name>\w+)/$','detail',name="datalog-controller"),
    url(r'^detail/(?P<name>\w+)/config/$','detail',{'config':True}),
    url(r'^detail/(?P<name>\w+)/graph/$','detailgraph'),
    url(r'^detail/(?P<name>\w+)/graph/(?P<start>\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d)--(?P<end>\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d)$',
        'detailgraph'),
    url(r'^series/(?P<name>\w+)-(?P<register>\w+/?\w*).csv$','series_csv',
        name="datalog-csvfile"),
    url(r'^graph.svg','graph',name="datalog-graph"),
)
