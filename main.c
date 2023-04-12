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

#define LEFT_BUTTON PD5
#define RIGHT_BUTTON PD6
#define TOGGLE_BUTTON PD7
#define SELECT_BUTTON PB0

#define LETTER_A  0x41;
#define LETTER_B 0x42;
#define LETTER_C 0x43
#define LETTER_D 0x44
#define LETTER_E 0x45
#define LETTER_F 0x46
#define LETTER_G 0x47
#define LETTER_H 0x48
#define LETTER_I 0x49
#define LETTER_J 0x4A
#define LETTER_K 0x4B
#define LETTER_L 0x4C
#define LETTER_M 0x4D
#define LETTER_N 0x4E
#define LETTER_O 0x4F
#define LETTER_P 0x50
#define LETTER_Q 0x51
#define LETTER_R 0x52

#define SEMICOLON 0x3A

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


/*
    Lab 3/4: Echo characters typed in terminal back to user -- used for debugging purposes
*/
void debug_echoing()
{
    while(1) {
        char in = serial_in(); 

        char buf[30];
        if ((in == 'a') || (in == 'e') || (in == 'i') || (in == 'o') || (in == 'u') || (in == 'y')) {
            snprintf(buf, 31, "You entered the vowel '%c'\n", in);
            serial_stringout(buf);
        } else {
            snprintf(buf, 31, "You entered the consonant '%c'\n", in);
            serial_stringout(buf);
        }
    }
}

void debug_rtc() {
    // read test
    
    uint8_t status;
    uint8_t status_delayed;
    uint8_t addr = 0;
    uint8_t rbuf[1];
    uint8_t seconds;
    uint8_t tens;
    uint8_t time;

    status = i2c_io(0xD0, &addr, 1, NULL, 0, rbuf, 1);
    if(status == 0) {
        serial_stringout("success ");
    } else {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d ", status);
        serial_stringout(buf);
    }
    seconds = rbuf[0] & (0x0F);
    tens = (rbuf[0] & (0x70)) >> 4;
    time = seconds + 10*tens;
    char buf[30];
    snprintf(buf, 31, "Time: %2d, ", time);
    serial_stringout(buf);

    _delay_ms(5000);

    serial_stringout("getting delayed time... ");
    status_delayed = i2c_io(0xD0, &addr, 1, NULL, 0, rbuf, 1);
    if(status_delayed == 0) {
        serial_stringout("success delayed ");
    } else {
        char buf[40];
        snprintf(buf, 41, "unsuccessful delayed i2c transfer %2d ", status_delayed);
        serial_stringout(buf);
    }
    seconds = rbuf[0] & (0x0F);
    tens = (rbuf[0] & (0x70)) >> 4;
    time = seconds + 10*tens;
    char buf_delay[30];
    snprintf(buf_delay, 31, "Delayed Time: %2d\n", time);
    serial_stringout(buf_delay);
    
   // write test

    /*
   unsigned char times[7];
   //uint8_t times[7];
   // seconds 0; enable oscillator
    times[0] = 0x00;
   // 23
   times[1] = 0x23;
   //12 hour; 5PM
   times[2] = 0x65;
   //tuesday (day 3)
   times[3] = 0x03;
   // date is 28th
   times[4] = 0x28;
   // month is march
   times[5] = 0x03;
   // year is 23
   times[6] = 0x23;
   unsigned char regaddr = 0;

    unsigned char status = i2c_io(0xD0, &regaddr, 1, times, 7, NULL, 0);
    _delay_ms(2000);
    */
}

uint8_t reset_time() {
    uint8_t times[4];

    // set RTC time to 0
    times[0] = 0x00;
    times[1] = 0x00;
    // 24 hour mode
    times[2] = 0x00;
    times[3] = 0x01;
    uint8_t write_addr = 0;
    uint8_t status = i2c_io(0xD0, &write_addr, 1, times, 4, NULL, 0);

    // if(status != 0) {
    //     char buf[40];
    //     snprintf(buf, 41, "unsuccessful i2c transfer %2d ", status);
    //     serial_stringout(buf);
    // }
    return status;
}

void lcd_clear(){
    uint8_t wbuf[2];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x51;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 2, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful lcd clear %2d \n", status);
        serial_stringout(buf);
    }
    _delay_ms(3);
}

void lcd_rtc(uint8_t hour, uint8_t minutes){
    uint8_t wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x04;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful rtc print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    _delay_ms(1);

    char sbuf[20];
    snprintf(sbuf, 20, "Time:  %02d:%02d ", hour, minutes);
    status = i2c_io(0x50, NULL, 0, (uint8_t *)sbuf, 13, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful rtc print %2d \n", status);
        serial_stringout(buf);
    }
}

