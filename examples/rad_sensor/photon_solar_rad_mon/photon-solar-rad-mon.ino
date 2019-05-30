/*
 * Remote radiation monitor with display through mobile Blynk app.
 * Monitor CPM and uSev/H from the DIYGeiger GK-B5 and solar information
 * from the makerPower MPPT charger remotely with data uploaded to the
 * cloud using a Particle Photon and displayed on a mobile device using
 * Blynk (blink.io).
 *
 * Photon Connections
 *   MPPT Solar Charger GND => Photon GND
 *   MPPT Solar Charger 5V  => Photon VIN
 *   MPPT Solar Charger SDA => Photon D0
 *   MPPT Solar Charger SCL => Photon D1
 *   GK-B5 FTDI Header GND  => Photon GND
 *   GK-B5 FTDI Header 5V   => Photon VIN
 *   GK-B5 FTDI TX Out      => Photon RX In
 *
 * Required Libraries
 *   Blynk (this program tested using version 0.5.4)
 *
 * An particle.io account is required and the Photon should be claimed by 
 * the account.  This program tested using Device OS version 1.1.0.  It
 * maybe loaded and compiled in either the Web or Desktop IDE.
 *
 * A free Blynk account is required and the Blynk app must be loaded
 * on a mobile device.  Please see the following GitHub repository
 * for instructions on configuring the app for display of data from
 * this program:
 *
 *   https://github.com/danjulio/MPPT-Solar-Charger/tree/master/examples/rad_sensor
 *
 * The GK-B5 outputs a <CR/LF> terminated string containing CPM, Dose and its VCC
 * level every minute by default.  For example:
 *   73,1.6591,5.17
 * It also outputs a text string describing the values when it first starts:
 *   CPM,uSv/h,Vcc
 *   
 * Distributed as-is; No warranty is given.
 */

#include <blynk.h>
#include "mpptChg.h"



// ========================================================================
// User parameters - configure this section by including your authorization
// token emailed to you when you configure the Blynk app inside the quotes
//

char auth[] = "YOUR_AUTH_STRING_HERE";



// ========================================================================
// Program Constants
//

#define BLYNK_PRINT Serial  // Set serial output for debug prints
//#define BLYNK_DEBUG       // Uncomment this to see detailed prints

#define LED_PIN D7

// Number of radiation monitor values to accumulate
#define RAD_MON_NUM_VALS 3

#define RAD_MON_VAL_CPM  0
#define RAD_MON_VAL_DOSE 1
#define RAD_MON_VAL_VCC  2

// Charger status strings
#define I2C_ERR_INDEX 8

const char* status_msg[] = {
  "NIGHT",
  "IDLE",
  "VSRCV",
  "SCAN",
  "BULK",
  "ABS",
  "FLOAT",
  "ILLEGAL",
  "I2C FAIL"
};

// Objects
BlynkTimer timer;

WidgetLCD lcd1(V3);
WidgetLCD lcd2(V4);

mpptChg chg;



// ========================================================================
// Variables
//
bool ledVal = false;

// Radiation Monitor State
float rad_mon_vals[RAD_MON_NUM_VALS];
int rad_mon_index;
bool rad_mon_build_frac;
float rad_mon_frac_divisor;

// MPPT Charger State
uint16_t reg_status;
int16_t  reg_vs;
int16_t  reg_is;
int16_t  reg_vb;
int16_t  reg_ib;
int16_t  reg_et;

int status_index;



// ========================================================================
// Subroutines
//

// Toggles the LED to let the local user know we're running
void ledEvent() {
    digitalWrite(LED_PIN, ledVal ? LOW : HIGH);
    ledVal = !ledVal;
}


// Automatically called to handle data from the radiation monitor
void serialEvent1() {
    char c;
    int n;
     
    while (Serial1.available()) {
        c = Serial1.read();
        
        if (c == 0x0A) {
            // Linefeed terminates record
            updateRadMonLcd();
            initRadMonVals();
        } else if (c == ',') {
            // Comma separates values
            rad_mon_index++;
            rad_mon_build_frac = false;
        } else if (c == '.') {
            // Period separates integer from fractional portion of value
            rad_mon_build_frac = true;
            rad_mon_frac_divisor = 10.0;  // First fractional character worth 1/10
        } else if ((n = charIsNum(c)) != -1) {
            // Accumulate part of a value
            if (rad_mon_index < RAD_MON_NUM_VALS) {
                if (!rad_mon_build_frac) {
                    // Integer portion
                    rad_mon_vals[rad_mon_index] = (rad_mon_vals[rad_mon_index] * 10) + n;
                } else {
                    // Fractional portion
                    rad_mon_vals[rad_mon_index] += (float) n / rad_mon_frac_divisor;
                    rad_mon_frac_divisor *= 10;
                }
            }
        }
        // else ignore all other characters
    }
}


