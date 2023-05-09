/*
* EE459, Spring 2023 Capstone Project
* Team 7 -- SleepRight
* * Baran Cinbis, Evan Hashemi, Pilar Luiz
*/

#include <avr/io.h>
#include "adc.h"

#define ADC_MUX_BITS 0b1111

#define ADC_PRSC  0b111         // Set the prescalar to divide by 128

void adc_init(void)
{
    // Initialize the ADC
    ADMUX |= (1 << REFS0);   // Set the REFS bits
    //ADMUX |= (1 << ADLAR);      // Keep this if only 8 bit (left adjust output); took out because we want 10 bit
    ADCSRA |= (ADC_PRSC << ADPS0);  // Set the prescalar bits
    ADCSRA |= (1 << ADEN);      // Enable the ADC
}

uint16_t adc_sample(uint8_t channel)
{
    ADMUX &= ~ADC_MUX_BITS;
    ADMUX |= ((channel & ADC_MUX_BITS) << MUX0); // Set the MUX bits
    ADCSRA |= (1 << ADSC);      // Start a conversion
    while (ADCSRA & (1 << ADSC)); // wait for conversion complete
    uint8_t lo = ADCL;
    uint8_t hi = ADCH;
    uint16_t x = hi;
    x = x << 8;
    x = x | lo;
    // 10-bit ADC sample
    return (x);                // Get converted value
}
