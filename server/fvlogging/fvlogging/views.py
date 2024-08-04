from django.http import HttpResponseRedirect


def frontpage(request):
    return HttpResponseRedirect("/datalog/")
