from django.db import models
from django.urls import reverse
import socket
import datetime
import django.utils.timezone
now = django.utils.timezone.now


class Controller(models.Model):
    """A controller that can be present on a RS485 bus.

    Has a number of registers (defined in a separate model).  Accessed
    by connecting to a TCP port, sending a SELECT ident command, and
    using READ and SET commands.
    """
    ident = models.CharField(max_length=8)
    description = models.TextField()
    address = models.TextField()
    port = models.IntegerField()
    active = models.BooleanField()

    def connect(self):
        """Connect to this controller.

        Returns a file-like connection to the RS485 bus, with this
        controller selected, or None if there is a failure.
        """
        if not self.active:
            return None
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            s.connect((self.address, self.port))
        except Exception:
            return None
        s.settimeout(1.5)
        sf = s.makefile('rw')
        s.close()
        sf.write(f"SELECT {self.ident}\n")
        sf.flush()
        response = sf.readline()
        if response != f"OK {self.ident} selected\n":
            sf.close()
            return None
        return sf

    def read(self, register):
        """Read a register as a string.
        """
        s = self.connect()
        if not s:
            return None  # Maybe raise exception instead?
        try:
            s.write("READ %s\n" % register)
            s.flush()
            response = s.readline().strip()
            if response[0:3] != "OK ":
                return None
            return response[3:]
        finally:
            s.close()

    def write(self, register, value):
        """Write a string to a register.
        """
        s = self.connect()
        if not s:
            return None  # Exception?
        try:
            s.write("SET %s %s\n" % (register, value))
            s.flush()
            response = s.readline().strip()
            return response
        finally:
            s.close()

    def regs(self):
        """Return register set as a dict for use in templates.

        Templates don't allow the '/' character, so remove it.
        """
        return {x.name.replace('/', ''): x for x in self.register_set.all()}

    def __str__(self):
        return self.ident

    def get_absolute_url(self):
        return reverse('datalog-controller', args=[self.ident])

    class Meta:
        ordering = ['id']


class Datum(models.Model):
    class Meta:
        abstract = True
    register = models.ForeignKey("Register", on_delete=models.PROTECT)
    timestamp = models.DateTimeField()

    @staticmethod
    def cast(value):
        return value


class StringDatum(Datum):
    data = models.TextField(null=True)

    def __str__(self):
        return self.data


class FloatDatum(Datum):
    data = models.FloatField(null=True)

    def __str__(self):
        if self.data:
            return "%0.2f" % self.data
        return "None"

    @staticmethod
    def cast(value):
        if value == "None":
            return None
        return float(value)


class IntegerDatum(Datum):
    data = models.IntegerField(null=True)

    def __str__(self):
        if self.data:
            return "%d" % self.data
        return "None"

    @staticmethod
    def cast(value):
        if value == "None":
            return None
        return int(value)


DATATYPES = (
    ('S', "StringDatum"),
    ('F', "FloatDatum"),
    ('I', "IntegerDatum"),
)

DATATYPE_DICT = {
    'S': StringDatum,
    'F': FloatDatum,
    'I': IntegerDatum,
}


class Register(models.Model):
    """A register found in a controller.

    We log values found in these registers at regular intervals.
    """
    controller = models.ForeignKey(Controller, on_delete=models.PROTECT)
    name = models.CharField(max_length=8)
    description = models.TextField()
    # Something to indicate the datatype - there are classes for each one
    # descending from an abstract base class
    datatype = models.CharField(max_length=1, choices=DATATYPES)
    unit = models.CharField(max_length=10, null=True, blank=True)
    readonly = models.BooleanField(
        help_text="Can value not be set on controller?")
    # Set a value after a time in the future
    future_value = models.CharField(max_length=20, blank=True, null=True)
    future_time = models.DateTimeField(blank=True, null=True)
    # If the most recent recorded value is older than this, read it
    # again from the hardware rather than the database
    max_interval = models.IntegerField()  # In seconds
    config = models.BooleanField()  # Is this a configuration register?
    frontpage = models.BooleanField(
        help_text="Show this register on the site front page?")
    graphcolour = models.CharField(
        max_length=20, blank=True,
        help_text="Colour of trace on controller's default graph, or "
        "blank to leave out")
    graphcolour_all = models.CharField(
        max_length=20, blank=True,
        help_text="Colour of trace on all graphs, or blank to leave out")

    def __str__(self):
        return "%s %s" % (self.controller, self.name)

    def value(self, force_check=False):
        # Read most recent (up to) two datapoints.
        dt = DATATYPE_DICT[self.datatype]
        dpl = dt.objects.filter(register=self).order_by('-timestamp')[:2]
        # If there are zero datapoints, we always record a new one.
        # Otherwise, we check to see how old the most recent datapoint
        # is, and consider recording a new one if it is more than
        # max_interval seconds old.
        if len(dpl) == 0 or force_check or (
                (now() - dpl[0].timestamp)
                > datetime.timedelta(seconds=self.max_interval)):
            r = self.controller.read(self.name)
            if not r:
                # Reading from the hardware failed.  We return the most
                # recent value if there is one, or None.
                if len(dpl) > 0:
                    return dpl[0]
                else:
                    return None
            val = dt.cast(r)
            if len(dpl) > 0 and val == dpl[0].data and len(dpl) == 2 \
               and dpl[0].data == dpl[1].data:
                # No change, and we already have two datapoints in a
                # row with this value.  We just update the timestamp
                # on the most recent.
                dpl[0].timestamp = now()
                dpl[0].save()
                dp = dpl[0]
            else:
                # We record a new datapoint.
                dp = dt(register=self, timestamp=now(), data=val)
                dp.save()
        else:
            # Return the most recent datapoint - it isn't time to check
            # the hardware yet
            dp = dpl[0]
        return dp

    def last_change(self):
        """When did the value last change?

        Does not read from the hardware; reports state solely from the
        database.
        """
        # Read most recent (up to) two datapoints.
        dt = DATATYPE_DICT[self.datatype]
        dpl = dt.objects.filter(register=self).order_by('-timestamp')[:2]
        if len(dpl) == 0:
            return
        elif len(dpl) == 1:
            return dpl[0].timestamp
        else:
            if dpl[0].data == dpl[1].data:
                return dpl[1].timestamp
            return dpl[0].timestamp

    def set(self, value):
        """Set a new value for this register

        Errors are currently ignored and the register value does not change.
        This could occur if the hardware is offline, for example.
        """
        rv = self.controller.write(self.name, value)
        # After setting a register we force a readback from the hardware,
        # ignoring the timestamp on the most recently recorded datapoint.
        self.value(force_check=True)
        return rv

    class Meta:
        ordering = ['id']


class NoteType(models.Model):
    desc = models.TextField()


class Note(models.Model):
    controller = models.ForeignKey(Controller, on_delete=models.CASCADE)
    type = models.ForeignKey(NoteType, on_delete=models.CASCADE)
    timestamp = models.DateTimeField()
    data = models.TextField()

    class Meta:
        ordering = ['-timestamp']
