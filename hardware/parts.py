#!/usr/bin/env python

class NetMergeError(Exception):
    def __init__(self,desc):
        self.s=desc
    def __str__(self):
        return self.s

class NetAlreadyNamed(Exception):
    def __init__(self,orig,new):
        self.orig=orig
        self.new=new
    def __str__(self):
        return "original name %s, new name %s"%(self.orig,self.new)

class DuplicateNet(Exception):
    def __init__(self,name):
        self.name=name
    def __str__(self):
        return self.name

class Net(object):
    nets=set() # All nets
    netnames={} # Nets with names
    def __init__(self,name=None,style=None,defaultName=None,initialPin=None):
        self.nets.add(self)
        self._name=name
        self._style=style
        self._defaultName=defaultName
        self._connections=set()
        if initialPin: self._connections.add(initialPin)
    def add(self,net):
        """
        Used to add two nets together.  Fails if they have
        incompatible names or styles.  The net being added to this one
        is deleted.

        """
        self._name=self._name or net._name
        self._style=self._style or net._style
        self._defaultName=self._defaultName or net._defaultName
        if net._name and (self._name!=net._name):
            raise NetMergeError("merging nets %s and %s"%(self._name,net._name))
        if net._style and (self._style!=net._style):
            raise NetMergeError("merging styles %s and %s"%(self._style,net._style))
        self._connections|=net._connections
        Net.nets.remove(net)
    def setNetName(self,name):
        if self._name and (name!=self._name):
            raise NetAlreadyNamed(self._name,name)
        if name in Net.netnames:
            raise DuplicateNet(name)
        self._name=name
        Net.netnames[name]=self
    def setNetStyle(self,style):
        self._style=style
    def setNameFromDefault(self):
        if not self._defaultName: return
        if self._name: return
        if self._defaultName in Net.netnames: return
        self.setNetName(self._defaultName)
    def __unicode__(self):
        return u"Net(%s,%s,%s)"%(self._name,self._style,self._connections)
    @staticmethod
    def findPinNet(pinname,defaultNetName=None):
        # Look in all the registered nets.  If found, return the net.
        # If not, create a new net and add the pin to it before returning.
        for n in Net.nets:
            if pinname in n._connections: return n
        return Net(defaultName=defaultNetName,initialPin=pinname)
    @staticmethod
    def outputNets():
        for n in Net.nets:
            n.setNameFromDefault()
            if n._name is None: print "# Unnamed net:"
            print "%s %s\t%s"%(n._name,n._style if n._style else "Signal",
                               " ".join(n._connections))

class Pin(object):
    """
    A pin on a component.

    """
    def __init__(self,number=None,defaultNetName=None):
        self.number=number
        self.defaultNetName=defaultNetName
    def __get__(self,instance,owner):
        """
        When read as attributes, pins return the net they are part of.

        """
        return Net.findPinNet(u"%s-%s"%(instance.name,self.number),
                              self.defaultNetName)
    def __set__(self,instance,value):
        """
        When written to as attributes, pins add themselves to nets.

        """
        value.add(Net.findPinNet(u"%s-%s"%(instance.name,self.number),
                                 self.defaultNetName))

class DuplicateComponent(Exception):
    def __init__(self,name):
        self.name=name
    def __str__(self):
        return self.name

class Component(object):
    components={}
    def __init__(self,name,label=None,**kwargs):
        if name in Component.components:
            raise DuplicateComponent(name)
        self.name=name
        self.components[name]=self
        self.label=label
        for x in kwargs:
            setattr(self,x,kwargs[x])
    def __unicode__(self):
        return u"Component %s"%self.name

class TwoPinWithValue(Component):
    a=Pin(1)
    b=Pin(2)
    def __init__(self,ctype,value,*args,**kwargs):
        self.ctype=ctype
        self.value=value
        Component.__init__(self,*args,**kwargs)
    def __unicode__(self):
        return u"%s(%s,%s)"%(self.ctype,self.name,self.resistance)

class Resistor(TwoPinWithValue):
    def __init__(self,name,resistance,*args,**kwargs):
        TwoPinWithValue.__init__(self,"Resistor",resistance,name,*args,**kwargs)

class Varistor(Component):
    a=Pin(1)
    wiper=Pin(2)
    b=Pin(3)
    def __init__(self,name,value,*args,**kwargs):
        Component.__init__(self,name,*args,**kwargs)
        self.value=value

class Crystal(TwoPinWithValue):
    def __init__(self,name,freq,*args,**kwargs):
        TwoPinWithValue.__init__(self,"Crystal",freq,name,*args,**kwargs)

class Capacitor(TwoPinWithValue):
    def __init__(self,name,value,*args,**kwargs):
        TwoPinWithValue.__init__(self,"Capacitor",value,name,*args,**kwargs)

class PCapacitor(Component):
    pos=Pin(1)
    neg=Pin(2)
    def __init__(self,name,value,*args,**kwargs):
        Component.__init__(self,name,*args,**kwargs)
        self.value=value
    def __unicode__(self):
        return u"PCapacitor(%s,%s)"%(self.name,self.value)

class LatchRelay(Component):
    """
    12-pin DIL form factor (only 200mil wide) with pins 2 and 11 missing.

    """
    reset_neg=Pin(1)
    reset_pos=Pin(12)
    set_neg=Pin(6)
    set_pos=Pin(7)
    common_a=Pin(4)
    nc_a=Pin(3)
    no_a=Pin(5)
    common_b=Pin(9)
    nc_b=Pin(10)
    no_b=Pin(8)

