#include <avr/interrupt.h>

void encoder_init(void);
void check_encoder(void);
void check_encoder_baran(void);

// Local variables
volatile uint8_t new_state, old_state;

// Variables from main.c
extern volatile int changed;
extern volatile int state;
extern volatile uint8_t wakeup_hours;
extern volatile uint8_t wakeup_minutes;
extern volatile uint8_t alarm_index;
extern volatile uint8_t hours; 
extern volatile uint8_t minutes;
extern volatile uint8_t clock_index; 
extern volatile uint8_t alarm_set;
