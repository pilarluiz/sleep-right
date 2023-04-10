#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "adc.h"
#include "pulse_sensor.h"
#include "serial.h"
#include "pulse_interrupt.h"
#include "i2c.h"
#include "sleep_stage.h"

// Determine real time clock baud rate
#define FOSC 7372800    // CPU clock freq
//#define BDIV (FOSC / 100000 - 16) / 2 + 1
#define BDIV (FOSC / 50000 - 16) / 2 + 1

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

void lcd_clear(){
    uint8_t wbuf[2];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x51;
    status = i2c_io(0x50, NULL, 0, wbuf, 2, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful lcd clear %2d \n", status);
        serial_stringout(buf);
    }
    _delay_ms(3);
}

void lcd_rtc(uint8_t hour, uint8_t minutes){
    char wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x04;
    status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful rtc print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    char sbuf[20];
    snprintf(sbuf, 20, "Time: %02d, %02d ", hour, minutes);
    status = i2c_io(0x50, NULL, 0, sbuf, 13, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful rtc print %2d \n", status);
        serial_stringout(buf);
    }
}

void lcd_alarm(uint8_t hour, uint8_t minutes){
    char wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x44;
    status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    char sbuf[20];
    snprintf(sbuf, 20, "Alarm: %02d, %02d ", hour, minutes);
    status = i2c_io(0x50, NULL, 0, sbuf, 13, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print %2d \n", status);
        serial_stringout(buf);
    }
}

void lcd_test(){
    uint8_t addr = 0;
    uint8_t wbuf[4];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x4B;
    //wbuf[1] = 0x00;
    uint8_t status = i2c_io(0x50, &addr, 1, wbuf, 2, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d \n", status);
        serial_stringout(buf);
        return status;
    }
    // _delay_ms(1000);

    // Clear screen 
    wbuf[1] = 0x51;
    status = i2c_io(0x50, &addr, 1, wbuf, 2, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d \n", status);
        serial_stringout(buf);
        return status;
    }

    _delay_ms(1000);

    // wbuf[0] = 0x00;
    // wbuf[1] = 0xA0;
    wbuf[0] = LETTER_E;
    wbuf[1] = 0x56;
    wbuf[2] = LETTER_A;
    wbuf[3] = LETTER_N;
    // wbuf[2] = 0; 
    status = i2c_io(0x50, NULL, 0, wbuf, 4, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d \n", status);
        serial_stringout(buf);
        return status;
    }

    _delay_ms(1000);
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

// TODO: Sleep mode functions
// [pilar] do we want this to be an interrupt? not sure how we want to do RTC & wakeup time
int check_if_wakeup() {
    if(!rtc_read()) {
        // no rtc error
    }
    
    
}

void enter_idle_mode() {
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

    }
    // Go back to sleep
    SMCR |= (1<<SE);
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
    void sleep_stage_init();

    // RTC init and write time //
    i2c_init(BDIV);
    _delay_ms(5000);
    //reset_time();
    rtc_set(2, 4, 19, 30);

    while (1) {

        //debug_rtc();
        rtc_read();
        _delay_ms(1000);
        lcd_test();
        
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






