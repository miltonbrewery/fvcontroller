from parts import *

# Fermenter controller parts list and netlist

# Mains power interface
live=Net("MainsLive","Power")
neutral=Net("MainsNeutral","Power")
earth=Net("Earth","Power")
MainsPowerConnector(name="J9",label="Mains",
                    live=live,neutral=neutral,earth=earth)

# Power - we have two separate power systems, plus mains.  Power for
# the opto isolators is isolated from the digital power, because it
# might accidentally be shorted to mains in the valves.

vcc=Net("Plus5v","Power")
gnd=Net("GND","Power")
unreg=Net("Unreg","Power")

valve_vcc=Net("ValveVCC","Power")
valve_gnd=Net("ValveGND","Power")
valve_unreg=Net("ValveUnreg","Power")

# Our transformer has two secondaries
xfmr=Transformer2x9(name="XFMR1",ac1=live,ac2=neutral)

# Two bridge rectifiers
BridgeRec(name="BR1",ac1=xfmr.sec1a,ac2=xfmr.sec1b,pos=unreg,neg=gnd)
BridgeRec(name="BR2",ac1=xfmr.sec2a,ac2=xfmr.sec2b,
          pos=valve_unreg,neg=valve_gnd)

# Two power regulators
reg7805(name="REG1",input=unreg,output=vcc,ground=gnd)
PCapacitor("C6","2200uF",pos=unreg,neg=gnd)
PCapacitor("C8","10uF",pos=vcc,neg=gnd)

reg7805(name="REG2",input=valve_unreg,output=valve_vcc,ground=valve_gnd)
PCapacitor("C7","2200uF",pos=valve_unreg,neg=valve_gnd)
PCapacitor("C9","10uF",pos=valve_vcc,neg=valve_gnd)

# Power connector; useful if we need regulated power
Connector2(name="J1",label="Power",pin1=gnd,pin2=vcc)

# Microcontroller, its power and oscillator

mcu=ATmega328(name="U1",gnd1=gnd,gnd2=gnd,vcc=vcc)
Crystal(name="XT1",freq="16MHz",a=mcu.osc1,b=mcu.osc2)
Capacitor("C1","22pF",a=mcu.osc1,b=gnd)
Capacitor("C2","22pF",a=mcu.osc2,b=gnd)
Capacitor("C3","100nF",a=mcu.aref,b=gnd)
Resistor("R1","10R",a=mcu.vcc,b=mcu.avcc)
Capacitor("C4","100nF",a=mcu.avcc,b=gnd)
# Reset circuit: pullup and capacitor
Capacitor("C5","100nF",a=mcu.reset,b=gnd) # Maybe omit?
Resistor("R6","10K",a=vcc,b=mcu.reset)

# FTDI interface for debug (including wire link to enable VCC over FTDI)

Connector6(name="J3",label="FTDI",
           pin1=gnd,pin3=Connector2(
        name="J4",label="FTDI power",pin1=vcc).pin2,
           pin5=mcu.TxD).pin3.setNetName("FTDI_VCC")

# ISP interface for programming
ISPConnector(name="J2",label="ISP",
             miso=mcu.miso,vcc=vcc,sck=mcu.sck,
             mosi=mcu.mosi,reset=mcu.reset,gnd=gnd)

# Darlington driver for relay coils and backlight
driver=ULN2803A("U3",gnd=gnd,com=vcc)
# Ground the unused inputs
driver.b7=gnd
driver.b8=gnd

# LCD interface
backlight=Net("Backlight","Power")
LCDConnector(name="J5",label="LCD",
             gnd=gnd,vcc=vcc,vo=Varistor(
        "VR1","10K",label="Contrast",a=vcc,b=gnd).wiper,
             rs=mcu.pc4,rw=gnd,e=mcu.pc5,
             d4=mcu.pc0,d5=mcu.pc1,d6=mcu.pc2,d7=mcu.pc3,
             bl_pos=Resistor("R2","6R",a=vcc).b,bl_neg=backlight)
# The backlight is run through the 2803 position 5
driver.c5=backlight
driver.b5=mcu.pd3
# Note that the LCDs I've bought for Milton (RS part number 720-0193)
# already have a series resistor for the backlight when connecting
# through pins 15 and 16 (this resistor is bypassed if you connect
# directly to the 'A' and 'K' pads at the other end of the board).  In
# this case, we omit R2 and use a wire link instead.

