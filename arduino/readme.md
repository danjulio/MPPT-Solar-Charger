## makerPower™ Arduino Library

![OLED Solar Monitor](pictures/lib_oled_test.png)

This directory contains a simple Arduino library providing access the makerPower via I2C and a couple of simple example sketches.  The library can also be compiled on a Raspberry Pi (requires [wiringPi](http://wiringpi.com/download-and-install/)).

### Sample Sketches

1. lib\_ser\_test - Reads several values from the board and outputs their values via the serial port.
2. lib\_ole\_test - Displays the charge state, solar power, battery voltage and current draw on a 2x16 character OLED display.  This sketch may be easily ported to a LCD-based 2x16 character display.  My prototype uses a Sparkfun LCD driver board that is actually an Arduino.

![OLED backside](pictures/oled_back.png)

### Linux Example
The linux directory contains a simple example using the library on a Raspberry Pi.  Put the source in the same directory as the library files and use the command line in the 'm' file to compile.  It assumes wiringPi has been installed. I just ```chmod +x m``` and compile using ```./m``` in the same directory as the source files.

Run the the demo program ```./test_i2c```.

Note that the I2C interface on all Pi versions has a bug that causes it to fail when the I2C slave stretches the clock (the makerPower stretches the clock slightly).  A work-around is to reduce the I2C clock rate to 50 kHz that can be done by adding the following line to the ```/boot/config.txt``` file.

  ```
  dtparam=i2c_arm_baudrate=50000
  ```
  