/* The serial port is used for two things in this project: debug
   output, and communication over the RS485 bus.  (There is no
   provision in the hardware for debug input - the RX line on the FTDI
   header is not connected.)

   The RS485 bus is full-duplex, but can only have one slave unit
   transmitting at once.  The controller sends SELECT commands to
   determine which slave unit transmits.  If we are selected then we
   MUST transmit (and cannot produce debug output); if we are not
   selected then we MUST NOT transmit (and may produce debug output).
   If we are selected while sending debug output, we stop immediately.

   Commands are received over the RS485 bus one line at a time.  They
   are executed on reception of the terminating '\n' by the main loop
   (i.e. interrupts on).  If we receive bytes while executing a
   command, we discard them.  If there's a buffer overflow, the
   receive buffer is discarded and we wait for the next '\n' before
   starting to receive again (and increase the appropriate error
   counter).  Reception into the receive buffer always starts at index
   0.

   The transmit buffer is a ring.  It is written into using printf()
   and friends through stdout.  This blocks if the buffer is full.
*/

#include <stdio.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "serial.h"
#include "config.h"
#include "registers.h"
#include "hardware.h"

/* Receive buffer.  rxptr is the offset in buffer for the next
   received byte; 0xfe means "disabled until next '\n'", and 0xff
   means "disabled until command processing is done" */
static uint8_t rxptr;
char rxbuf[SERIAL_RX_BUFSIZE];

/* Transmit buffer.  txend is the first free byte of the buffer
   (i.e. next byte to be added will go there).  txnext is the next
   byte of the buffer to send.  If txend==txnext then the buffer is
   empty.  If (txend+1)%SERIAL_TX_BUFSIZE==txnext then the buffer is
   full and adding a character should block until there is space. */
static uint8_t txend,txnext;
static uint8_t txbuf[SERIAL_TX_BUFSIZE];

uint8_t rx_data_available(void)
{
  return (rxptr==0xff);
}

void ack_rx_data(void)
{
  rxptr=0;
}

/* Immediately stop serial transmission */
void serial_transmit_abort(void)
{
  cli();
  txend=txnext=0;
  sei();
}

/* Must be called with interrupts enabled */
static int serial_transmit(char c, FILE *stream)
{
  uint8_t next;
  (void)stream;
  next=(txend+1)%SERIAL_TX_BUFSIZE;
  /* Block until there is room in the buffer */
  cli();
  while (next==txnext) {
    sei();
    _delay_us(1);
    cli();
  }
  txbuf[txend]=c;
  txend=next;
  UCSR0B |= (1 << UDRIE0); /* Enable transmit interrupt */
  sei();
  return 0;
}

/* Our stream for stdout */
static FILE serial_stdout =
  FDEV_SETUP_STREAM(serial_transmit, NULL, _FDEV_SETUP_WRITE);

/* Initialise the serial hardware; NB global interrupts must be
   disabled while doing this */
void serial_init(uint16_t bps)
{
  UBRR0=F_CPU/16/bps-1;
  UCSR0B=(1<<RXEN0)|(1<<TXEN0);
  UCSR0C=(3<<UCSZ00);
  stdout=&serial_stdout;

  /* Enable the receive interrupt */
  UCSR0B |= (1 << RXCIE0);
}

/* Byte received interrupt */
ISR(USART_RX_vect)
{
  uint8_t rxbyte;
  /* XXX if we are going to deal with receive errors, we should read
     the error flags here */
  rxbyte=UDR0;
  if (rxptr==0xfe) {
    /* Discard characters until '\n' is received */
    if (rxbyte=='\n' || rxbyte=='\r') {
      rxptr=0;
      return;
    }
  } else if (rxptr==0xff) {
    /* Discard characters until command processing is finished */
    return;
  } else if (rxbyte=='\n' || rxbyte=='\r') {
    char selectcmd[9];
    /* Complete command received */
    rxbuf[rxptr]=0;
    rxptr=0xff;
    /* We deal with SELECT commands here in this interrupt routine,
       because we want to be able to stop transmitting immediately on
       receipt of a SELECT for a different unit.  The command is still
       reported to the main loop in the usual way so it can issue an
       ack and do book-keeping. */
    strcpy_P(selectcmd,PSTR("SELECT "));
    if (strncmp(selectcmd,rxbuf,7)==0) {
      reg_read_string(&ident,selectcmd,9);
      if (strcmp(&rxbuf[7],selectcmd)!=0) {
	RS485_XMIT_OFF();
      }
    }
    rxptr=0xff;
    return;
  }
  rxbuf[rxptr]=rxbyte;
  rxptr++;
  if (rxptr>=SERIAL_RX_BUFSIZE) {
    /* Buffer overflow; now discard characters until '\n' is received */
    rxptr=0xfe;
  }
}

/* Transmit interrupt - data register empty */
ISR(USART_UDRE_vect)
{
  if (txend==txnext) {
    /* Buffer is empty.  Disable the interrupt. */
    UCSR0B &= ~(1 << UDRIE0);
    return;
  }
  UDR0=txbuf[txnext];
  txnext=(txnext+1)%SERIAL_TX_BUFSIZE;
}
