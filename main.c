#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "adc.h"
#include "pulse_sensor.h"
#include "serial.h"
#include "pulse_interrupt.h"

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


int main(void)
{
    // Voltage Level Test
    unsigned short ubrr = ( ( 7372800 / 16 ) / 9600) - 1; 
    serial_init(ubrr); 

    _delay_ms(100);

    // Initialize ADC and Pulse Sensor 
    adc_init();
    pulse_sensor_init();
    interrupt_init();

    while (1) {
        if (saw_start_of_beat()) {
            char buf[30];
            snprintf(buf, 31, "BPM: '%2d'\n", BPM);
            serial_stringout(buf); 
        }  

        _delay_ms(20);
    }

    return 0;   /* never reached */
}

