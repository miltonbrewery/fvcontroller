import sys
import argparse
import pathlib
import logging
import tomli
import selectors
import socket
import sdnotify
import paho.mqtt.client as mqtt
from .hardware import Bus

log = logging.getLogger(__name__)


def incoming_connection(sock, bus):
    log.debug("Accepting TCP connection")
    conn, addr = sock.accept()
    conn.settimeout(10)
    with conn.makefile('rb') as f:
        try:
            for data in f:
                data = data.strip()
                if not data:
                    continue
                response = bus.communicate(data)
                bus.interpret(data, response)
                try:
                    conn.send(response)
                except Exception as e:
                    log.error("Exception writing to socket: %s", e)
                    break
        except Exception as e:
            log.error("Exception reading from socket: %s", e)
    log.debug("Closing TCP connection")
    conn.close()


def on_mqtt_connect(client, userdata, flags, reason_code, properties):
    log.debug("MQTT connected reason_code %s", reason_code)
    bus = userdata
    client.subscribe(f"{bus.ha_discovery_prefix}/status")
    client.subscribe(f"{bus.mqtt_path}/+/+/command")
    bus.send_ha_discovery()


def on_mqtt_disconnect(client, userdata, flags, reason_code, properties):
    log.error("MQTT disconnected reason_code %s", reason_code)
    # We exit and expect to be restarted by systemd
    sys.exit(1)


def on_mqtt_message(client, userdata, msg):
    log.debug("MQTT message: topic %s payload %s", msg.topic, msg.payload)
    bus = userdata
    bus.process_mqtt_message(msg)


class MqttSelectorsHelper:
    def __init__(self, client, sel):
        self.client = client
        self.sel = sel
        client.on_socket_open = self.on_socket_open
        client.on_socket_close = self.on_socket_close
        client.on_socket_register_write = self.on_socket_register_write
        client.on_socket_unregister_write = self.on_socket_unregister_write

    def on_socket_open(self, client, userdata, sock):
        log.debug("Mqtt socket opened")
        self.sel.register(sock, selectors.EVENT_READ)

    def on_socket_close(self, client, userdata, sock):
        log.debug("Mqtt socket closing")
        self.sel.unregister(sock)

    def on_socket_register_write(self, client, userdata, sock):
        self.sel.modify(sock, selectors.EVENT_READ | selectors.EVENT_WRITE)

    def on_socket_unregister_write(self, client, userdata, sock):
        self.sel.modify(sock, selectors.EVENT_READ)

    def event(self, fileobj, mask):
        if mask & selectors.EVENT_WRITE:
            self.client.loop_write()
        if mask & selectors.EVENT_READ:
            self.client.loop_read()

    def misc(self):
        self.client.loop_misc()


def fvbridge():
    parser = argparse.ArgumentParser(description="FV controller interface")
    parser.add_argument(
        'configfile', type=pathlib.Path, help="Path to configuration file")
    parser.add_argument(
        '--debug', action="store_true",
        help="Output debug information")
    parser.add_argument(
        '--notify', action="store_true",
        help="Notify systemd once startup is complete")
    parser.add_argument(
        '--serial', help="Path to serial port")

    args = parser.parse_args()

    try:
        with open(args.configfile, "rb") as f:
            config = tomli.load(f)
    except FileNotFoundError:
        print(f"Could not open config file '{args.configfile}'")
        sys.exit(1)
    except tomli.TOMLDecodeError as e:
        print(str(e))
        sys.exit(1)

    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO)

    general = config.get("general", {})

    sp_path = general.get("serial", args.serial)
    if not sp_path:
        print("No serial port specified")
        sys.exit(1)

    mqtt_hostname = general.get("mqtt_hostname", "localhost")
    mqtt_port = general.get("mqtt_port", 1883)
    mqtt_username = general.get("mqtt_username")
    mqtt_password = general.get("mqtt_password")
    discovery_prefix = general.get("discovery_prefix", "homeassistant")
    mqtt_path = general.get("mqtt_path", "fvcontrol")
    listen_hostname = general.get("listen_hostname", "localhost")
    listen_port = general.get("listen_port", 1576)

    controller_config = config.get("controller", {})

    if not controller_config:
        log.warning("No controllers declared in configuration file")

    sel = selectors.DefaultSelector()

    mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    if mqtt_username:
        mqttc.username_pw_set(username=mqtt_username, password=mqtt_password)

    bus = Bus(sp_path, controller_config, mqttc, discovery_prefix, mqtt_path)

    mqttc.user_data_set(bus)
    mqttc.on_connect = on_mqtt_connect
    mqttc.on_disconnect = on_mqtt_disconnect
    mqttc.on_message = on_mqtt_message
    mqtt_helper = MqttSelectorsHelper(mqttc, sel)
    mqttc.will_set(bus.availability_topic, b"offline")

    log.debug("Opening listening socket %s", (listen_hostname, listen_port))
    sock = socket.socket()
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((listen_hostname, listen_port))
    sock.listen(10)
    sock.setblocking(False)

    sel.register(sock, selectors.EVENT_READ)

    log.debug("About to connect to mqtt %s", (mqtt_hostname, mqtt_port))
    mqttc.connect(mqtt_hostname, mqtt_port, 60)

    if args.notify:
        log.debug("Notifying startup complete to systemd")
        sdnotify.SystemdNotifier().notify("READY=1")

    log.debug("Entering event loop")
    while True:
        events = sel.select(5)
        for key, mask in events:
            if key.fileobj == sock:
                incoming_connection(sock, bus)
            else:
                # Must be the MQTT client
                mqtt_helper.event(key.fileobj, mask)
        mqtt_helper.misc()
        bus.announce_online()
        bus.poll()
