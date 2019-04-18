from django.conf.urls import include, url
from django.contrib import admin
import django.contrib.admindocs.urls
import fvlogging.views
import datalog.urls
admin.autodiscover()

urlpatterns = [
    # Examples:
    # url(r'^$', 'fvlogging.views.home', name='home'),
    # url(r'^fvlogging/', include('fvlogging.foo.urls')),

    url(r'^$', fvlogging.views.frontpage),

    url(r'^datalog/', include(datalog.urls.urlpatterns)),

    url(r'^admin/doc/', include(django.contrib.admindocs.urls)),
    url(r'^admin/', include(admin.site.urls)),
]
