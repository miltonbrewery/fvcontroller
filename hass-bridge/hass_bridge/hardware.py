import logging
import serial
import sys
import json
import time

log = logging.getLogger(__name__)

# This is mostly determined by the LCD display — the firmware doesn't
# do any character encoding or decoding
hw_charset = "ascii"


class Bus:
    """A system of controllers connected via a serial port
    """
    def __init__(self, sp_path, config, mqttc, ha_discovery_prefix,
                 mqtt_path):
        self.log = log.getChild(sp_path)
        self.mqttc = mqttc
        self.ha_discovery_prefix = ha_discovery_prefix
        self.mqtt_path = mqtt_path
        self.availability_topic = f"{mqtt_path}/status"
        self.selected = None
        self.mqtt_topics = {}
        self.last_online_announcement = 0.0

        self.log.debug("About to open serial port")
        try:
            self.s = serial.Serial(sp_path, timeout=1.0)
        except serial.SerialException:
            self.log.error("Could not open serial port")
            sys.exit(1)

        self.full_reset()
        self.controllers = {k: Controller(k, self, v)
                            for k, v in config.items()}

    def send_ha_discovery(self):
        """Send Home Assistant MQTT discovery messages
        """
        self.log.debug("Sending HA discovery messages")
        for controller in self.controllers.values():
            controller.send_ha_discovery()
        # Schedule next announcement for 5s in the future
        self.last_online_announcement = time.time() - 55.0

    def announce_online(self):
        if time.time() - self.last_online_announcement < 60.0:
            return
        self.mqttc.publish(self.availability_topic, b"online")
        self.last_online_announcement = time.time()

    def process_mqtt_message(self, msg):
        topic = msg.topic
        payload = msg.payload.decode("utf8")
        if topic in self.mqtt_topics:
            self.mqtt_topics[topic].process_mqtt_message(topic, payload)
        elif topic == f"{self.ha_discovery_prefix}/status" \
           and payload == "online":  # noqa: E127
            self.send_ha_discovery()

    def full_reset(self):
        """Return the bus to a known state

        Terminate any previous command with \n, and
        discard everything from the receive buffer.
        """
        # Terminate any partly-sent command.  We have to wait after
        # sending this for up to 0.1s for any output from the currently
        # selected controller to be completed; we receive and discard this
        # output.  No controller will send more than one line.
        self.s.write(b"\n")
        # Read until we time out
        foo = True
        while foo:
            foo = self.s.read()
            self.selected = None

    def communicate(self, command):
        self.s.write(command + b"\n")
        response = self.s.read_until()
        # A floating line produces \0 characters.  Remove them.
        response = response.replace(b'\0', b'')
        if response == b"":
            response = b"TIMEOUT\n"
        elif response[-1] != ord("\n"):
            response = b"CORRUPT\n"
        return response

    def interpret(self, sent_b: bytes, received_b: bytes):
        # The TCP interface was used to communicate directly with the
        # hardware. Try to figure out what happened.
        self.log.debug("3rd party sent: %s received %s", sent_b, received_b)
        sent = sent_b.decode(hw_charset)
        received = received_b.decode(hw_charset)
        if sent.startswith("SELECT "):
            selected = sent[7:]
            if received == f"OK {selected} selected\n":
                self.selected = self.controllers.get(selected)
            else:
                self.selected = None
            self.log.debug("Controller selected: %s", self.selected)
        else:
            if self.selected:
                self.selected.interpret(sent, received)
            else:
                self.log.debug("No controller selected, ignoring")

    def poll(self):
        for controller in self.controllers.values():
            controller.poll()


