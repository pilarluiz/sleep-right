/*
* EE459, Spring 2023 Capstone Project
* Team 7 -- SleepRight
* * Baran Cinbis, Evan Hashemi, Pilar Luiz
*/

#include "pulse_sensor.h"
#include <avr/io.h>

uint8_t sleep_stage(void);
void sleep_stage_init(void);
uint8_t stage_changed(void);

#define LIGHT_SLEEP 0
#define DEEP_SLEEP 1
#define REM_SLEEP 2
#define AWAKE 3

uint8_t last_stage;
extern int average_bpm[30];
uint8_t stage_change; 
