#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

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
#include "command.h"

static void choose_mode(void)
{
  const struct reg *mr;
  char mn[8];
  char name[9];
  char lo[5];
  char hi[5];
  char buf[32];
  int m;
  uint16_t timeout=0;

  BACKLIGHT_ON();
  ack_buttons();

  do {
    for (m=0; ; m++) {
      sprintf_P(mn,PSTR("m%d/name"),m);
      mr=reg_by_name(mn);
      if (!mr) {
	m=0;
	break;
      }
      reg_read_string(mr,name,9);
      sprintf_P(mn,PSTR("m%d/lo"),m);
      mr=reg_by_name(mn);
      reg_read_string(mr,lo,5);
      sprintf_P(mn,PSTR("m%d/hi"),m);
      mr=reg_by_name(mn);
      reg_read_string(mr,hi,5);
      sprintf_P(buf,PSTR("%s\n%s-%s"),name,lo,hi);
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
	  reg_write_string(&set_lo,lo);
	  reg_write_string(&set_hi,hi);
	  reg_write_string(&mode,name);
	  BACKLIGHT_OFF();
	  return;
	}
	ack_buttons();
	_delay_ms(1);
      }
      if (timeout==0) break;
    }
  } while (timeout>0);
}

static void trigger_backlight(void)
{
  struct storage s;
  s=reg_storage(&bl);
  cli();
  backlight_timer=eeprom_read_word((void *)s.loc.eeprom.start);
  sei();
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
  trigger_backlight();
  for (;;) {
    lcd_home_screen();
    if (backlight_timer!=0) {
      BACKLIGHT_ON();
    } else {
      BACKLIGHT_OFF();
    }
    if (get_buttons()) {
      if (get_buttons()==(K_UP|K_DOWN)) {
	/* We leave this main loop when entering setup mode because it
	   requires dedicated access to the 1-wire bus */
	sensor_setup();
      } else if (get_buttons()==(K_DOWN)) {
	choose_mode();
      }
      ack_buttons();
      trigger_backlight();
    }
    if (tprobe_timer==0) {
      read_probes();
      owb_start_temp_conversion();
      tprobe_timer=TEMPERATURE_PROBE_PERIOD;
    }
    if (rx_data_available()) {
      process_command();
      ack_rx_data();
    }
  }
  return 0;
}
