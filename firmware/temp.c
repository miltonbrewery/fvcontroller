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
uint8_t v0_state; /* Desired valve state: 0=closed, 1=open */
uint8_t v1_output_on;
uint8_t v2_output_on;

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
  uint8_t valve;

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

  /* Read valve mode from eeprom */
  s=reg_storage(&vtype);
  valve=eeprom_read_byte((void *)s.loc.eeprom.start);
  
  /* Be a thermostat, with valve opened to provide chilling */
  if (t0_temp>s_hi) {
    v0_state=1;
  }
  if (t0_temp<s_lo) {
    v0_state=0;
  }

  switch (valve) {
  case 0:
  case 0xff:
    /* Spring-return valve on VALVE1, nothing on VALVE2 */
    if (v0_state && !v1_output_on) {
      trigger_relay(VALVE1_SET);
      v1_output_on=1;
    }
    if (!v0_state && v1_output_on) {
      trigger_relay(VALVE1_RESET);
      v1_output_on=0;
    }
    break;
  case 1: /* Ball valve: open on VALVE1, close on VALVE2, no limit sensors */
  case 2: /* As case 1, but with limit sensors */
    if (v0_state) {
      /* VALVE1 should be energised, VALVE2 should be de-energised */
      if (v2_output_on) {
	trigger_relay(VALVE2_RESET);
	v2_output_on=0;
      }
      if (!v1_output_on) {
	trigger_relay(VALVE1_SET);
	v1_output_on=1;
      }
    } else {
      /* VALVE1 should be de-energised, VALVE2 should be energised */
      if (v1_output_on) {
	trigger_relay(VALVE1_RESET);
	v1_output_on=0;
      }
      if (!v2_output_on) {
	trigger_relay(VALVE2_SET);
	v2_output_on=1;
      }
    }
    break;
  }
}

uint8_t get_valve_state(void)
{
  struct storage s;
  uint8_t valve,v1,v2;

  /* Read valve mode from eeprom */
  s=reg_storage(&vtype);
  valve=eeprom_read_byte((void *)s.loc.eeprom.start);

  v1=read_valve(VALVE1_STATE);
  v2=read_valve(VALVE2_STATE);

  switch (valve) {
  case 0:
  case 0xff:
    /* Spring-return valve on VALVE1, nothing on VALVE2 */
    if (v0_state) {
      if (v1) return VALVE_OPEN;
      else return VALVE_OPENING;
    } else {
      if (v1) return VALVE_CLOSING;
      else return VALVE_CLOSED;
    }
    break;
  case 1: /* Ball valve with no limit sensors */
    if (v0_state) return VALVE_OPEN;
    else return VALVE_CLOSED;
    break;
  case 2: /* Ball valve with open limit sensor on VALVE1 and closed limit
	     sensor on VALVE2 */
    if (v0_state) {
      if (v1 && !v2) return VALVE_OPEN;
      if (v1 && v2) return VALVE_ERROR;
      return VALVE_OPENING;
    } else {
      if (v2 && !v1) return VALVE_CLOSED;
      if (v1 && v2) return VALVE_ERROR;
      return VALVE_CLOSING;
    }
    break;
  }
  return VALVE_ERROR;
}
