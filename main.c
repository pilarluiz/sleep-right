/*
* EE459, Spring 2023 Capstone Project
* Team 7 -- SleepRight
* * Baran Cinbis, Evan Hashemi, Pilar Luiz
*/

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
void exit_sleepmode(void);

// Determine real time clock baud rate
#define FOSC 7372800    // CPU clock freq
//#define BDIV (FOSC / 100000 - 16) / 2 + 1
#define BDIV (FOSC / 50000 - 16) / 2 + 1

#define LEFT_BUTTON PB1         // PIN15
#define RIGHT_BUTTON PD6        // PIN12
#define TOGGLE_BUTTON PD7
#define SELECT_BUTTON PB0

// State Machine
enum states {SETCLOCK, SETALARM, SLEEP, WAKE, ALARM, DONE};
volatile int state;

// RTC global variables
uint8_t seconds_ones;
uint8_t seconds_tens;
uint8_t seconds = 0;
uint8_t minutes_ones;
uint8_t minutes_tens;
volatile uint8_t minutes = 0;
uint8_t hours_ones;
uint8_t hours_tens;
volatile uint8_t hours = 0;
uint8_t day = 0;
char* day_word;
char* days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
volatile uint8_t clock_index;     // Used to differentiate between setting hours and minutes; 0 is hours 1 is minutes
int time_changed = 0; 


// Wakeup (alarm) variables
volatile uint8_t wakeup_minutes;
volatile uint8_t wakeup_hours;
volatile uint8_t wakeup_day;         // not day of month but day of week 1-7, 1 being Sunday
volatile uint8_t alarm_index = 0;    // Used to differentiate between setting hours and minutes; 0 is hours 1 is minutes
volatile uint8_t alarm_set = 0;

// Watchdog Timer Variables
volatile int wdt_interrupt_count = 0;
volatile uint8_t sleep_debug = 0;
volatile uint8_t wake_init = 0;

// Rotary encoder variables
volatile uint8_t changed = 0; 
volatile uint8_t increment=0;
uint8_t DEMO = 1; 


// Timer0 variables
volatile int timer0_modulus;

// EEPROM 
#define EEPROM_ADDR  200
#define EEPROM_ALARM_HOUR EEPROM_ADDR 
#define EEPROM_ALARM_MINUTES (EEPROM_ADDR + 1)


uint8_t check_if_at_wakeup() {
    if((wakeup_hours == hours) && (wakeup_minutes == minutes)) {
        return 1;
    } else {
        return 0;
    }
}

