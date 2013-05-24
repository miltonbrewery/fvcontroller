#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/atomic.h>
#include "registers.h"
#include "owb.h"
#include "temp.h"
#include "hardware.h"

static void eeprom_string_read(const struct reg *reg, char *buf, size_t len)
{
  struct storage s;
  char *tbuf;
  s=reg_storage(reg);
  tbuf=alloca(s.slen);
  eeprom_read_block(tbuf,(void *)s.loc.eeprom.start,s.loc.eeprom.length);
  tbuf[s.slen-1]=0;
  strncpy(buf,tbuf,len);
  buf[len-1]=0;
}

static uint8_t eeprom_string_write(const struct reg *reg, const char *buf)
{
  struct storage s;
  s=reg_storage(reg);
  eeprom_write_block(buf,(void *)s.loc.eeprom.start,s.loc.eeprom.length);
  return 0;
}

static void eeprom_uint32_read_bigendian(const struct reg *reg,
					 char *buf, size_t len)
{
  uint32_t r;
  struct storage s;
  s=reg_storage(reg);
  r=( ((uint32_t)eeprom_read_byte((void *)s.loc.eeprom.start+0)<<24) |
      ((uint32_t)eeprom_read_byte((void *)s.loc.eeprom.start+1)<<16) |
      ((uint32_t)eeprom_read_byte((void *)s.loc.eeprom.start+2)<<8) |
      ((uint32_t)eeprom_read_byte((void *)s.loc.eeprom.start+3)<<0) );
  snprintf_P(buf,len,PSTR("%" PRIu32),r);
}

static void eeprom_uint16_read(const struct reg *reg, char *buf, size_t len)
{
  uint16_t r;
  struct storage s;
  s=reg_storage(reg);
  r=eeprom_read_word((void *)s.loc.eeprom.start);
  snprintf_P(buf,len,PSTR("%" PRIu16),r);
}

static uint8_t eeprom_uint16_write(const struct reg *reg, const char *buf)
{
  struct storage s=reg_storage(reg);
  uint16_t r;
  if (sscanf_P(buf,PSTR("%u"),&r)!=1) return 1;
  eeprom_write_word((void *)s.loc.eeprom.start,r);
  return 0;
}

static void eeprom_uint8_read(const struct reg *reg, char *buf, size_t len)
{
  uint8_t r;
  struct storage s;
  s=reg_storage(reg);
  r=eeprom_read_byte((void *)s.loc.eeprom.start);
  snprintf_P(buf,len,PSTR("%" PRIu8),r);
}

static uint8_t eeprom_uint8_write(const struct reg *reg, const char *buf)
{
  struct storage s=reg_storage(reg);
  uint8_t r;
  if (sscanf_P(buf,PSTR("%u"),&r)!=1) return 1;
  eeprom_write_byte((void *)s.loc.eeprom.start,r);
  return 0;
}

static const char version_string[] PROGMEM = VERSION;

static void version_string_read(const struct reg *reg, char *buf, size_t len)
{
  (void)reg;
  strncpy_P(buf,version_string,len);
  buf[len-1]=0;
}

static void owb_addr_read(const struct reg *reg, char *buf, size_t len)
{
  struct storage s;
  uint8_t addr[8];
  s=reg_storage(reg);
  eeprom_read_block(addr,(void *)s.loc.eeprom.start,8);
  owb_format_addr(addr,buf,len);
}

static uint8_t owb_addr_write(const struct reg *reg, const char *buf)
{
  struct storage s;
  uint8_t addr[8];
  s=reg_storage(reg);
  if (owb_scan_addr(addr,buf)) {
    eeprom_write_block(addr,(void *)s.loc.eeprom.start,8);
    return 0;
  }
  return 1;
}

static void temperature_string_read(const struct reg *reg, char *buf, size_t len)
{
  struct storage s;
  s=reg_storage(reg);
  int32_t t;
  float tf;
  t=*(int32_t *)s.loc.ram;
  if (t==BAD_TEMP) {
    snprintf_P(buf,len,PSTR("None"));
  } else {
    tf=t/10000.0;
    snprintf_P(buf,len,PSTR("%f"),(double)tf);
  }
  buf[len-1]=0;
}

static void eeprom_temperature_string_read(const struct reg *reg,
					   char *buf, size_t len)
{
  struct storage s;
  s=reg_storage(reg);
  int32_t t;
  float tf;
  eeprom_read_block(&t,(void *)s.loc.eeprom.start,4);
  tf=t/10000.0;
  snprintf_P(buf,len,PSTR("%f"),(double)tf);
  buf[len-1]=0;
}

static uint8_t eeprom_temperature_string_write(const struct reg *reg,
					       const char *buf)
{
  struct storage s;
  int32_t t;
  float tf;
  s=reg_storage(reg);
  if (sscanf_P(buf,PSTR("%f"),&tf)!=1) return 1;
  t=(int32_t)(tf*10000.0);
  eeprom_write_block(&t,(void *)s.loc.eeprom.start,4);
  return 0;
}

