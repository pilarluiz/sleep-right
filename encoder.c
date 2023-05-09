/*
* EE459, Spring 2023 Capstone Project
* Team 7 -- SleepRight
* * Baran Cinbis, Evan Hashemi, Pilar Luiz
*/

#include "encoder.h"
#include <stdio.h>
#include <util/delay.h>

#include "lcd.h"
#include "serial.h"
#include "i2c.h"

void encoder_init() {
    // Pullup resistors
    PORTD |= (1<<PD2);
    PORTD |= (1<<PD4);

    // Enable interruts
    PCICR |= (1 << PCIE2);  // Enable PDINT on Port D
	PCMSK2 |= (1 << PCINT18 | 1 << PCINT20); // Interrupt on PD2, PD4 for encoder

    // Determine the initial state
    uint8_t bits, a, b;
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

void check_encoder_baran() {
    uint8_t bits, a, b;

    bits = PIND;		// Read the two encoder input at the same time
    a = bits & (1 << PD2);
    b = bits & (1 << PD4);

    //uint8_t increment=0;

    if (old_state == 0) {
        if (a) {
            new_state = 1;
            increment = 1;
            
        }
        else if (b) {
            new_state = 2;
            increment=0;
        }
    }
    else if (old_state == 1) {
        if (!a) {
            new_state = 0;
            increment=0;
        }
        else if (b) {
            new_state = 3;
            increment=1;
        }
    }
    else if (old_state == 2) {
        if (a) {
            new_state = 3;
            increment=0;
        }
        else if (!b) {
            new_state = 0;
            increment=1;
        }
    }
    else {   // old_state = 3
        if (!a) {
            new_state = 2;
            increment=1;
        }
        else if (!b) {
            new_state = 1;
            increment=0;
        }
    }

    // if(increment) {
    //     if(!alarm_set) {            // setting clock time       
    //         if(!clock_index) {   
    //             if(hours<23) {
    //                 hours++;
    //             } else {
    //                 hours = 0;
    //             }
    //         } else {
    //             if(minutes<59) {
    //                 minutes++;
    //             } else {
    //                 minutes=0;
    //             } 
    //         }
    //     } else {                    // setting alarm time
    //         if(!alarm_index) {
    //             if(wakeup_hours<23) {
    //                 wakeup_hours++;
    //             } else {
    //                 wakeup_hours = 0;
    //             }
    //         } else {
    //             if(wakeup_minutes<59) {
    //                 wakeup_minutes++;
    //             } else {
    //                 wakeup_minutes=0;
    //             } 
    //         }
    //     }
    // } else {
    //     if(!alarm_set) {            // setting clock time       
    //         if(!clock_index) {   
    //             if(hours>0) {
    //                 hours--;
    //             } else {
    //                 hours = 23;
    //             }
    //         } else {
    //             if(minutes>0) {
    //                 minutes--;
    //             } else {
    //                 minutes=59;
    //             } 
    //         }
    //     } else {                    // setting alarm time
    //         if(!alarm_index) {
    //             if(wakeup_hours>0) {
    //                 wakeup_hours--;
    //             } else {
    //                 wakeup_hours = 23;
    //             }
    //         } else {
    //             if(wakeup_minutes>0) {
    //                 wakeup_minutes--;
    //             } else {
    //                 wakeup_minutes=59;
    //             } 
    //         }
    //     }
    // }

    if (new_state != old_state) {
        changed = 1;
        old_state = new_state;
    }
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
        uint8_t wbuf[3];
        // wbuf[0] = 0xFE;
        // wbuf[1] = 0x45;
        // wbuf[2] = 0x14;
        // char sbuf[20];
        // snprintf(sbuf, 20, "SETCLOCK");
        // uint8_t status = i2c_io(0x50, NULL, 0, (uint8_t *)sbuf, 8, NULL, 0);

        if (!clock_index) {
            time = &hours;
            h = 1; 
        } else {
            time = &minutes;
            h = 0; 
        }
    } else if (state == 0b01000) {      // State is SETALARM
        // uint8_t wbuf[3];
        // wbuf[0] = 0xFE;
        // wbuf[1] = 0x45;
        // wbuf[2] = 0x14;
        // char sbuf[20];
        // snprintf(sbuf, 20, "SETALARM");
        // uint8_t status = i2c_io(0x50, NULL, 0, (uint8_t *)sbuf, 8, NULL, 0);

        if (!alarm_index) {
            time = &wakeup_hours;
            h = 1; 
        } else {
            time = &wakeup_minutes;
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
    check_encoder_baran();
}