class Controller:
    def __init__(self, name, bus, config):
        self.log = bus.log.getChild(name)
        self.name = name
        self.bus = bus
        self.mqtt_path = f"{bus.mqtt_path}/{name}"
        self.entity_prefix = config.get("entity_prefix", name.lower())
        self.unique_id = f"fvc_{name}"
        self.buttons = []
        self.registers = {
            r: Register.all_registers[r](self, r, config.get(r, {}))
            for r in config.get("registers", [])}
        if not self.registers:
            self.log.warning("No registers declared")
        self.ping()
        self.sw_version = self.read("ver")

    def __str__(self):
        return self.name

    def select(self):
        if self.bus.selected == self:
            return True
        r = self.bus.communicate(f"SELECT {self.name}".encode(hw_charset))
        if r != f"OK {self.name} selected\n".encode(hw_charset):
            self.log.error(f"Could not select, got {r} instead")
            self.bus.selected = None
            return False
        self.bus.selected = self
        return True

    def read(self, reg):
        if not self.select():
            return
        r = self.bus.communicate(f"READ {reg}".encode(hw_charset))\
                    .decode(hw_charset)
        return self.process_read_reply(r)

    def process_read_reply(self, r):
        if not r:
            self.log.debug("No response to READ")
            self.bus.selected = None
            return
        if len(r) < 4:
            self.log.debug("Short response to READ: %s", r)
            self.bus.selected = None
            return
        if r[:3] != "OK " or r[-1:] != "\n":
            self.log.debug("Error response to READ: %s", r)
            self.bus.selected = None
            return
        return r[3:-1]

    def write(self, reg, value):
        if not self.select():
            return
        r = self.bus.communicate(f"SET {reg} {value}".encode(hw_charset))\
                    .decode(hw_charset)
        return self.process_write_reply(reg, r)

    def process_write_reply(self, reg, r):
        if not r:
            self.log.debug("No response to SET")
            self.bus.selected = None
            return
        if r[:3] != "OK " or r[-1:] != "\n":
            self.log.debug("Error response to SET: %s", r)
            self.bus.selected = None
            return
        r = r[:-1]
        expected = f"OK {reg} set to "
        if not r.startswith(expected):
            self.log.error("Failed to set %s; received %s", reg, r)
            self.bus.selected = None
            return
        return r[len(expected):]

    def ping(self):
        if not self.select():
            return False
        r = self.read("ident")
        if not r:
            self.log.error("Ping could not read ident register")
            self.bus.selected = None
            return False
        if r != self.name:
            self.log.error(f"Ping returned unexpected result {r} instead")
            self.bus.selected = None
            return False
        return True

    def interpret(self, sent, received):
        if sent.startswith("READ "):
            regname = sent[5:]
            self.log.debug("3rd party read from %s", regname)
            reg = self.registers.get(regname)
            if reg:
                reg.publish_update(self.process_read_reply(received))
        elif sent.startswith("SET "):
            reg_and_val = sent[4:]
            if ' ' in reg_and_val:
                regname, val = reg_and_val.split(' ', maxsplit=1)
                reg = self.registers.get(regname)
                if reg:
                    self.log.debug("3rd party write to %s", reg)
                    reg.publish_update(self.process_write_reply(
                        regname, received))
        else:
            self.log.debug("Don't know that one")

    @property
    def ha_device(self):
        return {
            "identifiers": f"fvcontroller_{self.name}",
            "manufacturer": "Stephen Early",
            "model": "fvcontroller",
            "name": self.name,
            "sw_version": self.sw_version,
        }

    def send_ha_discovery(self):
        for register in self.registers.values():
            register.send_ha_discovery()
        for button in self.buttons:
            button.send_ha_discovery()

    def poll(self):
        for register in self.registers.values():
            register.poll()

    def add_button(self, button):
        self.buttons.append(button)


