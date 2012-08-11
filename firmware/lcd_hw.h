#ifndef _lcd_hw_h
#define _lcd_hw_h

/* Clear display (set whole display memory to 0x20)
   and set DDRAM address to 0 */
#define LCD_CLR 0x01

/* Set DDRAM address to 0, return cursor to original position */
#define LCD_HOME 0x02

/* inc is cursor move direction, shift is display shift */
#define LCD_ENTMODE(inc,shift) (0x04|((inc)?0x02:0)|((shift)?0x01:0))

/* on/off for display, cursor, and blinking cursor */
#define LCD_DISPCTL(disp,cursor,blink) \
  (0x08|((disp)?0x04:0)|((cursor)?0x02:0)|((blink)?0x01:0))

/* Cursor or display shift */
#define LCD_SHIFT(shift,right) (0x10|((shift)?0x08:0)|((right)?0x04:0))

/* two line display, 5x11 or 5x8 font */
#define LCD_FNSET(twoline,font) (0x20|((twoline)?0x08:0)|((font)?0x04:0))

/* Character RAM address */
#define LCD_CGADDR(addr) (0x40|((addr)&0x3f))

/* Display RAM address */
#define LCD_DDADDR(addr) (0x80|((addr)&0x7f))

#endif /* _lcd_hw_h */
