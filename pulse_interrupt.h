/*
* EE459, Spring 2023 Capstone Project
* Team 7 -- SleepRight
* * Baran Cinbis, Evan Hashemi, Pilar Luiz
*/

void interrupt_init(void);
void enable_interrupt(void);
void disable_interrupt(void);

#define DISABLE_PULSE_SENSOR_INTERRUPTS cli()
#define ENABLE_PULSE_SENSOR_INTERRUPTS sei()