#ifndef _temp_h
#define _temp_h

#include <stdint.h>

extern void read_probes(void);

#define BAD_TEMP 0x7FFFFFFF

extern int32_t t0_temp;
extern int32_t t1_temp;
extern int32_t t2_temp;
extern int32_t t3_temp;
extern uint8_t v0_state;
extern uint8_t v1_output_on;
extern uint8_t v2_output_on;

extern uint8_t get_valve_state(void);
#define VALVE_CLOSED 0
#define VALVE_OPENING 1
#define VALVE_OPEN 2
#define VALVE_CLOSING 3
#define VALVE_ERROR 4

#endif /* _temp_h */
