#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#include "adc.h"
#include "pulse_sensor.h"
#include "serial.h"
#include "pulse_interrupt.h"
#include "i2c.h"
#include "sleep_stage.h"
#include "encoder.h"
#include "lcd.h"

// Prototyping
void debug_echoing(void);
void debug_rtc(void);
uint8_t reset_time(void);
void lcd_clear(void);
void lcd_rtc(uint8_t, uint8_t);
void lcd_alarm(uint8_t, uint8_t);
uint8_t rtc_read(void);
uint8_t rtc_set(uint8_t, uint8_t, uint8_t, uint8_t);
uint8_t check_if_at_wakeup(void);
uint8_t check_if_one_cycle_from_wakeup(void);
int check_state(void);
void wake_up(void);
void enter_idle_sleep_mode(void);
// void timer_2_init(void);
void timer0_init(void);
void vibrate_motor(void);
void wake_up(void);

// Determine real time clock baud rate
#define FOSC 7372800    // CPU clock freq
//#define BDIV (FOSC / 100000 - 16) / 2 + 1
#define BDIV (FOSC / 50000 - 16) / 2 + 1

#define LEFT_BUTTON PB1         // PIN15
#define RIGHT_BUTTON PD6        // PIN12
#define TOGGLE_BUTTON PD7
#define SELECT_BUTTON PB0

// State Machine
enum states {SETCLOCK, SETALARM, SLEEP, WAKE, DONE};
volatile int state;

// RTC global variables
uint8_t seconds_ones;
uint8_t seconds_tens;
uint8_t seconds;
uint8_t minutes_ones;
uint8_t minutes_tens;
volatile uint8_t minutes;
uint8_t hours_ones;
uint8_t hours_tens;
volatile uint8_t hours;
uint8_t day;
char* day_word;
char* days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
volatile int clock_index = 0;     // Used to decipher between setting hours and minutes

// Alarm variables
volatile uint8_t alarm_hours;
volatile uint8_t alarm_minutes;
volatile int alarm_index = 0;    // Used to decipher between setting hours and minutes

// User set wakeup time
uint8_t wakeup_minutes_ones;
uint8_t wakeup_minutes_tens;
uint8_t wakeup_minutes;
uint8_t wakeup_hours_ones;
uint8_t wakeup_hours_tens;
uint8_t wakeup_hours;
uint8_t wakeup_day;         // not day of month but day of week 1-7, 1 being Sunday

// Timer2 variables
volatile int timer2_interrupt_count = 0;
volatile int changed = 0; 

// Timer0 variables
int timer0_modulus;

// TODO
// void timer_2_init() {
//     TCCR2A |= ((1<<) | (1<<));
// }

void enter_idle_sleep_mode() {              // does order of sleep mode enable statements matter??
    // Set prescalar for timer2
    TCCR2B |= ((1<<CS00) | (1<<CS02));      // prescalar 1024 -> slowest counting timer2 possible
    // Idle sleep mode -> SM2:0 = 000
    SMCR &= ~((1<<SM2) | (1<<SM1) | (1<<SM0));
    // Sleep Enable pin set to 1
    SMCR |= (1<<SE);
    
}

// TODO: Sleep mode functions
int check_state() {
    if(!rtc_read()) {       // no rtc error
        // at wakeup time? Then must wake up
        if( check_if_at_wakeup() ) {      // if within at wakeup time, must wakeup regardless of sleep cycle
            wake_up();
            return 1;
        } else if(check_if_one_cycle_from_wakeup()) {      // if 1 sleep cycle (1.5 hour) of wakeup, check sleep cycle
            // check if there is potentially an early wakeup
            // Determine current stage of sleep
            uint8_t stage = sleep_stage();
            if (stage == LIGHT_SLEEP) {
                serial_stringout("STAGE: LIGHT\n"); 
            } else if (stage == DEEP_SLEEP) {
                serial_stringout("STAGE: DEEP\n"); 
            } else if (stage == REM_SLEEP) {
                serial_stringout("STAGE: REM\n"); 
            } else {
                serial_stringout("STAGE: AWAKE\n"); 
            }

            if(stage_changed()) {    // Wake up when changing sleep stages
                wake_up();
                return 1;
            } else {
                return 0; 
            }
        } else {
            return 0;
        }
    } else {
        // Error reading RTC
        return -1;
    }
}

