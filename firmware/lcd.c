#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "registers.h"
#include "hardware.h"
#include "lcd_hw.h"
#include "temp.h"

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

static void var_str(const char *str)
{
  char c;
  while ((c=*str++)) lcd_data(c);
}

/* Draw the "idle" display */
void lcd_home_screen(void)
{
  char buf[16];
  struct storage s;
  int32_t s_hi,s_lo;
  float tf,slf,shf;
  lcd_cmd(LCD_DISPCTL(1,0,0)); /* No cursor */
  lcd_cmd(LCD_DDADDR(ROW1));
  /* Top left is station name, up to 8 characters */
  reg_read_string(&ident,buf,9);
  var_str(buf);
  lcd_data(' ');
  /* Now current set range */
  s=reg_storage(&set_lo);
  eeprom_read_block(&s_lo,(void *)s.loc.eeprom.start,4);
  slf=s_lo/10000.0;
  s=reg_storage(&set_hi);
  eeprom_read_block(&s_hi,(void *)s.loc.eeprom.start,4);
  shf=s_hi/10000.0;
  snprintf_P(buf,16,PSTR("%0.1f-%0.1f"),(double)slf,(double)shf);
  var_str(buf);
  lcd_data(' ');
  lcd_data(' ');
  /* Next line */
  lcd_cmd(LCD_DDADDR(ROW2));
  /* Bottom left is current temp, up to 5 characters */
  if (t0_temp==BAD_TEMP) {
    sprintf_P(buf,PSTR("XXXXX"));
  } else {
    tf=t0_temp/10000.0;
    snprintf_P(buf,9,PSTR("%0.1f"),(double)tf);
  }
  fixed_str(buf,5);
  lcd_data(' ');

  /* Current mode */
  reg_read_string(&mode,buf,9);
  fixed_str(buf,8);
  lcd_data(' ');
  /* Bottom right is valve state truncated to 1 character */
  if (v0_state) {
    if (read_valve(VALVE1_STATE)) {
      buf[0]='|'; /* Fully open */
    } else {
      buf[0]='O'; /* Opening */
    }
  } else {
    if (read_valve(VALVE1_STATE)) {
      buf[0]='o'; /* Closing */
    } else {
      buf[0]='-'; /* Closed */
    }
  }
  fixed_str(buf,1);
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
