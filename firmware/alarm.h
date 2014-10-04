#ifndef _alarm_h
#define _alarm_h

#include <stdlib.h>
#include <inttypes.h>

#define ALARM_NO_TEMPERATURE 0x01
#define ALARM_TEMPERATURE_LOW 0x02
#define ALARM_TEMPERATURE_HIGH 0x04
#define ALARM_VALVE_STUCK 0x08

extern uint8_t alarm;

extern const char *alarm_to_string_P();

#endif /* _alarm_h */
