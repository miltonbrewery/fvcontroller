from django.conf.urls import patterns, include, url

from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('',
    # Examples:
    # url(r'^$', 'fvlogging.views.home', name='home'),
    # url(r'^fvlogging/', include('fvlogging.foo.urls')),

    url(r'^$', 'fvlogging.views.frontpage'),

    url(r'^datalog/', include('datalog.urls')),

    url(r'^admin/doc/', include('django.contrib.admindocs.urls')),
    url(r'^admin/', include(admin.site.urls)),
)
