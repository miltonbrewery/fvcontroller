#ifndef _lcd_h
#define _lcd_h

/* If status is not NULL, display it as the second line.  It's a string
   in program memory. */
extern void lcd_home_screen(const char *status);

/* Display a message; \n starts the next line and should only occur once */
extern void lcd_message(const char *message);
extern void lcd_message_P(const char *message);

extern void lcd_init(void);

#endif /* _lcd_h */
