#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "adc.h"
#include "pulse_sensor.h"
#include "serial.h"

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

    while (1) {
        int bpm = get_pulse_reading();
        char buf[30];

        snprintf(buf, 31, "BPM: '%2d'\n", bpm);
        serial_stringout(buf);      

        _delay_ms(1500);
    }

    return 0;   /* never reached */
}

