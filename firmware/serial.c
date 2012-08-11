#include <stdio.h>
#include <avr/io.h>
#include "serial.h"

static int serial_transmit(char c, FILE *stream)
{
  (void)stream;
  while (!(UCSR0A&(1<<UDRE0)));
  UDR0=c;
  return 0;
}

/* Set up stdout for debugging */
static FILE serial_stdout =
  FDEV_SETUP_STREAM(serial_transmit, NULL, _FDEV_SETUP_WRITE);

/* Initialise the serial hardware */
void serial_init(uint16_t bps)
{
  UBRR0=F_CPU/16/bps-1;
  UCSR0B=(1<<RXEN0)|(1<<TXEN0);
  UCSR0C=(3<<UCSZ00);
  stdout=&serial_stdout;
}