// TODO: check for day as well
uint8_t check_if_at_wakeup() {
    int day_diff = wakeup_day - day;
    // check if within 5 minutes of wakeup time
    if( (wakeup_hours == hours && (abs(wakeup_minutes-minutes) <= 5) && (day_diff==0)) ||                 
        ((abs(wakeup_hours-hours) == 1) && (abs(wakeup_minutes-minutes) >= 55) && (day_diff==0)) ||         
        ((abs(wakeup_hours-hours) == 24) && (abs(wakeup_minutes-minutes) >= 55) && ((day_diff==1)||(day_diff==-6)))) {
        return 1;
    } else {
        return 0;
    }
}

/*
3 Cases for 1.5 hours away
Format for 3 cases:
Wakeup time range, begin to end
Current time range,	begin to end

CASE 1:
9       to 	9:29		same day
7:30	to	7:59
CASE 2:
9:30	to 	9:59		same day
8:00 	to 	8:29
CASE 3:
00 		to 	1:29		day up by 1 (difference in day by 1 unless at last day of week)
22:30	to 	23:59
*/

// TODO: check for day
uint8_t check_if_one_cycle_from_wakeup() {
    int total_minutes = 60*hours+minutes;
    int total_wakeup_minutes = 60*wakeup_hours+wakeup_minutes;
    int diff = total_wakeup_minutes-total_minutes;
    int day_diff = wakeup_day - day;
    if((diff<=90) && (day_diff==0)) {        // within 1 sleep cycle of wakeup time
        return 1;
    } else if((total_wakeup_minutes>=0) && (total_wakeup_minutes<=89) && ((-1*diff)<=1350) && ((day_diff==1) || (day_diff==-6))) {         // accounts for case 3
        return 1;
    } else {
        return 0;
    }
}

uint8_t abs(uint8_t value) {
    if(value<0) {
        return -1*value;
    } else {
        return value;
    }
}

void wake_up() {
    serial_stringout("at wakeup time \n");
    vibrate_motor();
    // turn timer off
    TCCR2B &= ~ ((1<<CS00) | (1<<CS01) | (1<<CS02));
    // turn sleep mode off
    SMCR &= ~(1<<SE);
}

ISR(TIMER2_COMPA_vect) {
    // Disable sleep enable
    SMCR &= ~(1<<SE);

    // with prescalar of 1024, maximum time before timer2 counts up is 1/(16,000,000/1024) * (2^8-1) = 0.01632s
    timer2_interrupt_count++; 
    // to check real time clock every five minutes counter must reach 5*60/0.01632 = 18,383 times
    if(timer2_interrupt_count >= 18383) {
        timer2_interrupt_count = 0;
        // Check RTC Clock
        check_state();
        
    }
    // Go back to sleep
    SMCR |= (1<<SE); 
}

//TODO connect motor to OC0A (PB6); currently is connectd to wrong pin (OC2A)
void timer0_init() {
    TCCR0A |= (0b11 << WGM00);  // Set for Fast PWM mode using OCR0A for the modulus
    TCCR0A |= (0b10 << COM0A0); // Set to turn OC0A (PB6) on at 0x00, off when TCNT0=OCR0A    
    timer0_modulus = 128;
    OCR0A = 128;     // Initial PWM pulse width
    TIMSK0 |= (1 << TOIE0);     // Enable Timer0 overflow interrupt 
}

void vibrate_motor() {
    TCCR0B |= (0b101 << CS00);  // 256 prescalar; turn on timer0
    int cycle_counter = 0;
    while(cycle_counter < 25) {     // vibrate motor for 25*200ms = 5s
        cycle_counter++;
        _delay_ms(200);
        timer0_modulus+=4;          
        OCR0A = timer0_modulus;     // increase duty cycle of PWM
    }
}