void lcd_alarm(uint8_t hour, uint8_t minutes){
    unsigned char wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x44;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    _delay_ms(1);
    
    char sbuf[20];
    snprintf(sbuf, 20, "Alarm: %02d:%02d ", hour, minutes);
    status = i2c_io(0x50, NULL, 0, (uint8_t *)sbuf, 12, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print %2d \n", status);
        serial_stringout(buf);
    }
}


uint8_t rtc_read() {
    uint8_t addr = 0;
    uint8_t rbuf[4];
    uint8_t status = i2c_io(0xD0, &addr, 1, NULL, 0, rbuf, 4);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d ", status);
        serial_stringout(buf);
        return status;
    }
    
    seconds_ones = rbuf[0] & (0x0F);
    seconds_tens = (rbuf[0] & (0x70)) >> 4;
    seconds = seconds_ones + 10*seconds_tens;
    minutes_ones = rbuf[1] & (0x0F);
    minutes_tens = (rbuf[1] & (0x70)) >> 4;
    minutes = minutes_ones + 10*minutes_tens;
    hours_ones = rbuf[2] & (0x0F);
    hours_tens = (rbuf[2] & (0x10)) >> 4;
    hours = hours_ones + 10*hours_tens;
    day = rbuf[3] & (0x07);
    day_word = days[day];

    char buf[40];

    snprintf(buf, 41, "Time: %02d:%02d:%02d ", hours, minutes, seconds);
    serial_stringout(buf);

    return status;
}

// TODO: Do we need the day and seconds for setting?
uint8_t rtc_set(uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds) {
    if((hours<0) || (hours>24) || (minutes>59) || (minutes<0) || (seconds>59) || (seconds<0) || (day>7) || (day<1)) {
        serial_stringout("invalid time");
        return -1;
    }
    uint8_t addr = 0;
    uint8_t wbuf[4];
    wbuf[0] = (seconds%10) | ((seconds/10)<<4);;
    wbuf[1] = (minutes%10) | ((minutes/10)<<4);
    wbuf[2] = (hours%10) | ((hours/10)<<4);
    wbuf[3] = day;
    
    uint8_t status = i2c_io(0xD0, &addr, 1, wbuf, 4, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d ", status);
        serial_stringout(buf);
    }
    return status;
}

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
    serial_init(ubrr); 
    
    state = SETCLOCK;

    _delay_ms(100);

    // Initialize ADC and Pulse Sensor 
    adc_init();
    pulse_sensor_init();
    interrupt_init();
    sleep_stage_init();

    // RTC init and write time //
    i2c_init(BDIV);
    _delay_ms(5000);
    //reset_time();
    rtc_set(2, 4, 19, 30);

    // PWM for buzzer
    timer0_init();


    DDRD |= (1<<PD6);   // haptic motor output; PD6 OC0A (pin12) or PD5 OC0B (pin11) 
    PORTD |= (1 << LEFT_BUTTON) | (1 << RIGHT_BUTTON) | (1 << TOGGLE_BUTTON);
    PORTB |= (1 << SELECT_BUTTON);
    lcd_clear();


    // TODO: initialize encoder
    encoder_init();

    // TODO: initialize the clock and minute times with 24 hour clock

    while (1) {
        if (state == SETCLOCK) {
            // Toggling
            if (PIND & (1 << TOGGLE_BUTTON)) {
                while (PIND & (1 << TOGGLE_BUTTON)) {}
                state = SETALARM; 
            }

            else if (PINB & (1 << SELECT_BUTTON)) {
                while (PINB & (1 << SELECT_BUTTON)) {}

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
                clock_index = (++clock_index) % 2;
            } 

            else if (PIND & (1 << LEFT_BUTTON)) {
                while (PIND & (1 << LEFT_BUTTON)) {}
                clock_index = (--clock_index) % 2;
            }

            if (changed) {
                lcd_rtc(hours, minutes);
            }
        } 
        
        else if(state == SETALARM) {
            // Toggling
            if (PIND & (1 << TOGGLE_BUTTON)) {
                while (PIND & (1 << TOGGLE_BUTTON)) {}
                state = SETCLOCK; 
            }

            else if (PINB & (1 << SELECT_BUTTON)) {
                while (PINB & (1 << SELECT_BUTTON)) {}

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
                alarm_index = (++alarm_index) % 2;
            } 

            else if (PIND & (1 << LEFT_BUTTON)) {
                while (PIND & (1 << LEFT_BUTTON)) {}
                alarm_index = (--alarm_index) % 2;
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