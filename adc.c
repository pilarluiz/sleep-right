#include <avr/io.h>

#include "adc.h"

#define ADC_MUX_BITS 0b1111

#define ADC_PRSC  0b111         // Set the prescalar to divide by 128

void adc_init(void)
{
    // Initialize the ADC
    ADMUX |= (1 << REFS0);   // Set the REFS bits
    ADMUX |= (1 << ADLAR);      // Left adjust the output
    ADCSRA |= (ADC_PRSC << ADPS0);  // Set the prescalar bits
    ADCSRA |= (1 << ADEN);      // Enable the ADC
}

uint8_t adc_sample(uint8_t channel)
{
    ADMUX &= ~ADC_MUX_BITS;
    ADMUX |= ((channel & ADC_MUX_BITS) << MUX0); // Set the MUX bits
    ADCSRA |= (1 << ADSC);      // Start a conversion
    while (ADCSRA & (1 << ADSC)); // wait for conversion complete
    return ADCH;                // Get converted value
}
