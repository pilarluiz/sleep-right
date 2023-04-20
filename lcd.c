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

    _delay_ms(1);

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

void lcd_clear_row(uint8_t row) {       // Note: row is 0 indexed!!
    uint8_t wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    if(row==0) {
        wbuf[2] = 0x00;
    } else if(row==1) {
        wbuf[2] = 0x40;
    } else if(row==2) {
        wbuf[2] = 0x14;
    } else if(row==3) {
        wbuf[2] = 0x54;
    } else {
        serial_stringout("Invalid row");
    }
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful lcd clear %2d \n", status);
        serial_stringout(buf);
    }
    _delay_ms(1);

    char clearbuf[20];
    snprintf(clearbuf, 20, "                    ");
    status = i2c_io(0x50, NULL, 0, (uint8_t *)clearbuf, 17, NULL, 0);

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

void lcd_bpm(uint8_t beats){
    unsigned char wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x14;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful bpm print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    _delay_ms(1);
    
    char sbuf[20];
    snprintf(sbuf, 20, "BPM: %03d", beats);
    status = i2c_io(0x50, NULL, 0, (uint8_t *)sbuf, 8, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful bpm print %2d \n", status);
        serial_stringout(buf);
    }
}

void lcd_stage(uint8_t stage){
    unsigned char wbuf[3];
    wbuf[0] = 0xFE;
    wbuf[1] = 0x45;
    wbuf[2] = 0x54;
    uint8_t status = i2c_io(0x50, NULL, 0, wbuf, 3, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful stage print (moveto) %2d \n", status);
        serial_stringout(buf);
    }
    
    _delay_ms(1);
    
    char sbuf[20];
    if (stage == 0) {
        snprintf(sbuf, 20, "Stage: LIGHT");
    } else if (stage == 1) {
        snprintf(sbuf, 20, "Stage: DEEP ");
    } else if (stage == 2) {
        snprintf(sbuf, 20, "Stage: REM  ");
    } else if (stage == 3) {
        snprintf(sbuf, 20, "Stage: AWAKE");
    }

    status = i2c_io(0x50, NULL, 0, (uint8_t *)sbuf, 12, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful stage print %2d \n", status);
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

    _delay_ms(1);
}


void lcd_wakeup(char* buf, int len) {
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

    _delay_ms(1);

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

    status = i2c_io(0x50, NULL, 0, "              ", 14, NULL, 0);
    if(status != 0) {
        char buf[40];
        snprintf(buf, 41, "unsuccessful alarm print %2d \n", status);
        serial_stringout(buf);
    }

    _delay_ms(1);

}