class Connector2(Component):
    pin1=Pin(1)
    pin2=Pin(2)

class Connector4(Connector2):
    pin3=Pin(3)
    pin4=Pin(4)

class Connector6(Connector4):
    pin5=Pin(5)
    pin6=Pin(6)

class Connector8(Connector6):
    pin7=Pin(7)
    pin8=Pin(8)

class ISPConnector(Component):
    miso=Pin(1)
    vcc=Pin(2)
    sck=Pin(3)
    mosi=Pin(4)
    reset=Pin(5)
    gnd=Pin(6)

class LCDConnector(Component):
    # The connector on the LCD module is reversed because it's on the
    # back rather than the front (this is quite a common arrangement).
    # Adjacent pins are swapped, i.e. LCD pin 1 is our pin 2, LCD pin
    # 3 is our pin 4, and so on.
    gnd=Pin(2)
    vcc=Pin(1)
    vo=Pin(4,"Contrast")
    rs=Pin(3)
    rw=Pin(6)
    e=Pin(5)
    d0=Pin(8)
    d1=Pin(7)
    d2=Pin(10)
    d3=Pin(9)
    d4=Pin(12)
    d5=Pin(11)
    d6=Pin(14)
    d7=Pin(13)
    bl_pos=Pin(16,"Backlight_Pos")
    bl_neg=Pin(15,"Backlight_Neg")

class OneWireConnector(Component):
    gnd=Pin(1)
    data=Pin(2)
    power=Pin(3)

class RS485Connector(Component):
    a=Pin(1)
    b=Pin(2) # a/b are receive on these boards
    y=Pin(3)
    z=Pin(4) # y/z are transmit
class RS485ConnectorRev(Component):
    a=Pin(4)
    b=Pin(3) # a/b are receive on these boards
    y=Pin(2)
    z=Pin(1) # y/z are transmit

class MainsPowerConnector(Component):
    neutral=Pin(1)
    earth=Pin(2)
    live=Pin(3)

class ValveConnector(Component):
    earth=Pin(1) # Green/yellow stripe
    neutral=Pin(2) # Blue
    motor=Pin(3) # Brown
    switch_a=Pin(4) # Grey
    switch_b=Pin(5) # Orange

class Transformer2x9(Component):
    ac1=Pin(1)
    ac2=Pin(2)
    sec1a=Pin(3,"Sec1A")
    sec1b=Pin(4,"Sec1B")
    sec2a=Pin(5,"Sec2A")
    sec2b=Pin(6,"Sec2B")

class BridgeRec(Component):
    ac1=Pin(1)
    ac2=Pin(2)
    pos=Pin(3)
    neg=Pin(4) # Order of pins on component may be wildly different!

class reg7805(Component):
    input=Pin(1)
    ground=Pin(2)
    output=Pin(3)

class ATmega328(Component):
    reset=Pin(1,"RESET")
    RxD=Pin(2,"RxD")
    TxD=Pin(3,"TxD")
    pd2=Pin(4,"PD2")
    pd3=Pin(5,"PD3")
    pd4=Pin(6,"PD4")
    vcc=Pin(7)
    gnd1=Pin(8)
    osc1=Pin(9,"OSC1")
    osc2=Pin(10,"OSC2")
    pd5=Pin(11,"PD5")
    pd6=Pin(12,"PD6")
    pd7=Pin(13,"PD7")
    pb0=Pin(14,"PB0")
    pb1=Pin(15,"PB1")
    pb2=Pin(16,"PB2")
    mosi=Pin(17,"MOSI")
    miso=Pin(18,"MISO")
    sck=Pin(19,"SCK")
    avcc=Pin(20,"AVCC")
    aref=Pin(21,"AREF")
    gnd2=Pin(22)
    pc0=Pin(23,"PC0")
    pc1=Pin(24,"PC1")
    pc2=Pin(25,"PC2")
    pc3=Pin(26,"PC3")
    pc4=Pin(27,"PC4")
    pc5=Pin(28,"PC5")

class MAX489(Component):
    ro=Pin(2) # Receiver output
    re=Pin(3) # Receiver enable, active low
    de=Pin(4) # Driver enable, active high
    di=Pin(5) # Driver input
    gnd1=Pin(6)
    gnd2=Pin(7)
    y=Pin(9,"Y") # Output 1
    z=Pin(10,"Z") # Output 2
    b=Pin(11,"B") # Input 2
    a=Pin(12,"A") # Input 1
    vcc=Pin(14)
    # Pins 1, 8, 13 not connected

class ULN2803A(Component):
    # bn are bases, cn are collectors
    b1=Pin(1)
    b2=Pin(2)
    b3=Pin(3)
    b4=Pin(4)
    b5=Pin(5)
    b6=Pin(6)
    b7=Pin(7)
    b8=Pin(8)
    gnd=Pin(9)
    com=Pin(10) # Usually VCC
    c8=Pin(11)
    c7=Pin(12)
    c6=Pin(13)
    c5=Pin(14)
    c4=Pin(15)
    c3=Pin(16)
    c2=Pin(17)
    c1=Pin(18)

class OptoIsolator2(Component):
    anode1=Pin(1)
    cathode1=Pin(2)
    anode2=Pin(3)
    cathode2=Pin(4)
    collector1=Pin(8)
    emitter1=Pin(7)
    collector2=Pin(6)
    emitter2=Pin(5)
