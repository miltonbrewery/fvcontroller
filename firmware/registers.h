#ifndef _registers_h
#define _registers_h

#include <avr/pgmspace.h>

struct storage {
  union {
    struct {
      uint16_t start;
      uint8_t length;
    } eeprom;
    void *ram;
    const void *progmem;
  } loc;
  uint8_t slen; /* How many bytes of buffer is required to read as a string */
};

struct reg;

typedef void (*readstr_fn)(const struct reg *reg,char *buf,size_t len);
typedef void (*writestr_fn)(const struct reg *reg,const char *buf);

struct PROGMEM reg {
  const char name[8];
  const char description[16];
  const struct storage storage;
  readstr_fn readstr;
  writestr_fn writestr;
};

extern const struct reg *reg_number(uint8_t n);
extern void reg_name(const struct reg *reg, char *buf); /* buf 9 bytes */
extern void reg_description(const struct reg *reg, char *buf); /* buf 17 bytes */
extern struct storage reg_storage(const struct reg *reg);
extern void reg_read_string(const struct reg *reg, char *buf, size_t len);
extern void reg_write_string(const struct reg *reg, char *buf);

extern struct reg ident;

#endif /* _registers_h */
