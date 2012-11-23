#ifndef _owb_h
#define _owb_h

#include <stdlib.h>

extern uint8_t owb_missing_cnt;
extern uint8_t owb_shorted_cnt;

extern void owb_init(void);

/* Retrieve device address from bus given index; returns 1 for success
   or 0 if there's no device at that index. */
extern uint8_t owb_get_addr(uint8_t addr[8], uint8_t index);

/* Return number of devices on bus; -1 indicates bus shorted to 0v,
   -2 indicates bus shorted to +5v */
extern int owb_count_devices(void);

/* Start temperature conversion */
extern void owb_start_temp_conversion(void);

/* Read temperature */
#define BAD_TEMP 0x7FFFFFFF
extern int32_t owb_read_temp(uint8_t *id);

/* Format a bus address for output */
extern void owb_format_addr(const uint8_t *addr, char *buf, size_t len);

/* Scan a string to a bus address; returns 1 on success */
extern int owb_scan_addr(uint8_t *addr, const char *buf);

#endif /* _owb_h */
