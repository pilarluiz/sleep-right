void pulse_sensor_init(void);
int get_pulse_reading(void);
void read_next_sample(void);
void process_latest_sample(void);

// Pulse detection output variables.
// Volatile because our pulse detection code could be called from an Interrupt
volatile int BPM;                // int that holds raw Analog in 0. updated every call to readSensor()
volatile int Signal;             // holds the latest incoming raw data (0..1023)
volatile int IBI;                // int that holds the time interval (ms) between beats! Must be seeded!
volatile int Pulse;              // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile int QS;                 // The start of beat has been detected and not read by the Sketch.
volatile int FadeLevel;          // brightness of the FadePin, in scaled PWM units. See FADE_SCALE
volatile int threshSetting;      // used to seed and reset the thresh variable
volatile int amp;                         // used to hold amplitude of pulse waveform, seeded (sample value)
volatile unsigned long lastBeatTime;      // used to find IBI. Time (sampleCounter) of the previous detected beat start.

// Variables internal to the pulse detection algorithm.
// Not volatile because we use them only internally to the pulse detection.
unsigned long sampleIntervalMs;  // expected time between calls to readSensor(), in milliseconds.
int rate[10];                    // array to hold last ten IBI values (ms)
unsigned long sampleCounter;     // used to determine pulse timing. Milliseconds since we started.
int N;                           // used to monitor duration between beats
int P;                           // used to find peak in pulse wave, seeded (sample value)
int T;                           // used to find trough in pulse wave, seeded (sample value)
int thresh;                      // used to find instant moment of heart beat, seeded (sample value)
int firstBeat;                   // used to seed rate array so we startup with reasonable BPM
int secondBeat;                  // used to seed rate array so we startup with reasonable BPM