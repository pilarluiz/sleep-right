#include <stdio.h>
#include <util/delay.h>

#include "lcd.h"
#include "serial.h"
#include "i2c.h"

void lcd_splash_screen() {
    lcd_clear();

    // Move to top row
    uint8_t wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x00;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    _delay_ms(1);

    char topbuf[20];
    snprintf(topbuf, 20, "EE459: SleepRight");
    status = i2c_io(0x50, NULL, 0, (uint8_t *)topbuf, 17, NULL, 0);

    // Move to bottom row 
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x14;
    status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print (moveto) %2d \n", status);
        serial_stringout(buf);
    }

    _delay_ms(1);

    char *bottombuf = "Baran, Evan, Pilar";
    status = i2c_io(0x50, NULL, 0, bottombuf, 18, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful rtc print %2d \n", status);
        serial_stringout(buf);
    }

    _delay_ms(3000);
    lcd_clear();
}

void lcd_clear() {
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

void lcd_debug_print(char* buf, int len) {
    unsigned char wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x54;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    _delay_ms(1);

    status = i2c_io(0x50, NULL, 0, (uint8_t*)buf, len, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print %2d \n", status);
        serial_stringout(buf);
    }
}