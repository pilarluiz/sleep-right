#include <stdint.h>

void lcd_splash_screen();
void lcd_clear();
void lcd_clear_row(uint8_t row);
void lcd_rtc(uint8_t hour, uint8_t minutes);
void lcd_alarm(uint8_t hour, uint8_t minutes);
void lcd_debug_print(char* buf, int len);