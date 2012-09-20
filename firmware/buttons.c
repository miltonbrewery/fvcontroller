#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "config.h"
#include "buttons.h"

static uint8_t current_buttons;
static uint8_t repeat_timer;
static volatile uint8_t buttons_pressed;

uint8_t get_buttons(void)
{
  uint8_t b;
  cli();
  b=buttons_pressed;
  sei();
  return b;
}

void ack_buttons(void)
{
  cli();
  buttons_pressed=0;
  sei();
}

/* We use the pin change interrupt to take a note of button presses
   even when they occur between timer interrupts.  The button press is
   not processed until the next timer interrupt. */
void buttons_init(void)
{
  /* Select pins 3,4,5 */
  PCMSK0|=(1<<PCINT3)|(1<<PCINT4)|(1<<PCINT5);

  /* Enable interrupt */
  PCICR|=(1<<PCIE0);
}

void buttons_poll(void)
{
  uint8_t b;

  b=PINB & BUTTONS_MASK;

  /* Has anything changed? */
  if (b^current_buttons) {
    repeat_timer=BUTTON_REPEAT_INITIAL;
    current_buttons=b;
  } else {
    /* If repeat_timer reaches zero, re-report pressed buttons and 
       reset it. */
    repeat_timer--;
    if (repeat_timer==0) {
      repeat_timer=BUTTON_REPEAT;
      buttons_pressed|=(~b)&BUTTONS_MASK;
    }
  }
}

/* We deal with button presses immediately.  Button releases and
   auto-repeat are left to the timer interrupt, which will help deal
   with bouncy contacts. */
ISR(PCINT0_vect)
{
  uint8_t b;
  b=PINB & BUTTONS_MASK;
  if (b^current_buttons) {
    repeat_timer=BUTTON_REPEAT_INITIAL;
    buttons_pressed|=(~b)&BUTTONS_MASK;
    current_buttons=current_buttons & b;
  }
}
