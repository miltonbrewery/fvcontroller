/* The one-wire bus is connected to PB0.  It should only ever have
   non-parasite-powered DS18B20s on it. */

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "owb.h"
#include "temp.h"
#include "registers.h"

#define OWB_READ() ( (PINB & (1<<PB0))==(1<<PB0))
#define OWB_LOW()  ( PORTB &= (~(1 << PB0)) )
#define OWB_HIGH() ( PORTB |= (1 << PB0) )
#define OWB_IN()   ( DDRB &= (~(1 << PB0 )) )
#define OWB_OUT()  ( DDRB |= (1 << PB0) )

#define OW_RECOVERY_TIME         10 /* us; increase for longer wires */

/* General one-wire-bus commands */
#define OWB_MATCH_ROM 0x55
#define OWB_SKIP_ROM 0xcc
#define OWB_SEARCH_ROM 0xf0

/* DS18B20 commands */
#define DS18X20_CONVERT_T 0x44
#define DS18X20_READ 0xbe

uint8_t owb_missing_cnt; /* Error counter: no devices detected */
uint8_t owb_shorted_cnt; /* Error counter: bus shorted */

void owb_init(void)
{
  OWB_HIGH();
  OWB_OUT();
}

/* Returns 0 for successful reset of at least one device; 1 for no devices
   detected, 2 for bus shorted to 0v, 3 for bus shorted to +5v */
static uint8_t owb_reset(void)
{
  uint8_t r;

  /* Idle 1-wire bus is high.  Pull low for 480us to reset. */
  OWB_LOW();
  OWB_OUT();
  _delay_us(20);
  if (OWB_READ()==1) return 3;
  _delay_us(460);

  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    OWB_IN();
    _delay_us(64);
    r=OWB_READ(); /* 0 if any slave is present or bus is shorted, else 1 */
  }
  
  /* After the presence pulse the slaves should all stop pulling down */
  _delay_us(480 - 64);
  if (OWB_READ()==0) {
    record_error(&owb_shorted_cnt);
    return 2;
  }

  if (r) record_error(&owb_missing_cnt);
  return r;
}

static uint8_t owb_bit_io(uint8_t b)
{
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    OWB_LOW();
    OWB_OUT();
    _delay_us(2);    // T_INT > 1usec accoding to timing-diagramm
    if (b) {
      OWB_HIGH();
      OWB_IN(); /* Raise bus early to write a 1, keep it low to write a 0 */
    }
    _delay_us(13);
    b=OWB_READ();
    _delay_us(43);
    OWB_HIGH();
    OWB_IN();
  } /* ATOMIC_BLOCK */
  _delay_us(OW_RECOVERY_TIME);
  return b;
}

static uint8_t owb_byte_wr(uint8_t b)
{
  uint8_t i = 8, j;
	
  do {
    j = owb_bit_io(b&1);
    b >>= 1;
    if (j) {
      b |= 0x80;
    }
  } while(--i);
	
  return b;
}

static uint8_t owb_byte_rd(void)
{
  return owb_byte_wr(0xFF); 
}

static uint8_t owb_rom_search( uint8_t diff, uint8_t *id )
{
  uint8_t i, j, next_diff;
  uint8_t b;
	
  if (owb_reset()) {
    return 0xff; /* No devices */
  }

  owb_byte_wr(OWB_SEARCH_ROM);
  next_diff=0x00;

  i=64; /* 64-bit ID */

  do {
    j=8; /* 8 bits */
    do {
      b=owb_bit_io(1); /* Read bit */
      if (owb_bit_io(1)) { /* Read complement bit */
	if (b) { /* Read 1 followed by 1 */
	  return 0; /* Error; exit now */
	}
      }
      else {
	if (!b) { /* Read 0 followed by 0 - clash at this location */
	  if (diff > i || ((*id & 1) && diff != i)) {
	    b = 1; /* Use 1 this time around */
	    next_diff = i;  /* Use 0 next time */
	  }
	}
      }
      owb_bit_io(b); /* Write the bit */
      *id >>= 1;
      if( b ) {
	*id |= 0x80; /* Store the bit */
      }
      
      i--;

    } while(--j);

    id++; /* Next byte */

  } while (i);

  return next_diff; /* Call again with this as diff to fetch next address */
}

int owb_count_devices(void)
{
  uint8_t addr[8];
  uint8_t diff=0xff;
  uint8_t r;
  int count;
  
  r=owb_reset();
  if (r==2) return -1; /* Bus shorted to 0v */
  if (r==3) return -2; /* Bus shorted to +5v */
  for (count=1; count<10; count++) {
    diff=owb_rom_search(diff,addr);
    if (diff==0xff) return 0; /* No devices found */
    if (diff==0) return count;
  }
  return count;
}

uint8_t owb_get_addr(uint8_t addr[8], uint8_t index)
{
  uint8_t diff=0xff;
  do {
    diff=owb_rom_search(diff,addr);
    if (diff==0xff) return 0; /* Failure */
    if (diff==0) {
      /* Last device */
      return (index==0);
    }
  }  while ((index--)>0);
  return 1;
}

void owb_start_temp_conversion(void)
{
  owb_reset();
  owb_byte_wr(OWB_SKIP_ROM);
  owb_byte_wr(DS18X20_CONVERT_T);
}

/* Dallas/Maxim 8-bit CRC over a buffer.  If the last byte of the buffer
   is its CRC then this will return 0 if the buffer is valid. */
static uint8_t owb_crc(const uint8_t *buf,int len)
{
  int i,j;
  uint8_t crc=0,b,bit;
  for (i=0; i<len; i++) {
    b=buf[i];
    for (j=8; j>0; j--) {
      bit=((crc^b)&0x01);
      if (bit) {
	crc^=0x18;
	crc>>=1;
	crc|=0x80;
      } else {
	crc>>=1;
      }
      b>>=1;
    }
  }
  return crc;
}

/* XXX not checked with negative temperatures */
int32_t owb_read_temp(uint8_t *id)
{
  int32_t temp;
  uint8_t i;
  uint8_t sp[9];
  if (owb_reset()) return BAD_TEMP;
  owb_byte_wr(OWB_MATCH_ROM);
  for (i=8; i>0; i--) {
    owb_byte_wr(*id++);
  }
  owb_byte_wr(DS18X20_READ);
  for (i=0; i<9; i++) {
    sp[i]=owb_byte_rd();
  }
  if (owb_crc(sp,9)!=0) return BAD_TEMP;
  temp=((sp[1]<<8)|sp[0])*625L;
  return temp;
}

static const char PROGMEM owb_addr_fstr[]="%02X%02X%02X%02X%02X%02X%02X%02X";

void owb_format_addr(const uint8_t *addr, char *buf, size_t len)
{
  snprintf_P(buf,len,owb_addr_fstr,
	     addr[0],addr[1],addr[2],addr[3],
	     addr[4],addr[5],addr[6],addr[7]);
  buf[len-1]=0;
}

int owb_scan_addr(uint8_t *addr, const char *buf)
{
  if (sscanf_P(buf,owb_addr_fstr,
	       &addr[0],&addr[1],&addr[2],&addr[3],
	       &addr[4],&addr[5],&addr[6],&addr[7])!=8) {
    return 0;
  }
  return 1;
}
