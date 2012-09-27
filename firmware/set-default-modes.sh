#!/bin/bash

set -e

avrdude -q -P usb -c avrispv2 -p atmega328 -y -U eeprom:w:default-modes.hex
