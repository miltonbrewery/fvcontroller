from datalog.models import *
from django.contrib import admin

admin.site.register(Controller)

class RegisterAdmin(admin.ModelAdmin):
    list_display=('controller','name','description','graphcolour','config')
    list_filter=('controller','config')

admin.site.register(Register,RegisterAdmin)
#admin.site.register(NoteType)