static void error_counter_read(const struct reg *reg, char *buf, size_t len)
{
  struct storage s;
  s=reg_storage(reg);
  snprintf_P(buf,len,PSTR("%d"),*(uint8_t *)s.loc.ram);
  buf[len-1]=0;
}

static uint8_t error_counter_write(const struct reg *reg, const char *buf)
{
  struct storage s;
  s=reg_storage(reg);
  unsigned int dec;
  uint8_t ok;
  uint8_t *err;
  err=(uint8_t *)s.loc.ram;
  if (sscanf_P(buf,PSTR("%u"),&dec)!=1) return 1;
  /* Unlike all the other registers, writing to an error counter
     _decreases_ the value in the counter by the amount that you
     write, rather than setting the value.  This enables errors to be
     acknowledged without losing errors that occur between a read and
     subsequent write of the register.

     It is an error to try to decrease the counter by more than its
     current value.

     We disable interrupts while we decrease the counter because it is
     possible that some error counters might be updated by interrupt
     service routines (eg. serial RX errors, once I get around to
     implementing that).
  */
  ok=0;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    if (dec>*err) {
      ok=1;
    } else {
      *err-=dec;
    }
  }
  return ok;
}

static void valve_state_read(const struct reg *reg, char *buf, size_t len)
{
  (void)reg;
  switch (get_valve_state()) {
  case VALVE_CLOSED:
    strncpy_P(buf,PSTR("Closed"),len);
    break;
  case VALVE_OPENING:
    strncpy_P(buf,PSTR("Opening"),len);
    break;
  case VALVE_OPEN:
    strncpy_P(buf,PSTR("Open"),len);
    break;
  case VALVE_CLOSING:
    strncpy_P(buf,PSTR("Closing"),len);
    break;
  default:
    strncpy_P(buf,PSTR("Error"),len);
    break;
  }
  buf[len-1]=0;
}

/* avrdude can maintain a reprogramming count in the last four bytes of
   eeprom with the -y option */
const struct reg flashcount={
  .name="flashcnt",
  .description="Reprogram count",
  .storage.loc.eeprom={0x03fc,0x04},
  .storage.slen=11,
  .readstr=eeprom_uint32_read_bigendian,
};

const struct reg ident={
  .name="ident",
  .description="Station ident",
  .storage.loc.eeprom={0x03f4,0x08},
  .storage.slen=9,
  .readstr=eeprom_string_read,
  .writestr=eeprom_string_write,
};

const struct reg vtype={
  .name="vtype",
  .description="Valve type",
  .storage.loc.eeprom={0x3f1,0x01},
  .storage.slen=4,
  .readstr=eeprom_uint8_read,
  .writestr=eeprom_uint8_write,
};

const struct reg bl={
  .name="bl",
  .description="Backlight time",
  .storage.loc.eeprom={0x3f2,0x02},
  .storage.slen=6,
  .readstr=eeprom_uint16_read,
  .writestr=eeprom_uint16_write,
};

static const struct reg version={
  .name="ver",
  .description="Firmware version",
  .storage.loc.progmem=version_string,
  .storage.slen=sizeof(version_string)+1,
  .readstr=version_string_read,
};

#define proberegs(probe,addr)			\
  static const struct reg probe={		\
    .name=#probe,				\
    .description=#probe " probe reading",	\
    .storage.loc.ram=&probe##_temp,		\
    .storage.slen=12,				\
    .readstr=temperature_string_read,		\
  };						\
  static const struct reg probe##_id={		\
    .name=#probe "/id",				\
    .description=#probe " probe address",	\
    .storage.loc.eeprom={addr,0x08},		\
    .storage.slen=17,				\
    .readstr=owb_addr_read,			\
    .writestr=owb_addr_write,			\
  };						\
  static const struct reg probe##_c0={		\
    .name=#probe "/c0",				\
    .description=#probe " cal point 0",		\
    .storage.loc.eeprom={addr+0x8,0x02},	\
    .storage.slen=6,				\
    .readstr=eeprom_uint16_read,		\
    .writestr=eeprom_uint16_write,		\
  };						\
  static const struct reg probe##_c0r={		\
    .name=#probe "/c0r",			\
    .description=#probe " reading at c0",	\
    .storage.loc.eeprom={addr+0xa,0x02},	\
    .storage.slen=6,				\
    .readstr=eeprom_uint16_read,		\
    .writestr=eeprom_uint16_write,		\
  };

proberegs(t0,0x010);
proberegs(t1,0x020);
proberegs(t2,0x030);
proberegs(t3,0x040);

const struct reg v0={
  .name="v0",
  .description="Valve state",
  .storage.loc.pin=0,
  .storage.slen=8,
  .readstr=valve_state_read,
};
const struct reg set_hi={
  .name="set/hi",
  .description="Upper set point",
  .storage.loc.eeprom={0x050,0x04},
  .storage.slen=12,
  .readstr=eeprom_temperature_string_read,
  .writestr=eeprom_temperature_string_write,
};
const struct reg set_lo={
  .name="set/lo",
  .description="Lower set point",
  .storage.loc.eeprom={0x054,0x04},
  .storage.slen=12,
  .readstr=eeprom_temperature_string_read,
  .writestr=eeprom_temperature_string_write,
};
const struct reg mode={
  .name="mode",
  .description="Mode name",
  .storage.loc.eeprom={0x058,0x08},
  .storage.slen=9,
  .readstr=eeprom_string_read,
  .writestr=eeprom_string_write,
};

