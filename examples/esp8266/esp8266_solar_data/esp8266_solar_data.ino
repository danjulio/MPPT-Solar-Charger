/*
 * MPPT Solar Charger + esp8266 driving Adafruit IO dashboard
 *
 * This program demonstrates displaying charger information on an Adafruit IO dashboard using
 * an esp8266 module.
 * 
 * Connections:
 *    MPPT Solar Charger GND => esp8266 GND
 *    MPPT Solar Charger 5V  => esp8266 +5V VIN (small module requires 3.3V external regulator)
 *    MPPT Solar Charger SDA => esp8266 D4
 *    MPPT Solar Charger SCL => esp8266 D2
 *
 * Required Libraries
 *    MPPT Solar Charger mpptChg Arduino library - https://github.com/danjulio/MPPT-Solar-Charger/tree/master/arduino
 *    Adafruit MPTT library - https://github.com/adafruit/Adafruit_MQTT_Library
 * 
 * You must have installed support for the esp8266 in the Arduino IDE.  In addition, you must have an account
 * at Adafruit (https://io.adafruit.com/).  Information about creating the dashboard is in the github respository
 * (https://github.com/danjulio/MPPT-Solar-Charger/tree/master/examples/motioneyeos).
 * 
 * This demo was tested on the Sparkfun ESP8266 WiFi Shield.  The built-in LED on pin 5 is used to indicate successful
 * connection to the local WiFi Network.  It may need to be changed for different esp8266 boards.
 * 
 * Distribured as-is; No warranty is given.
 * 
 */
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESP8266WiFi.h>
#include <mpptChg.h>


// ========================================================================
// User parameters - configure this section for your WiFi and Adafruit info
//
// WiFi parameters - replace the strings with your network's credentials
#define WLAN_SSID       "WLAN_SSID"
#define WLAN_PASS       "WLAN_PASS"

// Adafruit IO -  replace the USERNAME and KEY with your account info
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "AIO_USERNAME"  // Obtained from account info on io.adafruit.com
#define AIO_KEY         "AIO_KEY"       // Obtained from account info on io.adafruit.com



// ========================================================================
// Program Constants
//

// I2C Bus pins
#define SDA_PIN 4
#define SCL_PIN 2

// Status LED pin
#define LED_PIN 5

// Number of feeds
#define NUM_FEEDS 5

// Charger status strings
#define I2C_ERR_INDEX 8

const char* status_msg[] = {
  "NIGHT",
  "IDLE",
  "VSRCV",
  "SCAN",
  "BULK",
  "ABSORPTION",
  "FLOAT",
  "ILLEGAL",
  "I2C FAIL"
};


// ========================================================================
// Variables
//

// Solar charger object
mpptChg chg;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup feeds for temperature & humidity
Adafruit_MQTT_Publish chg_status = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solar_status");  // String
Adafruit_MQTT_Publish chg_power = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solar_power");    // Watts - float
Adafruit_MQTT_Publish chg_vb = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solar_vb");          // Volts - float
Adafruit_MQTT_Publish chg_ib = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solar_ib");          // mA - integer
Adafruit_MQTT_Publish chg_et = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solar_temp");        // C - float

// Charger values
uint16_t reg_status;
int16_t reg_vs;
int16_t reg_is;
int16_t reg_vb;
int16_t reg_ib;
int16_t reg_et;
float chg_watts;
float chg_batt_volts;
float chg_temp;

// Current status string index
int status_index = 0;


// ========================================================================
// Arduino environment entry points
//

void setup() {
  // Configure the LED for diagnostics
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED is active low
  
  // Use the hardware serial for diagnostics
  Serial.begin(115200);

  // Initialize the connection to the charger
  chg.begin(SDA_PIN, SCL_PIN);

  // Attempt to connect to our local WiFi (hangs until connection)
  wifi_connect();

  // Attempt to connect to Adafruit IO (hangs until connection)
  io_connect();

  // Indicate success
  digitalWrite(LED_PIN, LOW);
}


void loop() {
  int i;
  
  // Ping the server a few times to make sure we remain connected
  if(!mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      io_connect();
  }

  // Update values from the charger
  chg_update();

  // Push one data value every 2 seconds (so not to overrun the free version of IO)
  for (i=0; i<NUM_FEEDS; i++) {
    publish_data(i);
    delay(2000);
  }
}


// ========================================================================
// Subroutines
//

void wifi_connect() {
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
  }
  Serial.println(F("WiFi Connected"));
  Serial.print(F("  IP address: "));
  Serial.println(WiFi.localIP());
}


void io_connect() {
  Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {
    digitalWrite(LED_PIN, LOW);
    
    switch (ret) {
      case 1: Serial.println(F("  Wrong protocol")); break;
      case 2: Serial.println(F("  ID rejected")); break;
      case 3: Serial.println(F("  Server unavail")); break;
      case 4: Serial.println(F("  Bad user/pass")); break;
      case 5: Serial.println(F("  Not authed")); break;
      case 6: Serial.println(F("  Failed to subscribe")); break;
      default: Serial.println(F("  Connection failed")); break;
    }

    if(ret >= 0) {
      mqtt.disconnect();
    }
    
    delay(1000);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
  }

  Serial.println(F("Adafruit IO Connected!"));
}


void chg_update() {
  // Assume if we can read one value successfully, we can read them all
  if (chg.getStatusValue(SYS_STATUS, &reg_status)) {
    (void) chg.getIndexedValue(VAL_VS, &reg_vs);
    (void) chg.getIndexedValue(VAL_IS, &reg_is);
    (void) chg.getIndexedValue(VAL_VB, &reg_vb);
    (void) chg.getIndexedValue(VAL_IB, &reg_ib);
    (void) chg.getIndexedValue(VAL_EXT_TEMP, &reg_et);

    // Compute some values
    chg_watts = ((float) reg_vs * (float) reg_is) / 1000000.0;   // W = mA * mA / 1E6
    chg_batt_volts = (float) reg_vb / 1000.0;
    chg_temp = (float) reg_et / 10.0;

    // Determine our charge state
    status_index = reg_status & MPPT_CHG_STATUS_CHG_ST_MASK;  // 3 low bits are charger state
  } else {
    status_index = I2C_ERR_INDEX;
    Serial.println(F("getStatusValue failed"));
  }
}


void publish_data(int i) {
  switch (i) {
    case 0:
      if (!chg_status.publish((char*) status_msg[status_index])) {
        Serial.println(F("Failed to publish status"));
      } else {
        Serial.print(F("Published status "));
        Serial.println((char*) status_msg[status_index]);
      }
      break;

    case 1:
      if (!chg_power.publish(chg_watts)) {
        Serial.println(F("Failed to publish power"));
      } else {
        Serial.print(F("Published power = "));
        Serial.println(chg_watts);
      }
      break;

    case 2:
      if (!chg_vb.publish(chg_batt_volts)) {
        Serial.println(F("Failed to publish vb"));
      } else {
        Serial.print(F("Published vb = "));
        Serial.println(chg_batt_volts);
      }
      break;

    case 3:
      if (!chg_ib.publish(reg_ib)) {
        Serial.println(F("Failed to publish ib"));
      } else {
        Serial.print(F("Published ib = "));
        Serial.println(reg_ib);
      }
      break;

    case 4:
      if (!chg_et.publish(chg_temp)) {
        Serial.println(F("Failed to publish temp"));
      } else {
        Serial.print(F("Published temp = "));
        Serial.println(chg_temp);
      }
      break;
  }
}

