#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "config.h"
#include "serial.h"
#include "hardware.h"
#include "lcd.h"
#include "owb.h"
#include "registers.h"
#include "buttons.h"
#include "timer.h"
#include "setup.h"
#include "temp.h"

struct mode {
  char name[8];
  char lo[8];
  char hi[8];
};

struct mode modes[]={
  {"Ferment","22.0","23.0"},
  {"Slow","20.0","21.0"},
  {"Chill","7.5","8.0"},
  {"Off","95.0","96.0"},
};
#define mode_count 4

static void set_mode(const struct mode *m)
{
  reg_write_string(&set_lo,m->lo);
  reg_write_string(&set_hi,m->hi);
  reg_write_string(&mode,m->name);
}
static void choose_mode(void)
{
  struct mode *m;
  char buf[32];
  int mode;
  uint16_t timeout=0;

  BACKLIGHT_ON();
  lcd_message_P(PSTR("Choose mode..."));
  _delay_ms(1000);

  ack_buttons();

  do {
    for (mode=0; mode<mode_count; mode++) {
      m=&modes[mode];
      sprintf_P(buf,PSTR("%s\n%s-%s"),m->name,m->lo,m->hi);
      lcd_message(buf);
      for (timeout=10000; timeout>0; timeout--) {
	if (get_buttons()==K_UP) {
	  ack_buttons();
	  BACKLIGHT_OFF();
	  return;
	} else if (get_buttons()==K_DOWN) {
	  ack_buttons();
	  break;
	} else if (get_buttons()==K_ENTER) {
	  ack_buttons();
	  set_mode(m);
	  BACKLIGHT_OFF();
	  return;
	}
	ack_buttons();
	_delay_ms(1);
      }
      if (timeout==0) break;
    }
  } while (timeout>0);

  BACKLIGHT_OFF();
}

int main(void)
{
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

  owb_init();

  /* Relays should both be off */
  trigger_relay(VALVE1_RESET);
  trigger_relay(VALVE2_RESET);

  serial_init(9600);
  hw_init_lcd();
  timer_init();
  buttons_init();

  /* Hardware init complete; we can now enable interrupts */
  sei();

  lcd_init();

  /* This is the main loop.  We listen for keypresses all the time and
     use them to drive a menu system.  Also, we wait for a 1s timer
     expiry; when the timer expires we take a temperature reading and
     initiate a new one.  (Eventually we'll read multiple sensors, but
     we only have one configured at the moment.) */
  tprobe_timer=TEMPERATURE_PROBE_PERIOD;
  for (;;) {
    lcd_home_screen();
    if (get_buttons()) {
      if (get_buttons()==(K_UP|K_DOWN)) {
	/* We leave this main loop when entering setup mode because it
	   requires dedicated access to the 1-wire bus */
	sensor_setup();
      } else if (get_buttons()==(K_DOWN)) {
	choose_mode();
      }
      ack_buttons();
    }
    if (tprobe_timer==0) {
      read_probes();
      owb_start_temp_conversion();
      tprobe_timer=TEMPERATURE_PROBE_PERIOD;
    }
  }
  return 0;
}