#define moderegs(mode,addr)	 \
  static const struct reg mode##_name={		\
    .name=#mode "/name",			\
    .description="Mode " #mode " name",		\
    .storage.loc.eeprom={addr,0x08},		\
    .storage.slen=9,				\
    .readstr=eeprom_string_read,		\
    .writestr=eeprom_string_write,		\
  };						\
  static const struct reg mode##_lo={		\
    .name=#mode "/lo",				\
    .description="Mode " #mode " low set",	\
    .storage.loc.eeprom={addr+8,0x04},		\
    .storage.slen=5,				\
    .readstr=eeprom_string_read,		\
    .writestr=eeprom_string_write,		\
  };						\
  static const struct reg mode##_hi={		\
    .name=#mode "/hi",				\
    .description="Mode " #mode " hi set",	\
    .storage.loc.eeprom={addr+12,0x04},		\
    .storage.slen=5,				\
    .readstr=eeprom_string_read,		\
    .writestr=eeprom_string_write,		\
  };
moderegs(m0,0x060);
moderegs(m1,0x070);
moderegs(m2,0x080);
moderegs(m3,0x090);

static const struct reg err_miss={
  .name="err/miss",
  .description="owb missing",
  .storage.loc.ram=&owb_missing_cnt,
  .storage.slen=4,
  .readstr=error_counter_read,
  .writestr=error_counter_write,
};
static const struct reg err_shrt={
  .name="err/shrt",
  .description="owb shorted",
  .storage.loc.ram=&owb_shorted_cnt,
  .storage.slen=4,
  .readstr=error_counter_read,
  .writestr=error_counter_write,
};
static const struct reg err_crc={
  .name="err/crc",
  .description="DS18B20 bad CRC",
  .storage.loc.ram=&owb_crcerr_cnt,
  .storage.slen=4,
  .readstr=error_counter_read,
  .writestr=error_counter_write,
};
static const struct reg err_pwr={
  .name="err/pwr",
  .description="DS18B20 no power",
  .storage.loc.ram=&owb_powererr_cnt,
  .storage.slen=4,
  .readstr=error_counter_read,
  .writestr=error_counter_write,
};

static const PROGMEM struct reg *const all_registers[]={
  &ident, &flashcount, &version, &bl,
  &t0,&t0_id,&t0_c0,&t0_c0r,
  &t1,&t1_id,&t1_c0,&t1_c0r,
  &t2,&t2_id,&t2_c0,&t2_c0r,
  &t3,&t3_id,&t3_c0,&t3_c0r,
  &v0,&vtype,
  &set_hi,&set_lo,&mode,
  &m0_name,&m0_lo,&m0_hi,
  &m1_name,&m1_lo,&m1_hi,
  &m2_name,&m2_lo,&m2_hi,
  &m3_name,&m3_lo,&m3_hi,
  &err_miss,&err_shrt,&err_crc,&err_pwr,
};

const struct reg *reg_number(uint8_t n)
{
  const struct reg *rv;
  if (n>=(sizeof(all_registers)/sizeof(const struct reg *))) return NULL;
  memcpy_P(&rv,&all_registers[n],sizeof(const struct reg *));
  return rv;
}

const struct reg *reg_by_name(const char *name)
{
  char buf[16];
  const struct reg *r;
  uint8_t n=0;
  do {
    r=reg_number(n);
    if (!r) return NULL;
    reg_name(r,buf);
    if (strcmp(buf,name)==0) return r;
    n++;
  } while(1);
}

const struct reg *reg_by_name_P(const char *name)
{
  char buf[16];
  const struct reg *r;
  uint8_t n=0;
  do {
    r=reg_number(n);
    if (!r) return NULL;
    reg_name(r,buf);
    if (strcmp_P(buf,name)==0) return r;
    n++;
  } while (1);
}

void reg_name(const struct reg *reg,char *buf)
{
  strncpy_P(buf,reg->name,8);
  buf[8]=0;
}
void reg_description(const struct reg *reg,char *buf)
{
  strncpy_P(buf,reg->description,16);
  buf[16]=0;
}
struct storage reg_storage(const struct reg *reg)
{
  struct storage r;
  memcpy_P(&r,&reg->storage,sizeof(struct storage));
  return r;
}
void reg_read_string(const struct reg *reg, char *buf, size_t len)
{
  readstr_fn readstr;
  memcpy_P(&readstr,&reg->readstr,sizeof(readstr_fn));
  if (readstr)
    readstr(reg,buf,len);
  else
    buf[0]=0;
}
uint8_t reg_write_string(const struct reg *reg, const char *buf)
{
  writestr_fn writestr;
  memcpy_P(&writestr,&reg->writestr,sizeof(writestr_fn));
  if (writestr)
    return writestr(reg,buf);
  else
    return 1;
}

void record_error(uint8_t *err)
{
  if (*err<0xff) (*err)++;
}