int main(void)
{
    // Voltage Level Test
    unsigned short ubrr = ( ( FOSC / 16 ) / 9600) - 1; 

    // Initializations
    serial_init(ubrr); 

    _delay_ms(2000);
    adc_init();
    pulse_sensor_init();
    interrupt_init();
    sleep_stage_init();
    i2c_init(BDIV);
    timer0_init();      // PWM for buzzer
    encoder_init();

    // Pull-Up Resistors & Inputs/Outputs
    DDRD |= (1<<PD6);   // haptic motor output; PD6 OC0A (pin12) or PD5 OC0B (pin11) 
    PORTD |= (1 << RIGHT_BUTTON) | (1 << TOGGLE_BUTTON);
    PORTB |= (1 << SELECT_BUTTON) | (1 << LEFT_BUTTON);

    // Variable Initializations
    state = SETCLOCK;
    hours = 0;
    minutes = 0; 
    alarm_hours = 0; 
    alarm_minutes = 0; 
    
    lcd_splash_screen();

    // Initial time printout
    lcd_alarm(alarm_hours, alarm_minutes);
    lcd_rtc(hours, minutes);

    serial_stringout("testing\n");
    _delay_ms(50);

    while (1) {
        if (state == SETCLOCK) {
            // Toggling
            if (PIND & (1 << TOGGLE_BUTTON)) {
                while (PIND & (1 << TOGGLE_BUTTON)) {}
                serial_stringout("SETCLOCK: Toggle Pressed\n");
                lcd_debug_print("SETCLK: TOGGLE", 14);
                state = SETALARM; 
            }

            else if (PINB & (1 << SELECT_BUTTON)) {
                while (PINB & (1 << SELECT_BUTTON)) {}
                serial_stringout("SETCLOCK: Select Pressed\n");
                lcd_debug_print("SETCLK: SELECT", 14);

                // Irrelevant values now
                uint8_t day = 0; 
                uint8_t seconds = 0; 
                
                // Set RTC
                uint8_t status = rtc_set(day, hours, minutes, seconds);
                state = SLEEP; 
            }

            // If right button pressed, increase alarm index
            else if (PIND & (1 << RIGHT_BUTTON)) {
                while (PIND & (1 << RIGHT_BUTTON)) {}
                serial_stringout("SETCLOCK: Right Pressed\n");
                lcd_debug_print("SETCLK: RIGHT ", 14);
                clock_index = (clock_index + 1) % 2;
            } 

            else if (PINB & (1 << LEFT_BUTTON)) {
                while (PINB & (1 << LEFT_BUTTON)) {}
                serial_stringout("SETCLOCK: Left Pressed\n");
                lcd_debug_print("SETCLK: LEFT  ", 14);
                clock_index = (clock_index - 1) % 2;
            }

            if (changed) {
                lcd_rtc(hours, minutes);
            }
        } 
        
        else if(state == SETALARM) {
            // Toggling
            if (PIND & (1 << TOGGLE_BUTTON)) {
                while (PIND & (1 << TOGGLE_BUTTON)) {}
                serial_stringout("SETALARM: Toggle Pressed\n");
                state = SETCLOCK; 
            }

            else if (PINB & (1 << SELECT_BUTTON)) {
                while (PINB & (1 << SELECT_BUTTON)) {}
                serial_stringout("SETALARM: Select Pressed\n");

                // Irrelevant values now
                uint8_t day = 0; 
                uint8_t seconds = 0; 
                
                // Set RTC
                uint8_t status = rtc_set(day, hours, minutes, seconds);
                state = SLEEP; 
            }

            // If right button pressed, increase alarm index
            else if (PIND & (1 << RIGHT_BUTTON)) {
                while (PIND & (1 << RIGHT_BUTTON)) {}
                serial_stringout("SETALARM: Right Pressed\n");
                alarm_index = (alarm_index + 1) % 2;
            } 

            else if (PINB & (1 << LEFT_BUTTON)) {
                while (PINB & (1 << LEFT_BUTTON)) {}
                serial_stringout("SETALARM: Left Pressed\n");
                alarm_index = (alarm_index - 1) % 2;
            }

            if (changed) {
                lcd_alarm(alarm_hours, alarm_minutes);
            }
        } 
        
        else if (state == SLEEP) {
            // TODO
        } 
        
        else if (state == WAKE)
        {
            // TODO
        }
        
        // _delay_ms(500);
        // lcd_rtc(12,12);
        // lcd_alarm(8,3);
        // _delay_ms(2000);
        
        
        /*
        if (saw_start_of_beat()) {
            char buf[30];
            snprintf(buf, 31, "BPM: '%2d'\n", BPM);
            serial_stringout(buf); 

            // Add BPM to history array
            bpm_history[bpm_history_idx++] = BPM;
            bpm_history_idx %= 30;

            uint8_t stage = sleep_stage();
            // TODO: Do something with sleep stage (print to LCD, etc. )
        }  
        */

        _delay_ms(20);
        
    }

    return 0;   /* never reached */
}