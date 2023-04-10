# Sleep Right

## Opening the communication terminal with ATMega328

1. With USB serial adaptor not plugged in, find all names of files with this format: `ls /dev/tty*`
2. Plug in USB serial adator and rerun command to find file associated with device. The device should have a name like `/dev/cu.usbserial-A901C419`.
3. From terminal, enter command `screen name-of-device 9600`
   - [example] `screen /dev/tty.usbserial-A603A9DU 9600`

## Pulse Sensor

This project uses the Adafruit 1093 Pulse Sensor to get an analog reading from the sensor. We used the World Famous Electronics' [PulseSensorPlayground](https://github.com/WorldFamousElectronics/PulseSensorPlayground) to process the reading and covert it to beats per minute (BPM).

## Sleep Stage Algorithm

SleepRight uses BPM to determine a user's sleep stage.

<strong>Disclaimer:</strong> For a wearable to accurately determine a user's sleep stage, additional features are necessary, such as an acceleromter to detect movement, heart rate variability detection to determine REM versus NREM sleep, and personalized learning to optimize for a specific user.

In our project, we only use a pulse sensor and exclude additional "necessary" components. For simplification purposes, we assume the user is an adult with an average heart rate of 40-50 BPM during sleep. Our project detects three stages of sleep:

- <strong>Light Sleep</strong> (45 - 50 BPM): In light sleep, heart rate begins to slow. This stage of sleep accounts for most of one's time asleep.
- <strong>Deep Sleep</strong> (< 45 BPM) : Deep sleep is the stage where heart rate reaches the lowest levels. It occurs in the beginning of sleep, typically at night.
- <strong>REM</strong> (50 - 60 BPM): Rapid eye movement is the sleep stage most similar to being awake. In this stage, a peron's muscles fully react and they experience vivid dreaming. Heart rate may speed up to a heart rate similar to when you are awake. This stage of sleep occurs near the end of sleep, typically in the morning.