uint8_t check_if_one_cycle_from_wakeup() {
    int total_minutes = 60*hours+minutes;
    int total_wakeup_minutes = 60*wakeup_hours+wakeup_minutes;
    int diff = (total_wakeup_minutes-total_minutes) % (24*60);

    if((diff<=90)) {        // within 1 sleep cycle of wakeup time
        return 1;
    }
    else {
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
}

void exit_sleepmode() {
    // turn timer off
    TCCR2B &= ~ ((1<<CS00) | (1<<CS01) | (1<<CS02));
    // turn sleep mode off
    SMCR &= ~(1<<SE);
}


void timer0_init() {
    TCCR0A |= (0b11 << WGM00);  // Set for Fast PWM mode using OCR0A for the modulus
    TCCR0A |= (0b10 << COM0B0); // Set to turn OC0B (PB5) on at 0x00, off when TCNT0=OCR0A    
    timer0_modulus = 255;
    OCR0A = timer0_modulus;     // Initial PWM pulse width
    
    // TIMSK0 |= (1 << TOIE0);     // Enable Timer0 overflow interrupt 
}

void vibrate_motor() {
    PORTD |= (1<<PD5);
    _delay_ms(5000);
    PORTD &= ~(1<<PD5);

}

// thanks to this dude: https://wolles-elektronikkiste.de/en/sleep-modes-and-power-management
void watchdog_init(void){
  cli();
  wdt_reset();
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = (1<<WDIE) | (0<<WDE) | (1<<WDP3) | (1<<WDP0);  // 8s / interrupt, no system reset
  sei();
}

ISR(WDT_vect){//put in additional code here
    // Toggle pin to track sleep mode interrupts
    PORTD ^= (1<<PD3);
    // watchdog goes off every 8s, for 5 minutes 5*60/8 = 37
    wdt_interrupt_count++; 

    if(!sleep_debug) {
        // to check real time clock every five minutes counter must reach 5*60/8 = 37
        if(wdt_interrupt_count >= 2) {      // check rtc every 16 seconds for debug
            wdt_interrupt_count = 0;
            // Check to see if we're at wakeup time or 1.5 hours from wakeup time
            if ( check_if_at_wakeup() || check_if_one_cycle_from_wakeup()) {
                state = WAKE; 
                wake_init=1;
                
            }
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
    // pulse_sensor_init();
    i2c_init(BDIV);
    encoder_init();

    // Pull-Up Resistors & Inputs/Outputs
    PORTD |= (1 << RIGHT_BUTTON) | (1 << TOGGLE_BUTTON);
    PORTB |= (1 << SELECT_BUTTON) | (1 << LEFT_BUTTON);

    // Variable Initializations
    state = SETCLOCK;
    //rtc_read();
    clock_index=0;
    wakeup_hours = eeprom_read_byte((void *) EEPROM_ALARM_HOUR); 
    if((wakeup_hours > 23)||(wakeup_hours < 0)){
        wakeup_hours = 6;
        serial_stringout("invalid eeprom read hours")
    }
    wakeup_minutes = eeprom_read_byte((void *) EEPROM_ALARM_HOUR); 
    if((wakeup_minutes > 59)||(wakeup_minutes < 0)){
        wakeup_minutes = 0;
        serial_stringout("invalid eeprom read min")
    }
    
    // NEW: turn on LED brightness
    uint8_t wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x53;
    wbuf[2] = 0x08;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    _delay_ms(1);

    lcd_clear();
    _delay_ms(1000);
    lcd_splash_screen();

    rtc_read();
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

    // DEMO variables
    uint8_t DEMO_DEEP_count = 0; 
    uint8_t DEMO_LIGHT_count = 0; 
    uint8_t DEMO_REM_count = 0; 

    
    DDRD |= (1<<PD5);   // haptic motor output; PD6 OC0A (pin12) or PD5 OC0B (pin11) 

    sleep_debug = 0;
    DDRD |= (1<<PD3);

    while (1) {
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
                eeprom_update_byte((void *) EEPROM_ALARM_HOUR, wakeup_hours);
                eeprom_update_byte((void *) EEPROM_ALARM_MINUTES, wakeup_minutes);
                state = SLEEP; 
                watchdog_init();
                alarm_set=1;
                // Turn off LCD display backlight
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x53;
                wbuf[2] = 0x01;
                status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);
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

            if (changed) {
                cli();
                //serial_stringout("encoder changed");
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
                // if(state_machine_debug) {
                //     char buf[40];
                //     snprintf(buf, 41, "CLOCK TIME: %02d:%02d ", hours, minutes);
                //     serial_stringout(buf);
                // }
                sei();
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
                // set clock time
                uint8_t status = rtc_set(day, hours, minutes, seconds);
                eeprom_update_byte((void *) EEPROM_ALARM_HOUR, wakeup_hours);
                eeprom_update_byte((void *) EEPROM_ALARM_MINUTES, wakeup_minutes);
                state = SLEEP;                // will need to uncomment later
                watchdog_init();
                alarm_set=0;
                // Turn off LCD display
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x53;
                wbuf[2] = 0x01;
                status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);

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

            if (changed) {
                cli();
                //serial_stringout("encoder changed");
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
                sei();
                // if(state_machine_debug) {
                //     char buf[40];
                //     snprintf(buf, 41, "CLOCK TIME: %02d:%02d ", wakeup_hours, wakeup_minutes);
                //     serial_stringout(buf);
                // }
            }
        } 
        
        else if (state == SLEEP) {

            // Initialize power down sleep mode
            wdt_reset();
            set_sleep_mode(SLEEP_MODE_PWR_DOWN); // choose power down mode
            sleep_mode(); // sleep now!
            // Transition out of SLEEP mode
            // if ( check_if_at_wakeup() || check_if_one_cycle_from_wakeup()) {
            //     state = WAKE; 
            //     interrupt_init();
            //     sleep_stage_init();
            //     // exit_sleepmode();
            // }
            
        } 
        
        else if (state == WAKE)
        {
            if(wake_init) {
                wake_init = 0;
                // turn off watchdog?
                WDTCSR &= ~((1<<WDCE) | (1<<WDE));
                WDTCSR &= ~((1<<WDIE) | (1<<WDP3) | (1<<WDP0));  // 8s / interrupt, no system reset
                // pulse sensor stuff
                pulse_sensor_init();
                interrupt_init();
                sleep_stage_init();
                // Turn on LCD display
                uint8_t wbuf[3];
                wbuf[0] = 0xFE;
                wbuf[1] = 0x53;
                wbuf[2] = 0x08;
                uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
                _delay_ms(1);
            }
            if(state_machine_debug) {
                serial_stringout("WAKE");
                lcd_debug_print("WAKE", 4);
            }
            uint8_t rtc_status = rtc_read();
            if(time_changed) {
                time_changed = 0; 
                lcd_rtc(hours, minutes);
            }
            // within an hour and a half of wakeup time

            if (check_if_at_wakeup() || stage_changed()) {
                stage_change = 0; 
                lcd_wakeup("GOOD MORNING :)", 15);
                // Vibrate motor, etc

                wake_up();
                state = ALARM;
            }


            // Print BPM
            if (saw_start_of_beat() && (state != ALARM)) {
                if (!DEMO) {
                    char buf[30];
                    snprintf(buf, 31, "BPM: '%2d'\n", BPM);
                    serial_stringout(buf); 

                    lcd_bpm(BPM);

                    // Add BPM to history array
                    bpm_history[bpm_history_idx++] = BPM;
                    bpm_history_idx %= 30;
                } 
                
                // Demo code with fake BPM data
                else {
                    uint8_t DEMO_BPM; 
                    if(DEMO_DEEP_count < 30) {
                        DEMO_DEEP_count++;
                        if(DEMO_DEEP_count % 2) {
                            DEMO_BPM = 42;
                        } else {
                            DEMO_BPM = 41;
                        }
                    } else if (DEMO_LIGHT_count < 30) {
                        DEMO_LIGHT_count++;
                        if(DEMO_LIGHT_count % 2) {
                            DEMO_BPM = 47;
                        } else {
                            DEMO_BPM = 48;
                        }
                    } else if (DEMO_REM_count < 30) {
                        DEMO_REM_count++;
                        if(DEMO_REM_count % 2) {
                            DEMO_BPM = 55;
                        } else {
                            DEMO_BPM = 56;
                        }
                    } else {
                        DEMO_BPM = 60;
                    }

                    bpm_history[bpm_history_idx++] = DEMO_BPM;
                    bpm_history_idx %= 30;

                    lcd_bpm(DEMO_BPM);
                }

                if(state != ALARM) {
                    uint8_t stage = sleep_stage();
                    lcd_stage(stage);
                }
            }  

        } else if (state == ALARM) {
            if(state_machine_debug) {
                serial_stringout("ALARM");
                lcd_debug_print("ALARM", 5);
            }


            uint8_t rtc_status = rtc_read();
            if(time_changed) {
                time_changed = 0; 
                lcd_rtc(hours, minutes);
            }
            // If any button is pressed, go to done
            if (!(PIND & (1 << TOGGLE_BUTTON))) {
                while (!(PIND & (1 << TOGGLE_BUTTON))) {}
                _delay_ms(10);
                state = DONE;
            }

            else if (!(PINB & (1 << SELECT_BUTTON))) {
                while (!(PINB & (1 << SELECT_BUTTON))) {}
                _delay_ms(10);
                state = DONE;
            }

            // If right button pressed, increase alarm index
            else if (!(PIND & (1 << RIGHT_BUTTON))) {
                while (!(PIND & (1 << RIGHT_BUTTON))) {}
                _delay_ms(10);
                state = DONE;
            } 

            else if (!(PINB & (1 << LEFT_BUTTON))) {
                while (!(PINB & (1 << LEFT_BUTTON))) {}
                _delay_ms(10);
                state = DONE;
            }

        } else if (state == DONE) {
            lcd_debug_print("Press SEL to Restart", 20);
            
            uint8_t rtc_status = rtc_read();
            if(time_changed) {
                time_changed = 0; 
                lcd_rtc(hours, minutes);
            }

            if (!(PINB & (1 << SELECT_BUTTON))) {
                while (!(PINB & (1 << SELECT_BUTTON))) {}
                _delay_ms(10);
                alarm_set=1;
                state = SETALARM;
                alarm_set=1;
                lcd_debug_print("                    ", 20);
                DEMO_DEEP_count = 0; 
                DEMO_LIGHT_count = 0; 
                DEMO_REM_count = 0; 
                // Disable pulse senosr interrupts
                disable_interrupt();
            }
        }
        
        _delay_ms(20);
        
        
        
    }

    return 0;   /* never reached */
}