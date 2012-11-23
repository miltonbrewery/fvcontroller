#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "command.h"
#include "serial.h"
#include "registers.h"
#include "hardware.h"
#include "owb.h"

static uint8_t selected;

static const char PROGMEM okcmd[]="OK %s\n";
static const char PROGMEM noreg[]="ERR register %s does not exist\n";

static uint8_t it_is(const char *command)
{
  char buf[8];
  int len;
  strcpy_P(buf,command);
  len=strlen(buf);
  if (strncmp(buf,rxbuf,len)==0) {
    return len;
  }
  return 0;
}

static void select_cmd(const char *arg)
{
  char buf[9];
  reg_read_string(&ident,buf,9);
  if (strcmp(arg,buf)==0) {
    RS485_XMIT_ON();
    selected=1;
    serial_transmit_abort(); /* Stop any debug output */
    printf_P(PSTR("OK %s selected\n"),buf);
  } else {
    selected=0;
  }
}  

static void read_cmd(const char *arg)
{
  const struct reg *r;
  char buf[32];
  r=reg_by_name(arg);
  if (!r) {
    printf_P(noreg,arg);
    return;
  }
  reg_read_string(r,buf,32);
  printf_P(okcmd,buf);
}

static void help_cmd(const char *arg)
{
  const struct reg *r;
  char buf[32];
  int i;
  r=reg_by_name(arg);
  if (!r) {
    printf_P(PSTR("ERR Available registers: "));
    for (i=0; ; i++) {
      r=reg_number(i);
      if (r) {
	reg_name(r,buf);
	printf_P(PSTR("%s "),buf);
      } else { break; }
    }
    printf_P(PSTR("\n"));
  } else {
    reg_description(r,buf);
    printf_P(okcmd,buf);
  }
}

static void set_cmd(char *arg)
{
  char *val;
  const struct reg *r;
  /* We expect a space between the register name and new value */
  val=strchr(arg,' ');
  if (!val) {
    printf_P(PSTR("ERR SET needs argument after space\n"));
    return;
  }
  *val=0;
  val++;
  r=reg_by_name(arg);
  if (!r) {
    printf_P(noreg,arg);
    return;
  }
  if (reg_write_string(r,val)) {
    printf_P(PSTR("ERR write failed\n"));
    return;
  } else {
    /* We can read back the value using the big rxbuf, since we're throwing
       away its contents immediately afterwards */
    reg_read_string(r,val,SERIAL_RX_BUFSIZE-9);
    printf_P(PSTR("OK %s set to %s\n"),arg,val);
    return;
  }
}

static void scanbus_cmd(char *arg)
{
  int device_count,i;
  uint8_t addr[8];
  char buf[20];
  (void)arg;
  device_count=owb_count_devices();
  if (device_count==-1) {
    printf_P(PSTR("ERR Bus shorted to ground\n"));
    return;
  }
  if (device_count==-2) {
    printf_P(PSTR("ERR Bus shorted to +5v\n"));
    return;
  }
  printf_P(PSTR("OK %d sensors found"),device_count);
  if (device_count>0) {
    for (i=0; i<device_count; i++) {
      owb_get_addr(addr,i);
      owb_format_addr(addr,buf,sizeof(buf));
      printf_P(PSTR(" "));
      printf(buf);
    }
  }
  printf_P(PSTR("\n"));
}

void process_command(void)
{
  uint8_t len;
  if ((len=it_is(PSTR("SELECT ")))) {
    select_cmd(&rxbuf[len]);
    return;
  }
  if (selected) {
    if ((len=it_is(PSTR("READ ")))) {
      read_cmd(&rxbuf[len]);
    } else if ((len=it_is(PSTR("SET ")))) {
      set_cmd(&rxbuf[len]);
    } else if ((len=it_is(PSTR("HELP ")))) {
      help_cmd(&rxbuf[len]);
    } else if ((len=it_is(PSTR("SCANBUS")))) {
      scanbus_cmd(&rxbuf[len]);
    } else {
      printf_P(PSTR("ERR Unknown command; try SELECT, READ, SET, "
		    "HELP reg, SCANBUS\n"));
    }
  }
}
