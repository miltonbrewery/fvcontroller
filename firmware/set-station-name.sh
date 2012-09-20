#!/bin/bash

set -e

tmpfile=/tmp/fvname-$$

echo -n -e ${1}\\x00 >${tmpfile}
echo -n $1 | avr-objcopy --binary-architecture=avr --input-target=binary --change-addresses 0x3f4 ${tmpfile} -O ihex ${tmpfile}.hex
avrdude -q -P usb -c avrispv2 -p atmega328p -y -U eeprom:w:${tmpfile}.hex
rm -f ${tmpfile} ${tmpfile}.hex
