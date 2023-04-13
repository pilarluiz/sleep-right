uint8_t reset_time();
uint8_t rtc_read();
uint8_t rtc_set(uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds);
void debug_rtc();

extern uint8_t seconds_ones;
extern uint8_t seconds_tens;
extern uint8_t seconds;
extern uint8_t minutes_ones;
extern uint8_t minutes_tens;
extern volatile uint8_t minutes;
extern uint8_t hours_ones;
extern uint8_t hours_tens;
extern volatile uint8_t hours;
extern uint8_t day;
extern char* day_word;
extern char* days[7];