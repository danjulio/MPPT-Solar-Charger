#include <Adafruit_CharacterOLED.h>
#include <mpptChg.h>

// --- ARDUINO PIN DEFINITIONS
uint8_t RSPin = 2;
uint8_t RWPin = 3;
uint8_t ENPin = 4;
uint8_t D4Pin = 5;
uint8_t D5Pin = 6;
uint8_t D6Pin = 7;
uint8_t D7Pin = 8;

// --- Hardware objects
Adafruit_CharacterOLED lcd(OLED_V1, RSPin, RWPin, ENPin, D4Pin, D5Pin, D6Pin, D7Pin);
mpptChg chg;

uint16_t st;
int16_t vs, is, vb, ib;

void setup()
{
  chg.begin();
  lcd.begin(16, 2);
  delay(100);        // Let OLED finish initializing
}

void loop()
{
  lcd.clear();
  
  // Read and display values
  if (chg.getStatusValue(SYS_STATUS, &st)) {
    // Assume remaining reads will work
    (void) chg.getIndexedValue(VAL_VS, &vs);
    (void) chg.getIndexedValue(VAL_IS, &is);
    (void) chg.getIndexedValue(VAL_VB, &vb);
    (void) chg.getIndexedValue(VAL_IB, &ib);

    // Line 1
    lcd.setCursor(0, 0);
    printStatus(st);
    lcd.setCursor(8, 0);
    printMilliAsFloat(computePowerMw(vs, is));
    lcd.print(F("W"));

    // Line 2
    lcd.setCursor(0, 1);
    printMilliAsFloat(vb);
    lcd.print(F("V"));
    lcd.setCursor(8, 1);
    printRightMilli(ib);
    lcd.print(F("mA"));
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("read failed"));
  }
  
  delay(1000);
}

// Prints in form II.FF
void printMilliAsFloat(uint16_t val)
{
  uint16_t intPortion;
  uint16_t fracPortion;

  // Scale by 10 before we format
  val = val / 10;
  intPortion = val / 100;
  fracPortion = val % 100;

  // Integer portion
  if (intPortion < 10) {
    // leading space
    lcd.print(F(" "));
  }
  lcd.print(intPortion);
  lcd.print(F("."));

  // Fractional portion
  if (fracPortion < 10) {
    // leading zero
    lcd.print(F("0"));
  }
  lcd.print(fracPortion);
}

// Prints right-justified integer
void printRightMilli(uint16_t val)
{
  if (val < 10) {
    lcd.print(F("    "));
  } else if (val < 100) {
    lcd.print(F("   "));
  } else if (val < 1000) {
    lcd.print(F("  "));
  } else if (val < 10000) {
    lcd.print(F(" "));
  }
  lcd.print(val);
}

void printStatus(uint16_t s)
{
  switch (s & MPPT_CHG_STATUS_CHG_ST_MASK) {
    case MPPT_CHG_ST_NIGHT:
      lcd.print(F("NIGHT"));
      break;
    case MPPT_CHG_ST_IDLE:
      lcd.print(F("IDLE"));
      break;
    case MPPT_CHG_ST_VSRCV:
      lcd.print(F("VSRCV"));
      break;
    case MPPT_CHG_ST_SCAN:
      lcd.print(F("SCAN"));
      break;
    case MPPT_CHG_ST_BULK:
      lcd.print(F("BULK"));
      break;
    case MPPT_CHG_ST_ABSORB:
      lcd.print(F("ABSORB"));
      break;
    case MPPT_CHG_ST_FLOAT:
      lcd.print(F("FLOAT"));
      break;
    default:
      lcd.print(F("???"));
  }
}

uint16_t computePowerMw(uint16_t mV, uint16_t mA)
{
  uint32_t p;

  p = (uint32_t) mV * (uint32_t) mA;
  p = p / (uint32_t) 1000;
  return ((uint16_t) p);
}

