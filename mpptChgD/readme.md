## makerPower™ MPPT Solar Charger Daemon

Building and running the daemon requires [wiringPi](http://wiringpi.com/download-and-install/) to be installed.  The 'm' file contains the command line to compile it.  I just ```chmod +x m``` and compile using ```./m``` in the same directory as the source files.

### Functionality
The ```mpptChgD``` daemon provides the following functionality.

1. Simple character-based command/response access for applications through a pseudo-tty called ```/dev/mpptChg```
2. Optional functionality enabled by an external text configuration file
  * Automatic system shutdown on low-battery Alert
  * TCP Port interface supporting the same commands as the pseudo-tty
  * Logging of charger values to an external file at a user-specified rate
  * Configuration of charger parameters for non-default operation
  * Watchdog management

### Installation

1. Install and build [wiringPi](http://wiringpi.com/download-and-install/).  Note that wiringPi may be already installed on some distributions.
2. Build the executable (or use the binary provided here.  You may have to chmod +x, etc).
3. Copy the executable ```mpptChgD``` to a suitable location like ```/usr/local/bin```.
4. Copy the configuration file somewhere that makes sense to you.  I put it in ```/home/pi/mpptChgDconfig.txt```.  Edit it to configure operation and logging as you desire.  The template file here should have adequate internal documentation to allow you to configure it as necessary.
5. Configure your system to start the daemon automatically, for example in the ```/etc/rc.local``` file.

    ```
    # Start the charger daemon
    /usr/local/bin/mpptChgD -d -f /home/pi/mpptChgDconfig.txt &
    ```
6. For Raspberry Pi: The I2C interface on all Pi versions has a bug that causes it to fail when the I2C slave stretches the clock (the MPPT Solar Charger stretches the clock slightly).  A work-around is to reduce the I2C clock rate to 50 kHz that can be done by adding the following line to the ```/boot/config.txt``` file.

    ```
    dtparam=i2c_arm_baudrate=50000
    ```

#### Special note about libwiringPi dependency
The executable requires the libwiringPi dynamic library to be installed.  This is done automatically if you build wiringPi on the target machine.  However you may find that you only want to install the pre-built daemon.  This means you have to also install the wiringPi library.  I include the version that I built the included library with here.  Installation requires the following:

1. Copy the library file ```libwiringPi.so.X.Y``` to the system in ```/usr/local/lib```.
2. Create ```libwiringPi.so``` by linking the library: ```ln -s libwiringPi.so.X.Y libwiringPi.so```

If you cannot put the library in ```/usr/local/lib``` (for example you are installing on a pre-built distribution like ```motioneyeos```) then you can use the ```LD_LIBRARY_PATH``` environment variable to tell the system where to find it.  For example, if you can only use a specfic directory (shown as ```<DIR>``` below) then you can handle the installation as follows.

1. Copy the pre-built daemon and wiringPi library to ```<DIR>```.
2. Create the linked file in ```<DIR```: ```ln -s libwiringPi.so.X.Y libwiringPi.so```
3. Set the environment variable ```LD_LIBRARY_PATH``` to ```<DIR>``` in the same context you start the daemon.

	```
	export LD_LIBRARY_PATH=<DIR>
	<DIR>/mpptChgD -d -f <DIR>/mpptChgDconfig.txt &
	```


### Operation
The program may be run from the command line without the ```-d``` option for testing or when executed by a user-application.  The program will not run as a daemon without the option.

The program will log startup and error information to the system log. The ```-x <N>``` command line option controls verbosity.  By default <N> is 0.  Value of 1-3 give more verbose output.

A user application may communicate with the daemon in the same way it would communicate with an open serial port.  The application opens ```/dev/mpptChg``` for read/write access and then sends simple read or write commands to get or set SMBus register values.  For example the ```screen``` program can be used to communicate with the daemon (although there is no echo of the user's typed commands).

  ```
  screen /dev/mpptChg
  ```

Append the register name string to "READ=\<RegName\>" and terminate with a carriage return or linefeed to read a SMBus register.  For example:

  ```
  READ=STATUS
  READ=VS
  ```

The daemon will return a string in the form "\<RegName\>=\<Value\>\<LF\>".

  ```
  STATUS=136
  VS=17535
  ```
A complete list of SMBus register names can be found in the example configuration file.

Writing a SMBus register takes the form "\<RegName\>=\<Value\>\<LF\>".  Writing a RO register has no effect.

  ```
  PWROFF=11750
  ```

Access through the TCP port is identical.

### Configuration File

The configuration file, specified with the ```-f <file>``` command line option, controls operation of the following functions.

1. Enable/Disable remote TCP access, specify the maximum number of supported simultaneous connections and the TCP port to bind to.  Note that there may be a security risk having an open port on the computer.
2. Enable/Disable logging, specify the log interval (in seconds between samples) and the items to be logged.
3. Change the default charger parameters for default Bulk charge threshold, Float charge threshold, low-battery power off and power on thresholds.
4. Enable a watchdog function.  The daemon will enable the watchdog function on the charger, reset WDPWROFF to 10 seconds, and then periodically update the WDCNT SMBus register to prevent the charger from power-cycling the computer.  The daemon catches SIGINT, SIGHUP and SIGTERM and will attempt to disable the watchdog before terminating after receiving any of these signals.  However if the daemon may killed (SIGKILL or SIGSTOP) so that the watchdog function remains running in which case the computer will be power-cycled when it expires.  User code can  write to the psuedo-tty to disable the watchdog function immediately after killing the daemon in this case (```echo "WCNT=0" > /dev/mpptChg```).  If you are worried about a specific process failing and want to use the watchdog function to detect that then either the process needs to control the watchdog function or another script/program that is monitoring the process must control the watchdog function.

### Log File

The log file is a simple text file that is easy to parse.  It is read directly by the ```mppt_dashboard	``` program.  An example of the first few lines of a typical log file are shown below.

```
LOGGING: STATUS VS IS VB IB IC IT ET VM TH 
1523977824: 132 17588 560 12381 79 644 315 311 17555 14520 
1523977892: 132 17159 579 12453 79 646 326 319 17205 14493 
1523977953: 132 16953 583 12483 80 639 329 321 17048 14487 
1523978066: 132 17141 568 12526 79 628 336 319 17098 14493 
1523978126: 132 16493 542 12526 80 568 336 312 16448 14514 
1523978187: 132 16971 557 12538 79 608 333 302 16948 14544 
```

The first line contains a list of SMBus register values being logged.  Subsequent lines contain a timestamp (Unix epoch time in seconds) followed by a colon, followed by the register values in decimal form.  Voltage values are in mV, current values in mA, temperature are in Celcius * 10.  All values are separated by spaces and each line terminated with a newline character.
