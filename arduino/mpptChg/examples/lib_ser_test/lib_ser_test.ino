#include <mpptChg.h>

mpptChg chg;

uint16_t st;
int16_t vs, is, vb, ib;

void setup() {
  chg.begin();
  Serial.begin(57600);
}

void loop() {
  if (chg.getStatusValue(SYS_STATUS, &st)) {
    (void) chg.getIndexedValue(VAL_VS, &vs);
    (void) chg.getIndexedValue(VAL_IS, &is);
    (void) chg.getIndexedValue(VAL_VB, &vb);
    (void) chg.getIndexedValue(VAL_IB, &ib);

    Serial.print(st, HEX);
    Serial.print(" ");
    Serial.print(vs);
    Serial.print(" ");
    Serial.print(is);
    Serial.print(" ");
    Serial.print(vb);
    Serial.print(" ");
    Serial.println(ib);
  } else {
    Serial.println(F("getStatusValue failed"));
  }

  delay(1000);
}
