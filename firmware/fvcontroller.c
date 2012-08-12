#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "serial.h"
#include "hardware.h"
#include "lcd.h"
#include "registers.h"

int main(void)
{
  char buf[32];
  uint8_t n;
  const struct reg *r;
  /* Hardware initialisation: our pins are single-direction apart from
     the pin used for one-wire bus.  Initialise the pin direction
     registers here. */
  PORTB=0xff; /* All pins pulled up */
  DDRB=0x00; /* All pins inputs (one-wire-bus code will toggle bit 0) */
  PORTC=0;
  DDRC=0x3f; /* Pins 0-5 all outputs */
  PORTD=0;
  DDRD=0xff; /* All pins outputs (serial port overrides this register) */

  /* Make sure we default to not transmitting */
  RS485_XMIT_OFF();

  BACKLIGHT_OFF();

  /* Relays should both be off */
  trigger_relay(VALVE1_RESET);
  trigger_relay(VALVE2_RESET);

  serial_init(9600);
  hw_init_lcd();

  /* Hardware init complete; we can now enable interrupts */
  sei();

  lcd_init();

  printf_P(PSTR("Hello world!\n"),buf);

  hello_lcd();

  n=0;
  while ((r=reg_number(n++))) {
    reg_name(r,buf);
    printf("Name: %s\n",buf);
    reg_description(r,buf);
    printf("Description: %s\n",buf);
    reg_read_string(r,buf,32);
    printf("Value: %s\n",buf);
  }

  for (;;) {
    uint8_t b;

    b=PINB;
    printf_P(PSTR("PINB=%02x\n"),b);
    _delay_ms(1000);
  }
  return 0;
}
