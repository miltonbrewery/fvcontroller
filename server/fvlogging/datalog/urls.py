from django.conf.urls.defaults import *

urlpatterns=patterns(
    'datalog.views',

    # Main index page
    (r'^$','summary'),
)
