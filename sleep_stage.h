#include "pulse_sensor.h"
#include <avr/io.h>

uint8_t sleep_stage(void);
void sleep_stage_init(void);

#define LIGHT_SLEEP 0
#define DEEP_SLEEP 1
#define REM_SLEEP 2
#define AWAKE 3

uint8_t last_stage;
extern int average_bpm[30];