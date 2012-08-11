#include <util/delay.h>
#include "hardware.h"

void trigger_relay(uint8_t pin)
{
    OUTPUT_HIGH(PORTD, pin);
    _delay_ms(4); /* Datasheet says 4ms max operation time */
    OUTPUT_LOW(PORTD, pin);
}  
