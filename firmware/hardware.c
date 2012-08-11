/* Buttons and low-level LCD access */

#include <util/delay.h>
#include "hardware.h"

void trigger_relay(uint8_t pin)
{
    OUTPUT_HIGH(PORTD, pin);
    _delay_ms(4); /* Datasheet says 4ms max operation time */
    OUTPUT_LOW(PORTD, pin);
}  

static void lcd_pulse_e(void)
{
  OUTPUT_HIGH(PORTC,LCD_E);
  _delay_us(0.5);
  OUTPUT_LOW(PORTC,LCD_E);
}

static void lcd_nibble(uint8_t n, uint8_t rs)
{
  PORTC=n|(rs<<LCD_RS);
  lcd_pulse_e();
}

void hw_lcd_byte(uint8_t b, uint8_t rs)
{
  lcd_nibble(b>>4,rs);
  lcd_nibble(b&0xf,rs);
  if (rs==0 && (b==0x01 || b==0x02 || b==0x03)) {
    /* Long commands: clear display, return home */
    _delay_ms(1.52);
  } else {
    _delay_us(37);
  }
}

void hw_init_lcd(void)
{
  _delay_ms(40);
  lcd_nibble(3,0);
  _delay_ms(4.1);
  lcd_nibble(3,0);
  _delay_ms(0.1);
  lcd_nibble(3,0);
  _delay_us(37);

  lcd_nibble(2,0);
  _delay_us(37);
  hw_lcd_byte(0x28,0); /* Twoline, 5x8 font */
  _delay_us(37);
  hw_lcd_byte(0x08,0);
  _delay_us(37);
}
