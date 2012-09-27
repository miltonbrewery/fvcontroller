#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "buttons.h"
#include "setup.h"
#include "lcd.h"
#include "owb.h"
#include "hardware.h"
#include "registers.h"

static void assign_probe(uint8_t *addr)
{
  const struct reg *r;
  struct storage s;

  r=reg_by_name_P(PSTR("t0/id"));
  if (!r) {
    lcd_message_P(PSTR("Can't find t0/id"));
    _delay_ms(1000);
    return;
  }
  s=reg_storage(r);
  eeprom_write_block(addr,(void *)s.loc.eeprom.start,8);
  lcd_message_P(PSTR("Assigned"));
  _delay_ms(1000);
}

void sensor_setup(void)
{
  int device_count;
  int i;
  char buf[32];
  char buf2[16];
  uint8_t addr[8];
  uint32_t temp;
  uint16_t timeout=0;

  BACKLIGHT_ON();

  lcd_message_P(PSTR("Please wait...\nfinding sensors"));
  /* The sensors may be in the middle of a conversion.  Wait for it to
     finish. */
  _delay_ms(1000);
  /* Start a fresh conversion on all sensors and wait for it to finish */
  owb_start_temp_conversion();
  _delay_ms(1000);
  
  device_count=owb_count_devices();
  ack_buttons();
  if (device_count==-1) {
    lcd_message_P(PSTR("Bus shorted\nto ground."));
    _delay_ms(1000);
    return;
  }
  if (device_count==-2) {
    lcd_message_P(PSTR("Bus shorted\nto +5v."));
      _delay_ms(1000);
    return;
  }
  if (device_count==0) {
    lcd_message_P(PSTR("No sensors found."));
    _delay_ms(1000);
    return;
  }
  sprintf_P(buf,PSTR("%d sensors found."),device_count);
  lcd_message(buf);
  _delay_ms(1000);

  /* We have at least one sensor.  Display its ID in the first row,
     and its temperature in the second row.  "Down" moves to the next
     sensor, "Enter" lets you choose what to assign it to.  "Up"
     exits. */
  do {
    for (i=0; i<device_count; i++) {
      owb_get_addr(addr,i);
      owb_format_addr(addr,buf,sizeof(buf));
      strcat_P(buf,PSTR("\n"));
      temp=owb_read_temp(addr);
      snprintf_P(buf2,sizeof(buf2),PSTR("%d Temp: %" PRIu32),i,temp);
      strcat(buf,buf2);
      lcd_message(buf);
      for (timeout=60000; timeout>0; timeout--) {
	if (get_buttons()==K_UP) {
	  ack_buttons();
	  return;
	} else if (get_buttons()==K_DOWN) {
	  ack_buttons();
	  break;
	} else if (get_buttons()==K_ENTER) {
	  ack_buttons();
	  assign_probe(addr);
	  break;
	}
	ack_buttons();
	_delay_ms(1);
      }
      if (timeout==0) break;
    }
  } while (timeout>0);
}
