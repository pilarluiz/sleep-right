#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

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
volatile uint8_t clock_index;     // Used to differentiate between setting hours and minutes; 0 is hours 1 is minutes


// Wakeup (alarm) variables
volatile uint8_t wakeup_minutes;
volatile uint8_t wakeup_hours;
volatile uint8_t wakeup_day;         // not day of month but day of week 1-7, 1 being Sunday
volatile uint8_t alarm_index = 0;    // Used to differentiate between setting hours and minutes; 0 is hours 1 is minutes
volatile uint8_t alarm_set = 0;

// Watchdog Timer Variables
volatile int wdt_interrupt_count = 0;
volatile uint8_t sleep_debug = 0;

// Rotary encoder variables
volatile uint8_t changed = 0; 
volatile uint8_t increment=0;


// Timer0 variables
volatile int timer0_modulus;

// TODO
// void timer_2_sleep_init() {
    
//     // reset interrupt counter
//     timer2_interrupt_count = 0;
//     // CTC mode
//     //TCCR2A |= ((1<<) | (1<<));
//     TCCR2B |= (1<<WGM22);
//     // Enable CTC interrupt
//     TIMSK2 |= (1<<OCIE2A);
//     // Trigger interrupt at maximum counter value
//     OCR2A = 255;
//     // Switching timer 2 to asynchronous for sleep mode; take some precautions (see p.201 on atmega datasheet)
//     TCNT2=0;
//     // global interrupts
//     sei();
// }

// void enter_idle_sleep_mode() {              // does order of sleep mode enable statements matter??
//     timer_2_sleep_init();
//     // Set prescalar for timer2, start timer
//     TCCR2B |= ((1<<CS00) | (1<<CS02));      // prescalar 1024 -> slowest counting timer2 possible
//     // Idle sleep mode -> SM2:0 = 000
//     SMCR &= ~((1<<SM2) | (1<<SM1) | (1<<SM0));
//     // Sleep Enable pin set to 1
//     SMCR |= (1<<SE);
// }

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

// ISR(TIMER2_COMPA_vect) {
//     // Disable sleep enable
//     SMCR &= ~(1<<SE);

//     // with prescalar of 1024, maximum time before timer2 counts up is 1/(16,000,000/1024) * (2^8-1) = 0.01632s
//     timer2_interrupt_count++; 

//     if(!sleep_debug) {
//         // to check real time clock every five minutes counter must reach 5*60/0.01632 = 18,383 times
//         if(timer2_interrupt_count >= 18383) {
//             timer2_interrupt_count = 0;
//             // Check to see if we're at wakeup time or 1.5 hours from wakeup time
//             check_state();
//         }
//     } else {
//          // test sleep mode
//          // 5/0.01632 = 306
//         if(timer2_interrupt_count >= 1000) {    //306
//             timer2_interrupt_count = 0;
//             // toggle motor
//             PORTD ^= (1<<PD3);
//         }
//     }
//     // Go back to sleep
//     SMCR |= (1<<SE); 
// }

//TODO connect motor to OC0A (PB6); currently is connectd to wrong pin (OC2A)
void timer0_init() {
    TCCR0A |= (0b11 << WGM00);  // Set for Fast PWM mode using OCR0A for the modulus
    TCCR0A |= (0b10 << COM0B0); // Set to turn OC0B (PB5) on at 0x00, off when TCNT0=OCR0A    
    timer0_modulus = 128;
    OCR0A = timer0_modulus;     // Initial PWM pulse width
    TIMSK0 |= (1 << TOIE0);     // Enable Timer0 overflow interrupt 
}

void vibrate_motor() {
    TCCR0B |= (0b111 << CS00);  // 256 prescalar; turn on timer0
    int cycle_counter = 0;
    while(cycle_counter < 25) {     // vibrate motor for 25*200ms = 5s
        cycle_counter++;
        _delay_ms(200);
        timer0_modulus+=4;          
        OCR0A = timer0_modulus;     // increase duty cycle of PWM
    }
}

// void timer2_pwm_init(void)
// {
//     TCCR2A |= (0b11 << WGM20);  // Fast PWM mode, modulus = 256
//     TCCR2A |= (0b10 << COM2B0); // Turn D11 on at 0x00 and off at OCR2A
//     OCR2A = 128;                // Initial pulse duty cycle of 50%
//     TCCR2B |= (0b111 << CS20);  // Prescaler = 1024 for 16ms period
// }

// thanks to this dude: https://wolles-elektronikkiste.de/en/sleep-modes-and-power-management
void watchdog_init(void){
  cli();
  wdt_reset();
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = (1<<WDIE) | (0<<WDE) | (1<<WDP3) | (1<<WDP0);  // 8s / interrupt, no system reset
  sei();
}

