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

int32_t t0_temp=BAD_TEMP;
int32_t t1_temp=BAD_TEMP;
int32_t t2_temp=BAD_TEMP;
int32_t t3_temp=BAD_TEMP;
uint8_t v0_state;
uint8_t v1_state;

/* NB expects name to be a pointer to a string in progmem */
static int32_t read_probe(const char *name)
{
  uint8_t addr[8];
  const struct reg *r;
  struct storage s;
  char regname[9];

  strncpy_P(regname,name,9);
  strncat_P(regname,PSTR("/id"),9);
  r=reg_by_name(regname);
  s=reg_storage(r);
  eeprom_read_block(addr,(void *)s.loc.eeprom.start,8);
  return owb_read_temp(addr);
}

void read_probes(void)
{
  struct storage s;
  int32_t s_hi,s_lo;
  uint8_t old_v0;

  t0_temp=read_probe(PSTR("t0"));
  t1_temp=read_probe(PSTR("t1"));
  t2_temp=read_probe(PSTR("t2"));
  t3_temp=read_probe(PSTR("t3"));

  /* Don't be a thermostat if we don't have a reading */
  if (t0_temp==BAD_TEMP) return;

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
