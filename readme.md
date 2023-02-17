## makerPower™ MPPT Solar Charger

![MPPT Solar Charger](hardware/pictures/35_00082_02.png)

### LiFePO4 support (added 2/2023)
Firmware version 2.0 adds support for 12V (4-cell) LiFePO4 batteries.  Several charging parameters, shown below, are changed when a LiFePO4 battery type is selected.

1. Float/Bulk initial charge state threshold set to 13.2V
2. Bulk Voltage set to 14.4V
3. Power On charge voltage set to 13.6V
4. Temperature Compensation disabled
5. Charge temperature range between 0°C and 50°C

The charger will default to the lead acid battery type.  Add a jumper between the test pads shown below to configure the charger for a LiFePO4 battery.

![LiFePO4 configuration](hardware/pictures/LiFePO4_jumper.png)

### Contents
This repository contains documentation and software for the makerPower MPPT Solar Charger board (design documented at [hackaday.io](https://hackaday.io/project/161351-solar-mppt-charger-for-247-iot-devices)).  It can be found in my [tindie store](https://www.tindie.com/products/globoy/mppt-solar-charger-for-intelligent-devices/).

1. firmware - Charger C source code
2. hardware - Board documentation, schematic and connection diagrams for different uses
3. arduino - Arduino library and examples (can be compiled with wiringPi for Raspberry Pi too)
4. mppt_dashboard - Mac OS, Windows and Linux monitoring application that communicates with the charger via the mpptChgD daemon
5. mpptChgD - Linux Daemon compiled for Raspberry Pi that communicates with the charger via I2C

The makerPower is a combination solar battery charger and 5V power supply for IOT-class devices designed for 24/7 operation off of solar power. It manages charging a 12V AGM lead acid or LiFePO4 battery from common 36-cell 12V solar panels.  It provides 5V power output at up to 2A for systems that include sensors or communication radios.  Optimal charging is provided through a dynamic perturb-and-observe maximum power-point transfer converter (MPPT) and a 3-stage (BULK, ABSORPTION, FLOAT) charging algorithm.  A removable temperature sensor provides temperature compensation.  Operation is plug&play although additional information and configuration may be obtained through a digital interface.

* Optimized for commonly available batteries in the 7-18 Ah range and solar panels in the 10-35 Watt range
* Reverse Polarity protected solar panel input with press-to-open terminal block
* Fused battery input with press-to-open terminal block
* Maximum 2A at 5V output on USB Type A power output jack and solder header
* Automatic low-battery disconnect and auto-restart on recharged battery
* Temperature compensation sensor with internal sensor fallback (lead acid batteries)
* Disable charging when battery is too cold or too hot
* Status LED indicating charge and power conditions, fault information
* I2C interface for detailed operation condition readout and configuration parameter access
* Configurable battery charge parameters
* Status signals for Night detection and pre-power-down alert
* Night-only operating mode (switch 5V output on only at night)
* Watchdog functionality to power-cycle connected device if it crashes or for timed power-off control

### Applications
* Remote control and sense applications* Solar powered web or timelapse camera* Night-time “critter cam"* Solar powered LED night lighting controller

#### Bonus Application
The charger works well as a 12- and/or 5-V UPS when combined with a laptop power supply.  The laptop supply should be able to supply at least 3.5A at between 18.5 - 21V output (for example a Dell supply at 20V/3.5A) - a high enough voltage to initiate charging.  The charger will both charge the battery and supply the load current to the user's device and the battery will supply power if AC power fails.

### Compatible Solar Panels and Batteries
The makerPower is designed to use standard 25- or 35-Watt 12V solar panels with 7-Ah to 18-Ah 12V AGM type lead acid or LiFePO4 batteries. It has a maximum charge capacity of about 35-38 watts. A detailed sizing method is described in the user manual but it is possible to use smaller or larger panels and batteries depending on the application.

Typically a 25-Watt panel is paired with a 7-Ah battery for small systems (Arduino-type up to Raspberry Pi Zero type). A 35-Watt panel is paired with 9-Ah to 18-Ah batteries for larger systems. Larger batteries provide longer run-time during poor (lower light) charging conditions. A larger panel can provide more charge current during poor charging conditions.

Solar panels should be a 36-cell type with a typical maximum power-point of around 18V and maximum open-circuit voltage of 23V (typically around 21-22V). Available panels and batteries I have tested with are shown below.

* [25 Watt Panel](https://www.amazon.com/gp/product/B014UND3LA)
* [35 Watt Panel](https://www.amazon.com/gp/product/B01G1II6LY)
* [9 Ah Lead Acid Battery](https://www.amazon.com/Power-Sonic-PS-1290-Rechargeable-Battery-Terminals/dp/B002L6R130)
* [10 Ah LiFePO4 Battery](https://www.amazon.com/ExpertPower-Lithium-Rechargeable-2500-7000-lifetime/dp/B07X3Y3LS5)
* [18 Ah Lead Acid Battery](https://www.amazon.com/ExpertPower-EXP12180-Rechargeable-Battery-Bolts/dp/B00A82A3RK)

### Enclosures

I have used the [Carlon E989N](https://www.homedepot.com/p/Carlon-8-in-x-4-in-PVC-Junction-Box-E989N-CAR/100404099) enclosure found at a Home Depot home improvement store to hold the battery, charger and single-board computer.  It is a good size providing room for a 7-Ah to 10-Ah battery as well as room for heat dissipation from both the charger and computer.  Note that the charger can dissipate upwards of 5W when running at full capacity.

Other possible enclosures include the following.

* Hammond Manufacturing [RP1465/RP1465C](https://www.hammfg.com/electronics/small-case/plastic/rp)
* Bud Industries [PIP-11774/PIP-11774-C](https://www.budind.com/view/NEMA+Boxes/NEMA+4X+-+PIP)

### Questions?

Contact the designer - dan@danjuliodesigns.com
