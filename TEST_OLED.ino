#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// Display Configurations
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin Configuration Matrix
const int ALARM_BUZZER  = 25;
const int ALARM_LIGHTS  = 26;
const int BUTTON_SNOOZE = 14;
const int BUTTON_STOP   = 12;

// Real-Time Clock Module Instance
RTC_DS3231 rtc;

// Volatile Interrupt State Variables
volatile bool snoozePressed = false;
volatile bool stopPressed   = false;

// System Clock State Machine Variables
bool isAlarmActive = false;
bool lightStateToggle = false;
unsigned long snoozeEndTime = 0;
int snoozeCount = 0;
const int MAX_SNOOZES = 3;
bool rtcBatteryDead = false;

// Mock Schedule Variables (Simulating local memory entry)
int scheduledHour = 15;   // Mocking a class at 09:00 AM
int scheduledMinute = 37;
String mockClassName = "FKTEN Lab Class";

// Hardware Interrupt Service Routines (ISRs)
void IRAM_ATTR handleSnooze() { snoozePressed = true; }
void IRAM_ATTR handleStop()   { stopPressed = true; }

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize Outputs
  pinMode(ALARM_BUZZER, OUTPUT);
  pinMode(ALARM_LIGHTS, OUTPUT);
  digitalWrite(ALARM_BUZZER, LOW);
  digitalWrite(ALARM_LIGHTS, LOW);

  // Initialize Debounced Button Inputs
  pinMode(BUTTON_SNOOZE, INPUT_PULLDOWN);
  pinMode(BUTTON_STOP, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SNOOZE), handleSnooze, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_STOP), handleStop, RISING);

  // Initialize OLED Panel
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED initialization failed"));
    for(;;);
  }

  // Initialize Real-Time Clock
  if (!rtc.begin()) {
    Serial.println("DS3231 module missing!");
    for(;;);
  }

  // Check coin-cell fallback health
  rtcBatteryDead = rtc.lostPower();
  if (rtcBatteryDead) {
    Serial.println("Warning: RTC lost power. Resetting to compile time template.");
    // Automatically sets RTC to your computer's exact flashing time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  DateTime now = rtc.now();
  rtcBatteryDead = rtc.lostPower(); // Continuous power-rail polling

  // 1. Time Comparison Alarm Trigger Engine
  if (!isAlarmActive && snoozeEndTime == 0) {
    if (now.hour() == scheduledHour && now.minute() == scheduledMinute && now.second() == 0) {
      isAlarmActive = true;
      Serial.println("[ALARM TRIGGER] Match found with scheduled lecture timeline!");
    }
  }

  // 2. Handle completed snooze delay intervals
  if (!isAlarmActive && snoozeEndTime > 0 && millis() >= snoozeEndTime) {
    isAlarmActive = true;
    snoozeEndTime = 0;
    Serial.println("[ALARM RE-TRIGGER] Snooze timer expired. Waking student up.");
  }

  // 3. Process Active Alarm Alerts or Check Interrupt Buttons
  if (isAlarmActive) {
    if (stopPressed) {
      dismissAlarm();
    } 
    else if (snoozePressed) {
      triggerSnoozeCycle();
    } 
    else {
      // Create synchronous alert flash rhythms without blocking button presses
      lightStateToggle = !lightStateToggle;
      digitalWrite(ALARM_BUZZER, lightStateToggle ? HIGH : LOW);
      digitalWrite(ALARM_LIGHTS, lightStateToggle ? HIGH : LOW);
      delay(200); 
    }
  } else {
    // Keep outputs off when alarm is inactive
    digitalWrite(ALARM_BUZZER, LOW);
    digitalWrite(ALARM_LIGHTS, LOW);
  }

  // 4. Update the Visual OLED Screen Layout Matrix
  renderOLEDInterface(now);
  
  delay(50); // Yielding lag loop gap to stabilize execution balance
}

void triggerSnoozeCycle() {
  snoozePressed = false;
  if (snoozeCount < MAX_SNOOZES) {
    snoozeCount++;
    isAlarmActive = false;
    snoozeEndTime = millis() + (5 * 60 * 1000); // Postpone alert window by 5 minutes
    Serial.print("Alarm Snoozed. Active count: "); Serial.println(snoozeCount);
  } else {
    Serial.println("Max snoozes reached! You must wake up for class.");
  }
}

void dismissAlarm() {
  isAlarmActive = false;
  stopPressed = false;
  snoozePressed = false;
  snoozeCount = 0;
  snoozeEndTime = 0;
  Serial.println("Alarm successfully dismissed by student.");
}

void renderOLEDInterface(DateTime current) {
  display.clearDisplay();
  display.setTextSize(1);
  
  // Header Real-time Display Row
  display.setCursor(0, 0);
  display.print("Time: ");
  if(current.hour() < 10) display.print("0"); display.print(current.hour()); display.print(":");
  if(current.minute() < 10) display.print("0"); display.print(current.minute());

  // Render Low Battery warning cell alert block if coin-cell drains
  if (rtcBatteryDead) {
    display.drawRect(108, 1, 14, 7, SSD1306_WHITE);
    display.drawRect(122, 3, 2, 3, SSD1306_WHITE);
    if ((millis() / 500) % 2 == 0) {
       display.setCursor(75, 0);
       display.print("LOW BATT");
    }
  }

  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  // Dynamic Content Output Area
  if (isAlarmActive) {
    display.setCursor(15, 25);
    display.setTextSize(2);
    display.print("WAKE UP!!");
    display.setTextSize(1);
    display.setCursor(10, 48);
    display.print("Class: " + mockClassName);
  } else if (snoozeEndTime > 0) {
    display.setCursor(0, 22);
    display.print("Status: Alarms Snoozed");
    display.setCursor(0, 36);
    display.print("Snooze Cycle: "); display.print(snoozeCount); display.print("/"); display.print(MAX_SNOOZES);
    display.setCursor(0, 50);
    display.print("Next ring in minutes...");
  } else {
    display.setCursor(0, 20);
    display.print("Next: " + mockClassName);
    display.setCursor(0, 35);
    display.print("Alarm set at: ");
    if(scheduledHour < 10) display.print("0"); display.print(scheduledHour); display.print(":");
    if(scheduledMinute < 10) display.print("0"); display.print(scheduledMinute);
    display.setCursor(0, 52);
    display.print("System Status: Ready");
  }

  display.display();
}