# RS485 communication
# We use a single four-pin connector for both directions of
# communication.  We have two copies of this connector, to enable
# passthrough to another board or to enable the 120R terminating
# resistors to be fitted at each end of the network.

bustrans=MAX489(name="U2",gnd1=gnd,gnd2=gnd,vcc=vcc,re=gnd,
                ro=mcu.RxD,di=mcu.TxD,de=mcu.pd2)
RS485Connector(name="J6",label="RS485",
               a=bustrans.a,b=bustrans.b,y=bustrans.y,z=bustrans.z)
RS485ConnectorRev(name="J7",label="RS485",
               a=bustrans.a,b=bustrans.b,y=bustrans.y,z=bustrans.z)

# 1-wire bus interface
# A single three-pin connector, and a pullup resistor

OneWireConnector(name="J8",label="1-wire",data=mcu.pb0,gnd=gnd,power=vcc)
mcu.pb0.setNetName("OneWire")
Resistor("R3","4K7",a=mcu.pb0,b=vcc)

# Valve controllers
# Each valve needs a relay (controlled through the ULN2803A), an
# opto-isolator and its discrete components.

# Connect up relay control pins to the ULN2803A
driver.c1.setNetName("Relay1_Set")
driver.b1=mcu.pd4
driver.c2.setNetName("Relay1_Reset")
driver.b2=mcu.pd5
driver.c3.setNetName("Relay2_Set")
driver.b3=mcu.pd6
driver.c4.setNetName("Relay2_Reset")
driver.b4=mcu.pd7
relay1=LatchRelay(name="RELAY1",reset_pos=vcc,set_pos=vcc,common_a=live,
                  reset_neg=driver.c2,set_neg=driver.c1)
relay2=LatchRelay(name="RELAY2",reset_pos=vcc,set_pos=vcc,common_a=live,
                  reset_neg=driver.c4,set_neg=driver.c3)
valve1=ValveConnector(name="J10",label="Valve 1",earth=earth,
                      neutral=neutral,motor=relay1.no_a)
valve2=ValveConnector(name="J11",label="Valve 2",earth=earth,
                      neutral=neutral,motor=relay2.no_a)
valve1.motor.setNetName("Valve1_Motor")
valve1.motor.setNetStyle("Power")
valve2.motor.setNetName("Valve2_Motor")
valve2.motor.setNetStyle("Power")
# Opto-isolators: on the input side, 180R pullups to valve_vcc, then
# the microswitch, then the LED.  On the output side, 10k pullups to vcc.
Resistor("R4","180R",a=valve_vcc,b=valve1.switch_b)
Resistor("R5","180R",a=valve_vcc,b=valve2.switch_b)
Resistor("R7","10k",a=vcc,b=mcu.pb1)
Resistor("R8","10k",a=vcc,b=mcu.pb2)
OptoIsolator2(name="U4",cathode1=valve_gnd,cathode2=valve_gnd,
              emitter1=gnd,emitter2=gnd,
              anode1=valve1.switch_a,anode2=valve2.switch_a,
              collector1=mcu.pb1,collector2=mcu.pb2)
valve1.switch_b.setNetName("Valve1_Sensor_Out")
valve2.switch_b.setNetName("Valve2_Sensor_Out")
valve1.switch_a.setNetName("Valve1_Sensor_Rtn")
valve2.switch_a.setNetName("Valve2_Sensor_Rtn")

# Some random decoupling capacitors, to scatter around the board
Capacitor("C10","100nF",a=vcc,b=gnd)
Capacitor("C11","100nF",a=vcc,b=gnd)
Capacitor("C12","100nF",a=vcc,b=gnd)
Capacitor("C13","100nF",a=vcc,b=gnd)

# Front-panel buttons and comms LED
# We drive the LED through the ULN2803
# Pullups are not necessary, we keep the MCU's internal pullups turned on
commsled_pullup=Resistor("R9","180R",a=vcc)
commsled_pullup.b.setNetName("CommsLEDPower")
Connector8(name="J12",pin1=gnd,pin3=gnd,pin5=gnd,
           pin2=mcu.mosi,pin4=mcu.sck,pin6=mcu.miso,
           pin7=commsled_pullup.b,pin8=driver.c6)
driver.b6=mcu.pd2
driver.c6.setNetName("CommsLED")

# Output things
Net.outputNets()
