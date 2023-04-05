#include <stdio.h>
#include <avr/interrupt.h>

#include "adc.h"
#include "serial.h"
#include "pulse_sensor.h"
#include "pulse_interrupt.h"

#define PULSE_PIN 3
#define DEBUG 1

/*
    Initialize variables
*/
void pulse_sensor_init(void) {
    for(int i = 0; i < 10; ++i) {
        rate[i] = 0;
    }

    QS = 0; 
    BPM = 0; 
    IBI = 750;                  // 750ms per beat = 80 Beats Per Minute (BPM)
    Pulse = 0; 
    sampleCounter = 0; 
    lastBeatTime = 0; 
    P = 512;                    // peak at 1/2 the input range of 0..1023
    T = 512;                    // trough at 1/2 the input range.
    threshSetting = 750;
    thresh = threshSetting;     // reset the thresh variable with user defined THRESHOLD
    amp = 100;                  // beat amplitude 1/10 of input range.
    firstBeat = 1;              // looking for the first beat
    secondBeat = 0;             // not yet looking for the second beat in a row
    FadeLevel = 0;              // LED is dark.
    sampleIntervalMs = (2 * 1000L) / 1000;

    // Initialize bpm_history array with invalid readings & initialize index 
    for(int i = 0; i < 30; ++i) {
        bpm_history[i] = -1;
    }
    bpm_history_idx = 0; 
}

/*
    Function to get adc sample, process it, and return the BPM. 
*/
int get_pulse_reading(void) {
    read_next_sample();
    process_latest_sample();
    return BPM;
}

/*
    Take ADC sample on pulse sensor
*/
void read_next_sample(void) {
    // We assume assigning to an int is atomic.
    Signal = adc_sample(PULSE_PIN);

    // if(DEBUG) {
    //     char buf[30];
    //     snprintf(buf, 31, "ADC Signal Read: '%2d'\n", Signal);
    //     serial_stringout(buf);    
    // } 
}

/*
    Process sample for noise and other factors
*/
void process_latest_sample(void) {
    // Serial.println(threshSetting);
    // Serial.print('\t');
    // Serial.println(thresh);
    sampleCounter += sampleIntervalMs;              // keep track of the time in mS with this variable
    N = sampleCounter - lastBeatTime;               // monitor the time since the last beat to avoid noise

    //  find the peak and trough of the pulse wave
    if (Signal < thresh && N > (IBI / 5) * 3) {     // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T) {                           // T is the trough
            T = Signal;                             // keep track of lowest point in pulse wave
        }
    }

    if (Signal > thresh && Signal > P) {            // thresh condition helps avoid noise
        P = Signal;                                 // P is the peak
    }                                               // keep track of highest point in pulse wave

    //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
    // signal surges up in value every time there is a pulse
    // if(DEBUG) {
    //     char buf[100];
    //     snprintf(buf, 101, "Looking for heart beat...\n\tN=%d\n\tSignal=%d\n\tPulse=%d\n\tIBI=%d", N, Signal, Pulse, IBI);
    //     serial_stringout(buf);
    // }

    if (N > 250) {                             // avoid high frequency noise
        // if(DEBUG) {
        //     char buf[100];
        //     snprintf(buf, 101, "Looking for heart beat...\n\tN=%d\n\tSignal=%d\n\tPulse=%d\n\tIBI math=%d", N, Signal, Pulse, (IBI / 5) * 3);
        //     serial_stringout(buf);
        // }

        if ( (Signal > thresh) && (Pulse == 0) && (N > (IBI / 5) * 3) ) {
            // if(DEBUG) {
            //     serial_stringout("IN INNER LOOP");
            // }
            Pulse = 1;                             // set the Pulse flag when we think there is a pulse
            IBI = sampleCounter - lastBeatTime;    // measure time between beats in mS
            lastBeatTime = sampleCounter;          // keep track of time for next pulse

            if (secondBeat) {                       // if this is the second beat, if secondBeat == TRUE
                secondBeat = 0;                     // clear secondBeat flag
                for (int i = 0; i <= 9; i++) {      // seed the running total to get a realisitic BPM at startup
                    rate[i] = IBI;
                }
            }

            if (firstBeat) {                        // if it's the first time we found a beat, if firstBeat == TRUE
                firstBeat = 0;                      // clear firstBeat flag
                secondBeat = 1;                     // set the second beat flag
                // IBI value is unreliable so discard it
                return;
            }


            // keep a running total of the last 10 IBI values
            int runningTotal = 0;                   // clear the runningTotal variable

            for (int i = 0; i <= 8; i++) {          // shift data in the rate array
                rate[i] = rate[i + 1];              // and drop the oldest IBI value
                runningTotal += rate[i];            // add up the 9 oldest IBI values
            } 

            rate[9] = IBI;                          // add the latest IBI to the rate array
            runningTotal += rate[9];                // add the latest IBI to runningTotal
            runningTotal /= 10;                     // average the last 10 IBI values
            BPM = 60000 / runningTotal;             // how many beats can fit into a minute? that's BPM!
            QS = 1;                                 // set Quantified Self flag (we detected a beat)
        }
    }

    if (Signal < thresh && Pulse == 1) {  // when the values are going down, the beat is over
        Pulse = 0;                                  // reset the Pulse flag so we can do it again
        amp = P - T;                                // get amplitude of the pulse wave
        thresh = amp / 2 + T;                       // set thresh at 50% of the amplitude
        P = thresh;                                 // reset these for next time
        T = thresh;
    }

    if (N > 2500) {                                 // if 2.5 seconds go by without a beat
        thresh = threshSetting;                     // set thresh default
        P = 512;                                    // set P default
        T = 512;                                    // set T default
        lastBeatTime = sampleCounter;               // bring the lastBeatTime up to date
        firstBeat = 1;                              // set these to avoid noise
        secondBeat = 0;                             // when we get the heartbeat back
        QS = 0;
        BPM = 0;
        IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
        Pulse = 0;
        amp = 100;                  // beat amplitude 1/10 of input range.
    }
}

int saw_start_of_beat() {
    // Disable interrupts to avoid a race with the ISR.
    DISABLE_PULSE_SENSOR_INTERRUPTS;
    int started = QS;
    QS = 0;
    ENABLE_PULSE_SENSOR_INTERRUPTS;

    return started;
}