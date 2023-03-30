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
#define BDIV (FOSC / 100000 - 16) / 2 + 1

// RTC global variables
uint8_t seconds_ones;
uint8_t seconds_tens;
uint8_t seconds;
uint8_t minutes_ones;
uint8_t minutes_tens;
uint8_t hours_ones;
uint8_t hours_tens;
uint8_t day;

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
        //serial_stringout("success ");
    } else {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d ", status);
        //serial_stringout(buf);
    }
    seconds = rbuf[0] & (0x0F);
    tens = (rbuf[0] & (0x70)) >> 4;
    time = seconds + 10*tens;
    char buf[30];
    snprintf(buf, 31, "Time: %2d, ", time);
    //serial_stringout(buf);

    _delay_ms(5000);

    //serial_stringout("getting delayed time... ");
    status_delayed = i2c_io(0xD0, &addr, 1, NULL, 0, rbuf, 1);
    if(status_delayed == 0) {
        //serial_stringout("success delayed ");
    } else {
        char buf[40];
        snprintf(buf, 41, "unsuccessful delayed i2c transfer %2d ", status_delayed);
        //serial_stringout(buf);
    }
    seconds = rbuf[0] & (0x0F);
    tens = (rbuf[0] & (0x70)) >> 4;
    time = seconds + 10*tens;
    char buf_delay[30];
    snprintf(buf_delay, 31, "Delayed Time: %2d\n", time);
    //serial_stringout(buf_delay);
    
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

uint8_t init_rtc() {
    uint8_t times[4];
    // set RTC time to 0
    times[0] = 0x00;
    times[1] = 0x00;
    // 24 hour mode
    times[2] = 0x00;
    times[3] = 0x01;
    uint8_t write_addr = 0;
    uint8_t status = i2c_io(0xD0, &write_addr, 1, times, 4, NULL, 0);

    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful i2c transfer %2d ", status);
        serial_stringout(buf);
    }
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
    

    return status;
}


int main(void)
{
    // Voltage Level Test
    unsigned short ubrr = ( ( FOSC / 16 ) / 9600) - 1; 
    //serial_init(ubrr); 

    _delay_ms(100);

    // Initialize ADC and Pulse Sensor 
    //adc_init();
    //pulse_sensor_init();
    //interrupt_init();

    // RTC init and write time //
    i2c_init(BDIV);
    _delay_ms(3000);
    init_rtc();

    while (1) {

        debug_rtc();
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






