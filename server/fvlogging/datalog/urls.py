from django.conf.urls.defaults import *

urlpatterns=patterns(
    'datalog.views',

    # Main index page
    (r'^$','summary'),
    url(r'^detail/(?P<name>\w+)/$','detail',name="datalog-controller"),
    (r'^detail/(?P<name>\w+)/config/$','detail',{'config':True}),
    url(r'^series/(?P<name>\w+)-(?P<register>\w+/?\w*).csv$','series_csv',
        name="datalog-csvfile"),
)
