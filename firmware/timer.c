#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "buttons.h"

void timer_init(void)
{
  /* Set up timer1 to generate an interrupt every 0.1 seconds */
  /* Write PRTIM0 bit to zero to enable timer0  - this is the default value */

  /* Select the clock source - clk/64 = 250KHz */
  TCCR1B=(1<<CS11)|(1<<CS10);
  
  /* To get an interrupt at 10Hz we need to count up to 25000. */
  OCR1A=25000;

  /* Use CTC mode (WGM13:0=4) to reset counter when it reaches OCR1A */
  TCCR1B|=(1<<WGM12);

  /* Set OCF1A to enable interrupt on TOP */
  TIFR1=(1<<OCF1A);

  /* Enable interrupt */
  TIMSK1=(1<<OCIE1A);
}

uint8_t tprobe_timer;
uint16_t backlight_timer;

ISR(TIMER1_COMPA_vect)
{
  buttons_poll();
  if (tprobe_timer>0) tprobe_timer--;
  if (backlight_timer!=0xffff && backlight_timer>0) backlight_timer--;
}
