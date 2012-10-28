from django.db import models
import socket
import time

class Controller(models.Model):
    """A controller that can be present on a RS485 bus.  Has a number
    of registers (defined in a separate model).  Accessed by
    connecting to a TCP port, sending a SELECT ident command, and
    using READ and SET commands.

    """
    ident=models.CharField(max_length=8)
    description=models.TextField()
    address=models.TextField()
    port=models.IntegerField()
    active=models.BooleanField()
    def connect(self):
        """Return a file-like connection to the RS485 bus, with this
        controller selected, or None if there is a failure.

        """
        if not self.active: return None
        s=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        try:
            s.connect((self.address,self.port))
        except:
            return None
        s.settimeout(1.5)
        s=s.makefile('rw')
        s.write("SELECT %s\n"%self.ident)
        s.flush()
        response=s.readline()
        if response!="OK %s selected\n"%self.ident:
            s.close()
            return None
        return s
    def read(self,register):
        """Read a register as a string.

        """
        s=self.connect()
        if not s: return None # Maybe raise exception instead?
        try:
            s.write("READ %s\n"%register)
            s.flush()
            response=s.readline().strip()
            if response[0:3]!="OK ": return None
            return response[3:]
        finally:
            s.close()
    def write(self,register,value):
        """Write a string to a register.

        """
        s=self.connect()
        if not s: return None # Exception?
        try:
            s.write("SET %s %s\n"%(register,value))
            s.flush()
            response=s.readline().strip()
            return response
        finally:
            s.close()
    def regs(self):
        """Return register set as a dict for use in templates.
        Templates don't allow the '/' character, so remove it.

        """
        return {x.name.replace('/',''):x for x in self.register_set.all()}
    def __unicode__(self):
        return self.ident
    @models.permalink
    def get_absolute_url(self):
        return ('datalog-controller',[self.ident])

class Datum(models.Model):
    class Meta:
        abstract=True
    register=models.ForeignKey("Register")
    timestamp=models.DateTimeField()

class StringDatum(Datum):
    data=models.TextField()
    def __unicode__(self): return self.data

class FloatDatum(Datum):
    data=models.FloatField()
    def __unicode__(self): return self.data

class IntegerDatum(Datum):
    data=models.IntegerField()
    def __unicode__(self): return self.data

DATATYPES=(
    ('S',StringDatum),
    ('F',FloatDatum),
    ('I',IntegerDatum),
)

class Register(models.Model):
    """A register found in a controller.  We log values found in these
    registers at regular intervals.

    """
    controller=models.ForeignKey(Controller)
    name=models.CharField(max_length=8)
    description=models.TextField()
    # Something to indicate the datatype - there will be classes for each one
    # descending from an abstract base class
    datatype=models.CharField(max_length=1,choices=DATATYPES)
    unit=models.CharField(max_length=10,null=True,blank=True)
    readonly=models.BooleanField()
    # If the most recent recorded value is older than this, read it
    # again from the hardware rather than the database
    max_interval=models.IntegerField() # In seconds
    config=models.BooleanField() # Is this a configuration register?
    def __unicode__(self):
        return self.name
    def value(self):
        return self.controller.read(self.name)
    def set(self,value):
        return self.controller.write(self.name,value)
        
