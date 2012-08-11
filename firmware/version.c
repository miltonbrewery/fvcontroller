#include <avr/pgmspace.h>

char version[] PROGMEM = VERSION;

void get_version(char *buf, size_t len)
{
  strncpy_P(buf,version,len);
  buf[len-1]=0;
}

size_t get_version_length(void)
{
  return strlen_P(version)+1;
}
