#include "hardware.h"
#include "lcd_hw.h"

void hello_lcd(void)
{
  lcd_data('H');
  lcd_data('e');
  lcd_data('l');
  lcd_data('l');
  lcd_data('o');
  lcd_cmd(LCD_DDADDR(0x40));
  lcd_data('g');
  lcd_data('u');
  lcd_data('q');
}

void lcd_init(void)
{
  lcd_cmd(LCD_FNSET(1,0)); /* Twoline, 5x8 font */
  lcd_cmd(LCD_ENTMODE(1,0)); /* Address increases, no display shift */
  lcd_cmd(LCD_DISPCTL(1,0,0)); /* On, no cursor */
  lcd_cmd(LCD_CLR); /* Clear display, home cursor */
}