class Register:
    all_registers = {}
    names = {}
    writable = False
    poll_interval = 600
    component = "sensor"  # HA component
    discovery = {}  # Extra HA discovery parameters

    def __init__(self, controller, name, config):
        self.log = controller.log.getChild(name)
        self.name = name
        ha_name = name.replace("/", "_")
        self.controller = controller
        self.unique_id = f"{controller.unique_id}_{ha_name}"
        topic_prefix = f"{self.controller.mqtt_path}/{ha_name}"
        self.state_topic = f"{topic_prefix}/state"
        self.command_topic = f"{topic_prefix}/command"
        self.entity_name = f"{controller.entity_prefix}_{ha_name}"
        self.human_name = self.names[name]
        if "poll-interval" in config:
            self.poll_interval = config["poll-interval"]
        if "description" in config:
            self.human_name = config["description"]
        self.schedule_update(10)
        if self.writable:
            controller.bus.mqtt_topics[self.command_topic] = self

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        for name in cls.names.keys():
            cls.all_registers[name] = cls

    def __str__(self):
        return f"{self.name} on {self.controller}"

    def schedule_update(self, t):
        self.last_update = time.time() - self.poll_interval + t

    def send_ha_discovery(self):
        topic = f"{self.controller.bus.ha_discovery_prefix}/{self.component}/"\
            f"{self.unique_id}/config"
        msg = {
            "availability_topic": self.controller.bus.availability_topic,
            "device": self.controller.ha_device,
            "object_id": self.entity_name,
            "name": self.human_name,
            "state_topic": self.state_topic,
            "unique_id": self.unique_id,
            "expire_after": self.poll_interval + 30,
        }
        if self.writable:
            msg["command_topic"] = self.command_topic
        msg.update(self.discovery)
        self.controller.bus.mqttc.publish(topic, json.dumps(msg))
        self.schedule_update(10)

    def poll(self):
        if (time.time() - self.last_update) < self.poll_interval:
            return
        self.publish_update(self.controller.read(self.name))

    def publish_update(self, val):
        if not val:
            self.log.warning("Null update; not sending")
            # If it's a writable config register, it may be blank;
            # this is ok, we expect the user to set a value soon
            if self.writable:
                self.last_update = time.time()
            return
        payload = self.format_payload(val)
        if payload:
            self.controller.bus.mqttc.publish(
                self.state_topic, self.format_payload(val))
        self.ack(val)
        self.last_update = time.time()

    def process_mqtt_message(self, topic, payload):
        if not self.writable:
            self.log.error("Attempt to write to read-only register")
            return
        self.publish_update(self.controller.write(self.name, payload))

    def format_payload(self, val):
        """Process raw value from hardware before sending mqtt message

        Return None to prevent the message being sent
        """
        return val

    def ack(self, val):
        """Acknowledge receipt of val from the device
        """
        # Error registers will want to write val back to the device to
        # reduce the error counter
        pass


class ModeButton:
    def __init__(self, modename_reg):
        name = modename_reg.name.replace("name", "button")
        prefix = name[:2]
        controller = modename_reg.controller
        self.modename_reg = modename_reg
        self.log = controller.log.getChild(name)
        self.name = name
        ha_name = name.replace("/", "_")
        self.controller = modename_reg.controller
        self.unique_id = f"{controller.unique_id}_{ha_name}"
        topic_prefix = f"{self.controller.mqtt_path}/{ha_name}"
        self.command_topic = f"{topic_prefix}/command"
        self.entity_name = f"{controller.entity_prefix}_{ha_name}"
        self.human_name = f"{modename_reg.name} activate"
        self.regmap = [
            ("alarm/hi", f"{prefix}/a/hi"),
            ("alarm/lo", f"{prefix}/a/lo"),
            ("set/hi", f"{prefix}/hi"),
            ("set/lo", f"{prefix}/lo"),
            ("jog/hi", f"{prefix}/j/hi"),
            ("jog/lo", f"{prefix}/j/lo"),
            ("mode", f"{prefix}/name"),
        ]
        controller.bus.mqtt_topics[self.command_topic] = self
        controller.add_button(self)

    def send_ha_discovery(self):
        topic = f"{self.controller.bus.ha_discovery_prefix}/button/"\
            f"{self.unique_id}/config"
        msg = {
            "availability_topic": self.controller.bus.availability_topic,
            "device": self.controller.ha_device,
            "object_id": self.entity_name,
            "name": self.human_name,
            "command_topic": self.command_topic,
            "unique_id": self.unique_id,
        }
        self.controller.bus.mqttc.publish(topic, json.dumps(msg))

    def set_name(self, name):
        self.human_name = name
        self.send_ha_discovery()

    def process_mqtt_message(self, topic, payload):
        self.log.debug("pressed")
        # Read mode settings
        settings = [self.controller.read(src) for _, src in self.regmap]
        if not all(settings):
            # Reading at least one of these failed; we can't continue
            self.log.error("Failed to read settings for %s: %s",
                           self.name, settings)
            return

        # Write settings into active registers
        for ((dest, _), val) in zip(self.regmap, settings):
            written = self.controller.write(dest, val)
            reg = self.controller.registers.get(dest)
            if reg:
                reg.publish_update(written)


