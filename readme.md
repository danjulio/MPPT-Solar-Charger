## MPPT Solar Charger

![MPPT Solar Charger](hardware/pictures/35_00082_02.png)


### Contents
This repository contains documentation and software for the MPPT Solar Charger board (design documented at [hackaday.io](https://hackaday.io/project/161351-solar-mppt-charger-for-247-iot-devices)).  

1. hardware - Board documentation and schematic
2. arduino - Arduino library and examples (can be compiled with wiringPi for Raspberry Pi too)
3. mppt_dashboard - Mac OS, Windows and Linux monitoring application that communicates with the charger via the mpptChgD daemon
4. mpptChgD - Linux Daemon compiled for Raspberry Pi that communicates with the charger via I2C

The MPPT Solar Charger is a combination solar battery charger and 5V power supply for IOT-class devices designed for 24/7 operation off of solar power. It manages charging a 12V AGM lead acid battery from common 36-cell 12V solar panels.  It provides 5V power output at up to 2A for systems that include sensors or communication radios.  Optimal charging is provided through a dynamic perturb-and-observe maximum power-point transfer converter (MPPT) and a 3-stage (BULK, ABSORPTION, FLOAT) charging algorithm.  A removable temperature sensor provides temperature compensation.  Operation is plug&play although additional information and configuration may be obtained through a digital interface.

* Optimized for commonly available batteries in the 7-18 AHr range and solar panels in the 10-35 Watt range
* Reverse Polarity protected solar panel input with press-to-open terminal block
* Fused battery input with press-to-open terminal block
* Maximum 2A at 5V output on USB Type A power output jack and solder header
* Automatic low-battery disconnect and auto-restart on recharged battery
* Temperature compensation sensor with internal sensor fallback
* Status LED indicating charge and power conditions, fault information
* I2C interface for detailed operation condition readout and configuration parameter access
* Configurable battery charge parameters
* Status signals for Night detection and pre-power-down alert
* Night-only operating mode (switch 5V output on only at night)
* Watchdog functionality to power-cycle connected device if it crashes

### Applications
* Remote control and sense applications* Solar powered web cam* Night-time “critter” cam* Solar powered LED night lighting controller

#### Bonus Application
The charger works well as a 12- and/or 5-V UPS when combined with a laptop power supply.  The laptop supply should be able to supply at least 3.5A at between 18.5 - 21V output (for example a Dell supply at 20V/3.5A) - a high enough voltage to initiate charging.  The charger will both charge the battery and supply the load current to the user's device and the battery will supply power if AC power fails.


### Questions?

Contact the designer - dan@danjuliodesigns.com
