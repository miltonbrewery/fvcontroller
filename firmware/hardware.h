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

extern void trigger_relay(uint8_t pin);
extern uint8_t read_valve(uint8_t pin);

#define VALVE1_SET PD4
#define VALVE1_RESET PD5
#define VALVE1_STATE PB1
#define VALVE2_STATE PB2
#define VALVE2_SET PD6
#define VALVE2_RESET PD7

/* LCD data is on bits 0--3 of PORTC */
#define LCD_RS PC4
#define LCD_E PC5

extern void hw_init_lcd(void);
extern void hw_lcd_byte(uint8_t byte, uint8_t rs);
#define lcd_cmd(cmd) do { hw_lcd_byte((cmd),0); } while (0)
#define lcd_data(data) do { hw_lcd_byte((data),1); } while (0)

#endif /* _hardware_h */
