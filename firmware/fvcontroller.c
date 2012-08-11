#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "serial.h"
#include "hardware.h"
#include "version.h"

int main(void)
{
  char buf[32];
  int i;
  /* Hardware initialisation: our pins are single-direction apart from
     the pin used for one-wire bus.  Initialise the pin direction
     registers here. */
  DDRB=0x00; /* All pins inputs */
  DDRC=0x3f; /* Pins 0-5 all outputs */
  DDRD=0xff; /* All pins outputs (serial port overrides this register) */

  /* Make sure we default to not transmitting */
  RS485_XMIT_OFF();

  /* Relays should both be off */
  trigger_relay(VALVE1_RESET);
  trigger_relay(VALVE2_RESET);

  serial_init(9600);

  /* Init complete; we can now enable interrupts */
  sei();

  get_version(buf,32);
  for (i=32; i>0; i--) {
    printf_P(PSTR("Hello world!  Firmware %s\n"),buf);
    printf_P(PSTR("Version string length is %d\n"),get_version_length());
  }

  for (;;) {
    printf_P(PSTR("Backlight on\n"));
    BACKLIGHT_ON();
    _delay_ms(1000);
    printf_P(PSTR("Backlight off\n"));
    BACKLIGHT_OFF();
    _delay_ms(1000);
  }
  return 0;
}
