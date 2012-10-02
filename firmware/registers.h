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
    uint8_t pin;
  } loc;
  uint8_t slen; /* How many bytes of buffer is required to read as a string */
};

struct reg;

typedef void (*readstr_fn)(const struct reg *reg,char *buf,size_t len);
typedef uint8_t (*writestr_fn)(const struct reg *reg,const char *buf);

struct PROGMEM reg {
  const char name[8];
  const char description[16];
  const struct storage storage;
  readstr_fn readstr;
  writestr_fn writestr;
};

extern const struct reg *reg_number(uint8_t n);
extern const struct reg *reg_by_name(const char *name);
extern const struct reg *reg_by_name_P(const char *name);
extern void reg_name(const struct reg *reg, char *buf); /* buf 9 bytes */
extern void reg_description(const struct reg *reg, char *buf); /* buf 17 bytes */
extern struct storage reg_storage(const struct reg *reg);
extern void reg_read_string(const struct reg *reg, char *buf, size_t len);
/* Returns 0 for success, non-zero for failure */
extern uint8_t reg_write_string(const struct reg *reg, const char *buf);

extern void record_error(uint8_t *err);

/* Registers accessed by name in the code */
extern const struct reg ident,bl,t0,v0,v0_s,v1_s,set_hi,set_lo,mode;

#endif /* _registers_h */
