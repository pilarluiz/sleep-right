#include "sleep_stage.h"

// TODO: Create tests for sleep stages

// fucntion

// for (i = 0; i < 30; ) 
//      BPM = 30 // DEEP
// for (i = 0 ; i < 30)
//      BPM = 40 //
// for (i = 0; i < 30) 
//      BPM = 50


void sleep_stage_init()
{
    last_stage = AWAKE;
    stage_change = 0; 
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

    // Sleep stage has changed
    if (last_stage != current_stage) {
        if (last_stage == LIGHT_SLEEP) {
            stage_change = 1; 
        }

        last_stage = current_stage; 
    } else {
        stage_change = 0; 
    }

    return current_stage;
}

uint8_t stage_changed()
{
    return stage_change; 
}