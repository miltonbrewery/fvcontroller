#include <avr/pgmspace.h>
#include "registers.h"
#include "hardware.h"
#include "lcd_hw.h"

#define ROW1 0x00
#define ROW2 0x40

/* Send a string to the display, padding to 'len' characters with spaces */
static void fixed_str(const char *str, uint8_t len)
{
  char c;
  while ((c=*str++) && len--) {
    lcd_data(c);
  }
  if (len==0xff) return;
  while (len--) {
    lcd_data(' ');
  }
}

/* Draw the "idle" display */
void lcd_home_screen(void)
{
  char buf[9];
  lcd_cmd(LCD_DISPCTL(1,0,0)); /* No cursor */
  lcd_cmd(LCD_DDADDR(ROW1));
  /* Top left is station name, up to 8 characters */
  reg_read_string(&ident,buf,9);
  fixed_str(buf,8);
  lcd_data(' ');
  /* Top right is current mode, up to 7 characters */
  reg_read_string(&mode,buf,9);
  fixed_str(buf,7);
  /* Next line */
  lcd_cmd(LCD_DDADDR(ROW2));
  /* Bottom left is current temp, up to 5 characters */
  reg_read_string(&t0,buf,8);
  fixed_str(buf,7);
  lcd_data(' ');
  lcd_data(' ');
  lcd_data(' ');
  /* Bottom right is valve state */
  reg_read_string(&v0,buf,8);
  fixed_str(buf,6);
}

void lcd_message(const char *message)
{
  int i;
  lcd_cmd(LCD_CLR);
  lcd_cmd(LCD_DDADDR(ROW1));
  for (i=0; message[i]; i++) {
    if (message[i]!='\n') {
      lcd_data(message[i]);
    } else {
      lcd_cmd(LCD_DDADDR(ROW2));
    }
  }
}

void lcd_message_P(const char *message)
{
  int i;
  lcd_cmd(LCD_CLR);
  lcd_cmd(LCD_DDADDR(ROW1));
  for (i=0; pgm_read_byte(message+i); i++) {
    if (pgm_read_byte(message+i)!='\n') {
      lcd_data(pgm_read_byte(message+i));
    } else {
      lcd_cmd(LCD_DDADDR(ROW2));
    }
  }
}

void lcd_init(void)
{
  lcd_cmd(LCD_FNSET(1,0)); /* Twoline, 5x8 font */
  lcd_cmd(LCD_ENTMODE(1,0)); /* Address increases, no display shift */
  lcd_cmd(LCD_DISPCTL(1,0,0)); /* On, no cursor */
  lcd_cmd(LCD_CLR); /* Clear display, home cursor */
}
