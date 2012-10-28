from django.conf.urls.defaults import *

urlpatterns=patterns(
    'datalog.views',

    # Main index page
    (r'^$','summary'),
    url(r'^detail/(?P<name>\w+)/$','detail',name="datalog-controller"),
    (r'^detail/(?P<name>\w+)/config/$','detail',{'config':True}),
)
