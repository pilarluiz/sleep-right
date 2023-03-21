# Sleep Right

## Opening the communication terminal with ATMega328

# On MacOS

1. With USB serial adaptor not plugged in, find all names of files with this format: `ls /dev/tty*`
2. Plug in USB serial adator and rerun command to find file associated with device. The device should have a name like `/dev/cu.usbserial-A901C419`.
3. From terminal, enter command `screen name-of-device 9600`
   - [pilar's laptop] `screen /dev/tty.usbserial-A603A9DU 9600` for bottom right port