class TempReg(Register):
    names = {
        "t0": "Fermenter temperature",
        "t1": "t1",
        "t2": "t2",
        "t3": "t3",
    }
    poll_interval = 60
    discovery = {
        "state_class": "measurement",
        "device_class": "temperature",
        "unit_of_measurement": "°C",
        "suggested_display_precision": 2,
    }

    def format_payload(self, val):
        try:
            # The CLB sensor sometimes returns nonsensical values that
            # mess up the scale on graphs in Home Assistant
            if float(val) < 0.0:
                return
            if float(val) > 100.0:
                return
        except Exception:
            pass
        return val


class TempIDReg(Register):
    names = {
        "t0/id": "t0 probe ID",
        "t1/id": "t1 probe ID",
        "t2/id": "t2 probe ID",
        "t3/id": "t3 probe ID",
    }
    discovery = {
        "entity_category": "diagnostic",
    }
    poll_interval = 86400


class TempSetReg(Register):
    names = {
        "set/lo": "Low set point",
        "set/hi": "High set point",
        "alarm/lo": "Alarm low set point",
        "alarm/hi": "Alarm high set point",
        "jog/lo": "Stuck valve low set point",
        "jog/hi": "Stuck value high set point",
    }
    writable = True
    component = "number"
    discovery = {
        "device_class": "temperature",
        "unit_of_measurement": "°C",
        "step": 0.1,
        "min": 0,
        "max": 100,
    }
    poll_interval = 60

    def format_payload(self, val):
        return f"{float(val):0.1f}"


class ValveStatus(Register):
    names = {"v0": "Valve status"}
    poll_interval = 60


class Mode(Register):
    names = {"mode": "Mode"}
    poll_interval = 60
    component = "text"
    writable = True
    discovery = {
        'max': 8,
        'min': 1,
    }


class Alarm(Register):
    names = {"alarm": "Alarm"}
    poll_interval = 60


class ConfigModeName(Register):
    names = {
        "m0/name": "Mode 0 name",
        "m1/name": "Mode 1 name",
        "m2/name": "Mode 2 name",
        "m3/name": "Mode 3 name",
        "m4/name": "Mode 4 name",
        "m5/name": "Mode 5 name",
    }
    poll_interval = 86400
    component = "text"
    writable = True
    discovery = {
        "entity_category": "config",
        "max": 8,
        "min": 1,
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.modebutton = ModeButton(self)

    def publish_update(self, val):
        super().publish_update(val)
        self.modebutton.set_name(val)


_mode_temp_names = {}
for m in range(6):
    _mode_temp_names.update({
        f"m{m}/lo": f"Mode {m} low set point",
        f"m{m}/hi": f"Mode {m} high set point",
        f"m{m}/a/lo": f"Mode {m} alarm low set point",
        f"m{m}/a/hi": f"Mode {m} alarm high set point",
        f"m{m}/j/lo": f"Mode {m} stuck valve low set point",
        f"m{m}/j/hi": f"Mode {m} stuck valve high set point",
    })


class ConfigModeTemp(Register):
    names = _mode_temp_names
    poll_interval = 86400
    writable = True
    component = "number"
    discovery = {
        "entity_category": "config",
        "device_class": "temperature",
        "unit_of_measurement": "°C",
        "step": 0.1,
        "min": 0,
        "max": 100,
    }

    def format_payload(self, val):
        return f"{float(val):0.1f}"


# XXX TODO: various other configuration registers for valve mode,
# backlight, etc.


class ErrorReg(Register):
    names = {
        "err/miss": "Missing probe errors",
        "err/shrt": "Shorted probe bus errors",
        "err/crc": "Probe CRC errors",
        "err/pwr": "Probe power errors",
    }
    discovery = {
        "entity_category": "diagnostic",
        "state_class": "measurement",
    }
    poll_interval = 60

    def ack(self, val):
        if val != "0":
            self.controller.write(self.name, val)
