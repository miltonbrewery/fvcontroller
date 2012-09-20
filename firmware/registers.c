#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "registers.h"
#include "owb.h"
#include "temp.h"

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

static void eeprom_string_write(const struct reg *reg, const char *buf)
{
  struct storage s;
  s=reg_storage(reg);
  eeprom_write_block(buf,(void *)s.loc.eeprom.start,s.loc.eeprom.length);
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

static void eeprom_uint16_write(const struct reg *reg, const char *buf)
{
  (void)reg;
  (void)buf;
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

static void owb_addr_write(const struct reg *reg, const char *buf)
{
  (void)reg;
  (void)buf;
}

/* XXX *_temperature_* known not to work with negative temperatures yet! */
static void temperature_string_read(const struct reg *reg, char *buf, size_t len)
{
  struct storage s;
  s=reg_storage(reg);
  int32_t t;
  int16_t temp;
  int16_t frac;
  t=*(int32_t *)s.loc.ram;
  temp=t/10000;
  frac=t%10000;
  snprintf_P(buf,len,PSTR("%" PRIi16 ".%04" PRIi16),temp,frac);
  buf[len-1]=0;
}

static void eeprom_temperature_string_read(const struct reg *reg,
					   char *buf, size_t len)
{
  struct storage s;
  s=reg_storage(reg);
  int32_t t;
  int16_t temp;
  int16_t frac;
  eeprom_read_block(&t,(void *)s.loc.eeprom.start,4);
  temp=t/10000;
  frac=t%10000;
  snprintf_P(buf,len,PSTR("%" PRIi16 ".%04" PRIi16),temp,frac);
  buf[len-1]=0;
}

static void eeprom_temperature_string_write(const struct reg *reg,
					    const char *buf)
{
  struct storage s;
  int32_t t;
  int16_t temp;
  int16_t frac;
  s=reg_storage(reg);
  sscanf_P(buf,PSTR("%i.%i"),&temp,&frac);
  t=((int32_t)temp*10000)+(int32_t)frac;
  eeprom_write_block(&t,(void *)s.loc.eeprom.start,4);
}

static void valve_state_read(const struct reg *reg, char *buf, size_t len)
{
  struct storage s;
  s=reg_storage(reg);
  uint8_t state;
  state=*(uint8_t *)s.loc.ram;
  if (state==0) {
    strncpy_P(buf,PSTR("Closed"),len);
  } else {
    strncpy_P(buf,PSTR("Open"),len);
  }
  buf[len-1]=0;
}

/* avrdude can maintain a reprogramming count in the last four bytes of
   eeprom with the -y option */
struct reg flashcount={
  .name="flashcnt",
  .description="Reprogram count",
  .storage.loc.eeprom={0x03fc,0x04},
  .storage.slen=11,
  .readstr=eeprom_uint32_read_bigendian,
};

struct reg ident={
  .name="ident",
  .description="Station ident",
  .storage.loc.eeprom={0x03f4,0x08},
  .storage.slen=9,
  .readstr=eeprom_string_read,
  .writestr=eeprom_string_write,
};

static struct reg version={
  .name="ver",
  .description="Firmware version",
  .storage.loc.progmem=version_string,
  .storage.slen=sizeof(version_string)+1,
  .readstr=version_string_read,
};

struct reg t0={
  .name="t0",
  .description="t0 probe reading",
  .storage.loc.ram=&t0_temp,
  .storage.slen=7,
  .readstr=temperature_string_read,
};
static struct reg t0_id={
  .name="t0/id",
  .description="t0 probe address",
  .storage.loc.eeprom={0x010,0x08},
  .storage.slen=17,
  .readstr=owb_addr_read,
  .writestr=owb_addr_write,
};
static struct reg t0_c0={
  .name="t0/c0",
  .description="t0 cal point 0",
  .storage.loc.eeprom={0x018,0x02},
  .storage.slen=6,
  .readstr=eeprom_uint16_read,
  .writestr=eeprom_uint16_write,
};
static struct reg t0_c0r={
  .name="t0/c0r",
  .description="t0 reading at c0",
  .storage.loc.eeprom={0x01a,0x02},
  .storage.slen=6,
  .readstr=eeprom_uint16_read,
  .writestr=eeprom_uint16_write,
};
struct reg v0={
  .name="v0",
  .description="Valve 0",
  .storage.loc.ram=&v0_state,
  .storage.slen=7,
  .readstr=valve_state_read,
};
struct reg set_hi={
  .name="set/hi",
  .description="Upper set point",
  .storage.loc.eeprom={0x050,0x04},
  .storage.slen=7,
  .readstr=eeprom_temperature_string_read,
  .writestr=eeprom_temperature_string_write,
};
struct reg set_lo={
  .name="set/lo",
  .description="Lower set point",
  .storage.loc.eeprom={0x054,0x04},
  .storage.slen=7,
  .readstr=eeprom_temperature_string_read,
  .writestr=eeprom_temperature_string_write,
};
struct reg mode={
  .name="mode",
  .description="Mode name",
  .storage.loc.eeprom={0x058,0x08},
  .storage.slen=9,
  .readstr=eeprom_string_read,
  .writestr=eeprom_string_write,
};

static const PROGMEM struct reg *all_registers[]={
  &ident, &flashcount, &version,
  &t0,&t0_id,&t0_c0,&t0_c0r,&v0,
  &set_hi,&set_lo,&mode,
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
void reg_write_string(const struct reg *reg, const char *buf)
{
  writestr_fn writestr;
  memcpy_P(&writestr,&reg->writestr,sizeof(writestr_fn));
  if (writestr)
    writestr(reg,buf);
}

void record_error(uint8_t *err)
{
  if (*err<0xff) (*err)++;
}
