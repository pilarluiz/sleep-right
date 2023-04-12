#include "encoder.h"

void encoder_init() {
    uint8_t bits, a, b;

    // Enable interruts
    PCICR |= (1 << PCIE2);  // Enable PDINT on Port D
	PCMSK2 |= (1 << PD2 | 1 << PD4); // Interrupt on PD2, PD4 for encoder

    // Determine the initial state
    bits = PIND;
    a = bits & (1 << PD2);
    b = bits & (1 << PD4);

    if (!b && !a)
	    old_state = 0;
    else if (!b && a)
	    old_state = 1;
    else if (b && !a)
	    old_state = 2;
    else
	    old_state = 3;

    new_state = old_state;
}

void check_encoder()
{
    uint8_t bits, a, b;
    volatile uint8_t* time; 
    uint8_t h;

    bits = PIND;		// Read the two encoder input at the same time
    a = bits & (1 << PD2);
    b = bits & (1 << PD4);

    // Determine what variable to modify
    if (state == 0b10000) {     // State is SETCLOCK
        if (!clock_index) {
            time = &hours;
            h = 1; 
        } else {
            time = &minutes;
            h = 0; 
        }
    } else if (state == 0b01000) {      // State is SETALARM
        if (!alarm_index) {
            time = &alarm_hours;
            h = 1; 
        } else {
            time = &alarm_minutes;
            h = 0; 
        }
    } else {
        // Ignore encoder in all other states
        return; 
    }

    if (old_state == 0) {
        if (a) {
            new_state = 1;
            if (h) {
                *time = ((*time) + 1) % 24; 
            } else {
                *time = ((*time) + 1) % 60; 
            }
        }
        else if (b) {
            new_state = 2;
            if (h) {
                *time = ((*time) - 1) % 24; 
            } else {
                *time = ((*time) - 1) % 60; 
            }
        }
    }
    else if (old_state == 1) {
        if (!a) {
            new_state = 0;
            if (h) {
                *time = ((*time) - 1) % 24; 
            } else {
                *time = ((*time) - 1) % 60; 
            }
        }
        else if (b) {
            new_state = 3;
            if (h) {
                *time = ((*time) + 1) % 24; 
            } else {
                *time = ((*time) + 1) % 60; 
            }
        }
    }
    else if (old_state == 2) {
        if (a) {
            new_state = 3;
            if (h) {
                *time = ((*time) - 1) % 24; 
            } else {
                *time = ((*time) - 1) % 60; 
            }
        }
        else if (!b) {
            new_state = 0;
            if (h) {
                *time = ((*time) + 1) % 24; 
            } else {
                *time = ((*time) + 1) % 60; 
            }
        }
    }
    else {   // old_state = 3
        if (!a) {
            new_state = 2;
            if (h) {
                *time = ((*time) + 1) % 24; 
            } else {
                *time = ((*time) + 1) % 60; 
            }
        }
        else if (!b) {
            new_state = 1;
            if (h) {
                *time = ((*time) - 1) % 24; 
            } else {
                *time = ((*time) - 1) % 60; 
            }
        }
    }

    if (new_state != old_state) {
        changed = 1;
        old_state = new_state;
    }
}

ISR(PCINT2_vect)    // Interrupts on PORTD pins
{
    check_encoder();
}
