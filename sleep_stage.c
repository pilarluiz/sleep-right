#include "sleep_stage.h"

// TODO: If stage switched, check if current time is optimal wakeup time

// TODO: Create tests for sleep stages

void sleep_stage_init()
{
    last_stage = AWAKE;
}

uint8_t sleep_stage()
{
    // Get average BPM of past 30 BPMs
    int average_bpm = 0; 
    int valid_readings = 0; 
    for(int i = 0; i < 30; ++i) {
        if (bpm_history[i] > 0) {
            average_bpm += bpm_history[i];
            ++valid_readings;
        }
    }
    average_bpm /= valid_readings;

    uint8_t current_stage; 

    if (average_bpm < 45) {
        current_stage = DEEP_SLEEP;
    } else if (average_bpm < 50) {
        current_stage = LIGHT_SLEEP;
    } else if (average_bpm < 60) {
        current_stage = REM_SLEEP;
    } else {
        current_stage = AWAKE;
    }

    if (last_stage != current_stage) {
        // TODO: stage has changed; check if optimal time to wakeup
    }

    return current_stage;
}