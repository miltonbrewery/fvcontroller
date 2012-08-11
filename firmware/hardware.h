#ifndef _hardware_h
#define _hardware_h

#include <avr/io.h>

#define OUTPUT_LOW(port,pin)    port &= ~(1<<pin)
#define OUTPUT_HIGH(port,pin)   port |= (1<<pin)
#define SET_INPUT(portdir,pin)  portdir &= ~(1<<pin)
#define SET_OUTPUT(portdir,pin) portdir |= (1<<pin)


#define BACKLIGHT_ON() OUTPUT_HIGH(PORTD,PD3)
#define BACKLIGHT_OFF() OUTPUT_LOW(PORTD,PD3)

#define RS485_XMIT_ON() OUTPUT_HIGH(PORTD,PD2)
#define RS485_XMIT_OFF() OUTPUT_LOW(PORTD,PD2)

#define VALVE1_SET PD4
#define VALVE1_RESET PD5
#define VALVE2_SET PD6
#define VALVE2_RESET PD7

#endif /* _hardware_h */