// Called to update charger related values
void chargerEvent() {
    updateChargerData();
    updateChargerLcd();
    updateChargerGraph();
}


void updateRadMonLcd() {
    lcd1.clear();
    lcd1.print(0, 0, String::format("CPM %d", (int) rad_mon_vals[RAD_MON_VAL_CPM]));
    lcd1.print(10, 0, String::format("%0.2fV", rad_mon_vals[RAD_MON_VAL_VCC]));
    lcd1.print(0, 1, String::format("uSv/h %0.2f", rad_mon_vals[RAD_MON_VAL_DOSE]));
}


void updateChargerData() {
    // Assume if we can read one value successfully, we can read them all
    if (chg.getStatusValue(SYS_STATUS, &reg_status)) {
        (void) chg.getIndexedValue(VAL_VS, &reg_vs);
        (void) chg.getIndexedValue(VAL_IS, &reg_is);
        (void) chg.getIndexedValue(VAL_VB, &reg_vb);
        (void) chg.getIndexedValue(VAL_IB, &reg_ib);
        (void) chg.getIndexedValue(VAL_EXT_TEMP, &reg_et);
        status_index = reg_status & MPPT_CHG_STATUS_CHG_ST_MASK;  // 3 low bits are charger state
    } else {
        reg_vs = 0;
        reg_is = 0;
        reg_vb = 0;
        reg_ib = 0;
        reg_et = 0;
        status_index = I2C_ERR_INDEX;
        Serial.println("getStatusValue failed");
    }
}


void updateChargerLcd() {
    float chg_batt_volts;
    float chg_temp;
    
    // Compute some values
    chg_batt_volts = (float) reg_vb / 1000.0;
    chg_temp = (float) reg_et / 10.0;
    
    // Update the LCD display widget
    lcd2.clear();
    lcd2.print(0, 0, status_msg[status_index]);
    lcd2.print(10, 0, String::format("%2.1fC", chg_temp));
    lcd2.print(0, 1, String::format("%2.2fV", chg_batt_volts));
    lcd2.print(10, 1, String::format("%4dmA", reg_ib));
}


void updateChargerGraph() {
    float chg_solar_volts;
    float chg_solar_amps;
    float chg_solar_watts;
    
    // Compute some values
    chg_solar_volts = (float) reg_vs / 1000.0;
    chg_solar_amps = (float) reg_is / 1000.0;
    chg_solar_watts = ((float) reg_vs * (float) reg_is) / 1000000.0;   // W = mA * mA / 1E6
    
    // Update the graph
    Blynk.virtualWrite(V0, chg_solar_volts);
    Blynk.virtualWrite(V1, chg_solar_amps);
    Blynk.virtualWrite(V2, chg_solar_watts);
}


void initRadMonVals() {
    for (rad_mon_index=0; rad_mon_index<RAD_MON_NUM_VALS; rad_mon_index++) {
        rad_mon_vals[rad_mon_index] = 0;
    }
    rad_mon_index = 0;
    rad_mon_build_frac = false;
    rad_mon_frac_divisor = 10.0;
}


// Returns 0-9 if char is a number, -1 if it is not
int charIsNum(char c) {
    if ((c >= '0') && (c <= '9')) {
        return (c - '0');
    } else {
        return -1;
    }
}


// ========================================================================
// Particle environment entry points
//

void setup() {
    // Local diagnostic interface
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    delay(5000); // Allow time for user to open a serial connection after programming
    Serial.println("Hello");
    
    // Hardware serial interface for radiation monitor
    Serial1.begin(9600);
    
    // Initialize the charger interface
    chg.begin();
    Serial.println("Charger begin");
    
    // Connect to Blynk
    Blynk.begin(auth);
    Serial.println("Blynk auth");
    
    // Initialize rad monitor values
    initRadMonVals();

    // Setup timer callbacks for monitoring the charger and updating related displays
    timer.setInterval(500L, ledEvent);
    timer.setInterval(2000L, chargerEvent);
}


void loop() {
    Blynk.run();
    timer.run();
}