#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "registers.h"

uint32_t ctmp;

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
  (void)reg;
  (void)buf;
}

static void eeprom_uint32_read(const struct reg *reg, char *buf, size_t len)
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

static void eeprom_uint32_write(const struct reg *reg, const char *buf)
{
  uint32_t r;
  struct storage s;
  s=reg_storage(reg);
  r=strtoul(buf,NULL,0);
  eeprom_update_byte((void *)s.loc.eeprom.start+3,(r&0x000000ff)>>0);
  eeprom_update_byte((void *)s.loc.eeprom.start+2,(r&0x0000ff00)>>8);
  eeprom_update_byte((void *)s.loc.eeprom.start+1,(r&0x00ff0000)>>16);
  eeprom_update_byte((void *)s.loc.eeprom.start+0,(r&0xff000000)>>24);
}

static const char version_string[] PROGMEM = VERSION;

static void version_string_read(const struct reg *reg, char *buf, size_t len)
{
  (void)reg;
  strncpy_P(buf,version_string,len);
  buf[len-1]=0;
}

struct reg ctmp_reg={
  .name="CTMP",
  .description="Current temp",
  .storage.loc.ram=&ctmp,
  .storage.slen=7,
};

/* avrdude can maintain a reprogramming count in the last four bytes of
   eeprom with the -y option */
struct reg flashcount={
  .name="flashcnt",
  .description="Reprogram count",
  .storage.loc.eeprom={0x03fc,0x04},
  .storage.slen=11,
  .readstr=eeprom_uint32_read,
  .writestr=eeprom_uint32_write,
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

static const PROGMEM struct reg *all_registers[]={
  &ctmp_reg, &ident, &flashcount, &version,
};

const struct reg *reg_number(uint8_t n)
{
  const struct reg *rv;
  if (n>=(sizeof(all_registers)/sizeof(const struct reg *))) return NULL;
  memcpy_P(&rv,&all_registers[n],sizeof(const struct reg *));
  return rv;
}

void reg_name(const struct reg *reg,char *buf)
{
  strcpy_P(buf,reg->name);
  buf[8]=0;
}
void reg_description(const struct reg *reg,char *buf)
{
  strcpy_P(buf,reg->description);
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
void reg_write_string(const struct reg *reg, char *buf)
{
  writestr_fn writestr;
  memcpy_P(&writestr,&reg->writestr,sizeof(writestr_fn));
  if (writestr)
    writestr(reg,buf);
}