ISR(WDT_vect){//put in additional code here
    // Turn off watchdog timer


    // watchdog goes off every 8s, for 5 minutes 5*60/8 = 37
    wdt_interrupt_count++; 

    if(!sleep_debug) {
        // to check real time clock every five minutes counter must reach 5*60/8 = 37
        if(wdt_interrupt_count >= 37) {
            wdt_interrupt_count = 0;
            // Check to see if we're at wakeup time or 1.5 hours from wakeup time
            check_state();
        }
    } else {
        PORTD ^= (1<<PD3);
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
    //interrupt_init();
    sleep_stage_init();
    i2c_init(BDIV);
    encoder_init();

    // Pull-Up Resistors & Inputs/Outputs
    PORTD |= (1 << RIGHT_BUTTON) | (1 << TOGGLE_BUTTON);
    PORTB |= (1 << SELECT_BUTTON) | (1 << LEFT_BUTTON);

    // Variable Initializations
    state = SETCLOCK;
    rtc_read();
    clock_index=0;
    wakeup_hours = 6; 
    wakeup_minutes = 0; 
    
    lcd_clear();
    _delay_ms(1000);
    lcd_splash_screen();

    // Initial time printout
    lcd_alarm(wakeup_hours, wakeup_minutes);
    lcd_rtc(hours, minutes);
    // Underline cursor on
    // uint8_t wbuf[2];
    // wbuf[0] = 0xFE;
    // wbuf[1] = 0x47;
    // uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 2, NULL, 0);
    // _delay_ms(1);

    // state machine debug
    uint8_t state_machine_debug=0;

    // enable global interrupts
    sei();

    

    //                  HAPTIC MOTOR
    //DDRD |= (1<<PD5);   // haptic motor output; PD6 OC0A (pin12) or PD5 OC0B (pin11) 
    //timer0_init();
    //sei();
    //TCCR0B |= (0b100 << CS00);  // 256 prescalar; turn on timer0

    // timer2 pwm debug
    // DDRD |= (1<<PD3);
    // timer2_init();

    //                  SLEEP MODE 
    //sei();
    // sleep_debug=1;
    // DDRD |= (1<<PD3);
    // enter_idle_sleep_mode();

    // DDRD |= (1<<PD3);
    // SMCR |= (1<<SE);
    // SMCR &= ~((1<<SM2) | (1<<SM0));
    // SMCR |= ((1<<SM1));
    // Sleep Enable pin set to 1

    // _delay_ms(2000);
    // PORTD |= (1<<PD3);
    watchdog_init();
    sleep_debug = 1;
    DDRD |= (1<<PD3);

    while (1) {
        

        
        // watchdog timer interrupt power down mode demo
        // PORTD |= (1<<PD3);
        // _delay_ms(3500);
        // PORTD &= ~(1<<PD3);
        // _delay_ms(3500);
        wdt_reset();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN); // choose power down mode
        sleep_mode(); // sleep now!
        
        /*
        if (state == SETCLOCK) {
            // Toggling
            if (!(PIND & (1 << TOGGLE_BUTTON))) {
                while (!(PIND & (1 << TOGGLE_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETCLOCK: Toggle Pressed\n");
                    lcd_debug_print("SETCLK: TOGGLE", 14);
                }
                
                state = SETALARM; 
                alarm_set=1;
            }

            else if (!(PINB & (1 << SELECT_BUTTON))) {
                while (!(PINB & (1 << SELECT_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETCLOCK: Select Pressed\n");
                    lcd_debug_print("SETCLK: SELECT", 14);
                }
                
                // Set RTC
                uint8_t status = rtc_set(day, hours, minutes, seconds);
                state = SETALARM; 
                alarm_set=1;
            }

            // If right button pressed, increase clock index
            else if (!(PIND & (1 << RIGHT_BUTTON))) {
                while (!(PIND & (1 << RIGHT_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETCLOCK: Right Pressed\n");
                    lcd_debug_print("SETCLK: RIGHT ", 14);
                }
                clock_index = (clock_index + 1) % 2;
                // Move cursor
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x45;
                wbuf[2] = 0x13;
                uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);
            } 

            else if (!(PINB & (1 << LEFT_BUTTON))) {
                while (!(PINB & (1 << LEFT_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETCLOCK: Left Pressed\n");
                    lcd_debug_print("SETCLK: LEFT  ", 14);
                }
                clock_index = (clock_index - 1) % 2;
                // Move cursor
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x45;
                wbuf[2] = 0x10;
                uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);
            }

            // TODO: logic to change clock hours, minutes with encoder

            if (changed) {
                changed=0;
                if(increment) {
                    if(!alarm_set) {            // setting clock time       
                        if(!clock_index) {   
                            if(hours<23) {
                                hours++;
                            } else {
                                hours = 0;
                            }
                        } else {
                            if(minutes<59) {
                                minutes++;
                            } else {
                                minutes=0;
                            } 
                        }
                    } else {                    // setting alarm time
                        if(!alarm_index) {
                            if(wakeup_hours<23) {
                                wakeup_hours++;
                            } else {
                                wakeup_hours = 0;
                            }
                        } else {
                            if(wakeup_minutes<59) {
                                wakeup_minutes++;
                            } else {
                                wakeup_minutes=0;
                            } 
                        }
                    }
                } else {
                    if(!alarm_set) {            // setting clock time       
                        if(!clock_index) {   
                            if(hours>0) {
                                hours--;
                            } else {
                                hours = 23;
                            }
                        } else {
                            if(minutes>0) {
                                minutes--;
                            } else {
                                minutes=59;
                            } 
                        }
                    } else {                    // setting alarm time
                        if(!alarm_index) {
                            if(wakeup_hours>0) {
                                wakeup_hours--;
                            } else {
                                wakeup_hours = 23;
                            }
                        } else {
                            if(wakeup_minutes>0) {
                                wakeup_minutes--;
                            } else {
                                wakeup_minutes=59;
                            } 
                        }
                    }
                }
                lcd_rtc(hours, minutes);
            }
        } 
        
        else if(state == SETALARM) {
            // Toggling
            if (!(PIND & (1 << TOGGLE_BUTTON))) {
                while (!(PIND & (1 << TOGGLE_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETALARM: Toggle Pressed\n");
                    lcd_debug_print("SETARM: TOGGLE", 14);
                }
                state = SETCLOCK; 
                alarm_set=0;
            }

            else if (!(PINB & (1 << SELECT_BUTTON))) {
                while (!(PINB & (1 << SELECT_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETALARM: Select Pressed\n");
                    lcd_debug_print("SETARM: SELECT ", 14);
                }
                state = SLEEP;                // will need to uncomment later
                alarm_set=0;
            }

            // If right button pressed, increase alarm index
            else if (!(PIND & (1 << RIGHT_BUTTON))) {
                while (!(PIND & (1 << RIGHT_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETALARM: Right Pressed\n");
                    lcd_debug_print("SETARM: RIGHT ", 14);
                }
                alarm_index = (alarm_index + 1) % 2;
                // Move cursor
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x45;
                wbuf[2] = 0x53;
                uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);
            } 

            else if (!(PINB & (1 << LEFT_BUTTON))) {
                while (!(PINB & (1 << LEFT_BUTTON))) {}
                _delay_ms(10);
                if(state_machine_debug) {
                    serial_stringout("SETALARM: Left Pressed\n");
                    lcd_debug_print("SETARM: LEFT  ", 14);
                }
                alarm_index = (alarm_index - 1) % 2;
                // Move cursor
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x45;
                wbuf[2] = 0x50;
                uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);
            }

            // TODO: logic to change wakeup_hours, wakeup_minutes, wakeup_day with encoder

            if (changed) {
                changed = 0;
                if(increment) {
                    if(!alarm_set) {            // setting clock time       
                        if(!clock_index) {   
                            if(hours<23) {
                                hours++;
                            } else {
                                hours = 0;
                            }
                        } else {
                            if(minutes<59) {
                                minutes++;
                            } else {
                                minutes=0;
                            } 
                        }
                    } else {                    // setting alarm time
                        if(!alarm_index) {
                            if(wakeup_hours<23) {
                                wakeup_hours++;
                            } else {
                                wakeup_hours = 0;
                            }
                        } else {
                            if(wakeup_minutes<59) {
                                wakeup_minutes++;
                            } else {
                                wakeup_minutes=0;
                            } 
                        }
                    }
                } else {
                    if(!alarm_set) {            // setting clock time       
                        if(!clock_index) {   
                            if(hours>0) {
                                hours--;
                            } else {
                                hours = 23;
                            }
                        } else {
                            if(minutes>0) {
                                minutes--;
                            } else {
                                minutes=59;
                            } 
                        }
                    } else {                    // setting alarm time
                        if(!alarm_index) {
                            if(wakeup_hours>0) {
                                wakeup_hours--;
                            } else {
                                wakeup_hours = 23;
                            }
                        } else {
                            if(wakeup_minutes>0) {
                                wakeup_minutes--;
                            } else {
                                wakeup_minutes=59;
                            } 
                        }
                    }
                }
                lcd_alarm(wakeup_hours, wakeup_minutes);
            }
        } 
        
        else if (state == SLEEP) {
            // TODO: sleep mode, figure out how to turn off LCD backlight
        } 
        
        else if (state == WAKE)
        {
            // TODO
        }
        
        */
        
        
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