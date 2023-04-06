#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "adc.h"
#include "pulse_sensor.h"
#include "serial.h"
#include "pulse_interrupt.h"
#include "i2c.h"

// Determine real time clock baud rate
#define FOSC 7372800    // CPU clock freq
//#define BDIV (FOSC / 100000 - 16) / 2 + 1
#define BDIV (FOSC / 50000 - 16) / 2 + 1

// RTC global variables
uint8_t seconds_ones;
uint8_t seconds_tens;
uint8_t seconds;
uint8_t minutes_ones;
uint8_t minutes_tens;
uint8_t minutes;
uint8_t hours_ones;
uint8_t hours_tens;
uint8_t hours;
char* day;
char* days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// User set wakeup time
uint8_t wakeup_minutes_ones;
uint8_t wakeup_minutes_tens;
uint8_t wakeup_minutes;
uint8_t wakeup_hours_ones;
uint8_t wakeup_hours_tens;
uint8_t wakeup_hours;

// Timer variables
volatile int timer2_interrupt_count = 0;

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
    day = days[rbuf[3] & (0x07)];

    char buf[40];

    snprintf(buf, 41, "Time: %02d:%02d:%02d ", hours, minutes, seconds);
    serial_stringout(buf);

    return status;
}

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

uint8_t abs(uint8_t value) {
    if(value<0) {
        return -1*value;
    } else {
        return value;
    }
}

// TODO: Sleep mode functions
void check_if_wakeup() {
    if(!rtc_read()) {       // no rtc error
        // at wakeup time? Then must wake up
        if( (wakeup_hours == hours && (abs(wakeup_minutes-minutes) <= 5) ) || 
        ((abs(wakeup_hours-hours) == 1) && (abs(wakeup_minutes-minutes) >= 55)) || 
        ((abs(wakeup_hours-hours) == 24) && (abs(wakeup_minutes-minutes) >= 55))) {
            serial_stringout("at wakeup time \n");
            vibrate_motor();
            // turn timer off
            TCCR2B &= ~ ((1<<CS00) | (1<<CS01) | (1<<CS02));
            // turn sleep mode off
            SMCR &= ~(1<<SE);
            return 1;

        } else if() {           // within 1 sleep cycle (1.5 hour) if wakeup?
            // check if there is potentially an early wakeup
            sleep_algorithm_here();
        } else {
            return 0;
        }
    } else {
        // Error reading RTC
        return -1;
    }
}



void enter_idle_sleep_mode() {
    // Sleep Enable pin set to 1
    SMCR |= (1<<SE);
    // Idle sleep mode -> SM2:0 = 000
    SMCR &= ~((1<<SM2) | (1<<SM1) | (1<<SM0));
}

// TODO
void timer_2_init() {
    
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
        check_if_wakeup();
        
    }
    // Go back to sleep
    SMCR |= (1<<SE);
}

void sleep_algorithm_here() {
    // at wake up time
}

void vibrate_motor() {

}

void lcd_test() {

    uint8_t write_addr = 0xFE;
    uint8_t status = i2c_io(0x28, 0xFE, 1, 0x41, 1, NULL, 0);
    if(status) {
        serial_stringout("LCD error");
    }
    // _delay_ms(2000);
    // status = i2c_io(0x28, 0xFE, 1, 0x42, 1, NULL, 0);
    // if(status) {
    //     serial_stringout("LCD error");
    // }

}


int main(void)
{
    // Voltage Level Test
    unsigned short ubrr = ( ( FOSC / 16 ) / 9600) - 1; 
    serial_init(ubrr); 

    _delay_ms(100);

    // Initialize ADC and Pulse Sensor 
    adc_init();
    pulse_sensor_init();
    interrupt_init();

    // RTC init and write time //
    i2c_init(BDIV);
    _delay_ms(5000);
    //reset_time();
    rtc_set(2, 4, 19, 30);



    while (1) {
        lcd_test();

        //debug_rtc();
        //rtc_read();
        _delay_ms(2000);
        /*
        if (saw_start_of_beat()) {
            char buf[30];
            snprintf(buf, 31, "BPM: '%2d'\n", BPM);
            serial_stringout(buf); 
        }  

        _delay_ms(20);
        */
    }

    return 0;   /* never reached */
}






