#ifndef _serial_h
#define _serial_h

#include "config.h"

extern void serial_init(uint16_t bps);

extern uint8_t rx_data_available(void);
extern void ack_rx_data(void);
extern void serial_transmit_abort(void);
extern char rxbuf[SERIAL_RX_BUFSIZE];

#endif /* _serial_h */
