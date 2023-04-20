#include <stdio.h>
#include <stddef.h>
#include <util/delay.h>

#include "rtc.h"
#include "i2c.h"
#include "serial.h"

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

    uint8_t old_hours = hours; 
    uint8_t old_minutes = minutes; 
    
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

    if ((old_hours != hours) || (old_minutes != minutes)) {
        time_changed = 1; 
    }

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