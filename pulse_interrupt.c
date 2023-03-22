#include "pulse_interrupt.h"
#include "pulse_sensor.h"
#include <avr/interrupt.h>

void interrupt_init() {
    TCCR1A = 0x00;            // Disable PWM and go into CTC mode
    TCCR1C = 0x00;            // don't force compare
    // #if F_CPU == 16000000L    // if using 16MHz crystal
    //     TCCR1B = 0x0C;          // prescaler 256
    //     OCR1A = 0x007C;         // count to 124 for 2mS interrupt
    // #elif F_CPU == 8000000L   // if using 8MHz crystal
    TCCR1B = 0x0B;          // prescaler = 64
    OCR1A = 0x00F9;         // count to 249 for 2mS interrupt
    // #endif
    TIMSK1 = 0x02;            // Enable OCR1A match interrupt	DISABLE BY SETTING TO 0x00
    ENABLE_PULSE_SENSOR_INTERRUPTS;
    // result = true;
}

void enable_interrupt() {
    DISABLE_PULSE_SENSOR_INTERRUPTS;
    TIMSK1 = 0x02;            // Enable OCR1A match interrupt
    ENABLE_PULSE_SENSOR_INTERRUPTS;
}

void disable_interrupt() {
    DISABLE_PULSE_SENSOR_INTERRUPTS;
    TIMSK1 = 0x00;            // Disable OCR1A match interrupt
    ENABLE_PULSE_SENSOR_INTERRUPTS;
}

ISR(TIMER1_COMPA_vect)
{
    DISABLE_PULSE_SENSOR_INTERRUPTS;         // disable interrupts while we do this

    read_next_sample();
    process_latest_sample();

    ENABLE_PULSE_SENSOR_INTERRUPTS;          // enable interrupts when you're done
}