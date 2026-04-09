## esp32io : serial communication for ESP32 ##

esp32io consists of two main parts; a plotter (utilizing raylib) and a serial connection. The serial interface is read in a separate thread (in the read_ESP32_data() function) and have a simple proof-reading mechanism built-in; each data packet (consisting of a 20 bytes struct) contains a signature. The data stream is aligned to the signature if needed. Often this occurs once during the first bytes read, since the ESP32 continuously send data.

The data stream is collected into a data array which is the sent to the plotter. The sampling rate (adjustable), and data samples are averaged before plotting for sampling rates < 1. 

The data_t struct in types.h has to be adjusted to match the data sent by the ESP32 before building. A simple configuration (config.txt) is used to setup the plotter correctly. It defines the number of subplots (matching the recieved data_t structure) and dimensions of the raylib window.

Baudrates that work (don't know why not all of them) with the current setup:
    - 115200
    - 460800
    - 576000
    - 921600

The list is non-exhaustive, it's just the ones I tested. In the testing setup, at a baudrate of 576000, a signal frequency of ~3 KHz is achieved using a simple ESP32 loop with no delays.

Also included in the repo is serial_write, a debugging app, which writes data serially to one of the /dev/pts ttys.
