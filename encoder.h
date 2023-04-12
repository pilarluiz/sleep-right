#include <avr/interrupt.h>

void encoder_init(void);
void check_encoder(void);

// Local variables
volatile uint8_t new_state, old_state;

// Variables from main.c
extern volatile int changed;
extern volatile int state;
extern volatile uint8_t alarm_hours;
extern volatile uint8_t alarm_minutes;
extern volatile int alarm_index;
extern volatile uint8_t hours; 
extern volatile uint8_t minutes;
extern volatile int clock_index; 
