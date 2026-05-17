#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (true) delay(500);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power — setting compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("Printing RTC every second:");
}

void loop() {
  DateTime n = rtc.now();
  Serial.printf("%04u-%02u-%02u %02u:%02u:%02u\n",
                n.year(), n.month(), n.day(),
                n.hour(), n.minute(), n.second());
  delay(1000);
}