#include <string.h>
#include <avr/pgmspace.h>
#include "alarm.h"

uint8_t alarm;

const char *alarm_to_string_P()
{
  if (alarm & ALARM_NO_TEMPERATURE) {
    return PSTR("Probe error");
  } else if (alarm & ALARM_TEMPERATURE_LOW) {
    return PSTR("Temperature low");
  } else if (alarm & ALARM_TEMPERATURE_HIGH) {
    return PSTR("Temperature high");
  } else if (alarm & ALARM_VALVE_STUCK) {
    return PSTR("Valve stuck");
  }
  return PSTR("None");
}
