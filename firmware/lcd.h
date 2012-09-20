#ifndef _lcd_h
#define _lcd_h

extern void lcd_home_screen(void);

/* Display a message; \n starts the next line and should only occur once */
extern void lcd_message(const char *message);
extern void lcd_message_P(const char *message);

extern void lcd_init(void);

#endif /* _lcd_h */
