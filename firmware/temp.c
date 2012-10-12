#include <stdio.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include "temp.h"
#include "hardware.h"
#include "registers.h"
#include "owb.h"

/* The hardware reads out temperatures in multiples of 1/16 degree
   (0.0625).  We then take that and apply calibration data,
   potentially interpolating.  This suggests the natural datatype for
   temperature in the firmware is a ten-thousandth of a degree in an
   int32_t. */

int32_t t0_temp;
int32_t t1_temp;
int32_t t2_temp;
uint8_t v0_state;
uint8_t v1_state;

void read_probes(void)
{
  uint8_t addr[8];
  const struct reg *r;
  struct storage s;
  int32_t t;
  int32_t s_hi,s_lo;
  uint8_t old_v0;

  r=reg_by_name_P(PSTR("t0/id"));
  s=reg_storage(r);
  eeprom_read_block(addr,(void *)s.loc.eeprom.start,8);
  t=owb_read_temp(addr);
  if (t!=BAD_TEMP) t0_temp=t;
  //  printf("t0_temp=%" PRIi32 "\n",t0_temp);

  /* Read s_hi and s_lo from eeprom */
  s=reg_storage(&set_hi);
  eeprom_read_block(&s_hi,(void *)s.loc.eeprom.start,4);
  s=reg_storage(&set_lo);
  eeprom_read_block(&s_lo,(void *)s.loc.eeprom.start,4);

  /* Be a thermostat! */
  old_v0=v0_state;
  if (t0_temp>s_hi) {
    v0_state=1;
  }
  if (t0_temp<s_lo) {
    v0_state=0;
  }
  if (old_v0==0 && v0_state==1) {
    trigger_relay(VALVE1_SET);
  }
  if (old_v0==1 && v0_state==0) {
    trigger_relay(VALVE1_RESET);
  }
}
