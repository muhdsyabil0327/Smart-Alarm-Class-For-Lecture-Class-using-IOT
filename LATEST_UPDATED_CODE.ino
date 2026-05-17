#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// Screen Configurations
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin Configuration Matrix
const int ALARM_BUZZER  = 25;
const int ALARM_LIGHTS  = 26;
const int BUTTON_SNOOZE = 14;
const int BUTTON_STOP   = 12;

RTC_DS3231 rtc;

// Network Access Credentials
const char* ssid = "YOUR_WIFI_SSID";         // Replace with your WiFi name
const char* password = "YOUR_WIFI_PASSWORD"; // Replace with your WiFi password

// Absolute Base URL & Key Assignments
const char* supabase_base_url = "https://supabase.co";
const char* supabase_apikey = "Your_public_Anon_key";

// Volatile Interrupt Handlers
volatile bool snoozePressed = false;
volatile bool stopPressed   = false;

// Alarm Framework Variables
bool isAlarmActive = false;
bool lightStateToggle = false;
unsigned long snoozeEndTime = 0;
int snoozeCount = 0;
const int MAX_SNOOZES = 3;
bool rtcBatteryDead = false;
bool isSnoozing = false; // BUG FIX FLAG: Locks cloud synchronization out during snooze routines

// Dynamic Cloud Memory Allocation Variables
int scheduledHour = 0;
int scheduledMinute = 0;
String cloudClassName = "Loading...";
unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 10000; // 10-second sync window for breadboard verification

void IRAM_ATTR handleSnooze() { snoozePressed = true; }
void IRAM_ATTR handleStop()   { stopPressed = true; }

void renderOLEDMessage(String l1, String l2) {
  display.clearDisplay();
  display.setCursor(0, 15); display.print(l1);
  display.setCursor(0, 35); display.print(l2);
  display.display();
}

void syncWithSupabase() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Fetch current real hardware clock time to build the dynamic query filter
  DateTime currentNow = rtc.now();
  char currentTimeStr[9];
  sprintf(currentTimeStr, "%02d:%02d:00", currentNow.hour(), currentNow.minute());
  
  // Converts standard Sunday=0 to match your Monday=1 column index setup
  int dayIdx = currentNow.dayOfTheWeek();
  if (dayIdx == 0) dayIdx = 7;

  // DYNAMIC FILTERING GET STRING: Pulls only classes whose time >= current clock hour/minute
  String dynamicUrl = String(supabase_base_url) + 
                      "?day_index=eq." + String(dayIdx) +
                      "&lecture_time=gte." + String(currentTimeStr) +
                      "&order=lecture_time.asc&limit=1";

  WiFiClientSecure client;
  client.setInsecure(); // Permits SSL handshake over port 443 safely
  
  HTTPClient http;
  Serial.println("\n[SECURE DYNAMIC LINK] Filtering future timeline entries...");
  http.begin(client, dynamicUrl);
  
  // Inject required header tokens
  http.addHeader("apikey", supabase_apikey);
  String authValue = "Bearer " + String(supabase_apikey);
  http.addHeader("Authorization", authValue.c_str());
  http.addHeader("Accept", "application/json");
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("[RAW DATA RECEIVED]: " + payload);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      JsonArray arr = doc.as<JsonArray>();
      if (arr.size() > 0) {
        JsonObject currentSchedule = arr[0].as<JsonObject>(); 
        
        cloudClassName = currentSchedule["class_name"].as<String>();
        String rawTime = currentSchedule["lecture_time"].as<String>();
        
        scheduledHour = rawTime.substring(0, 2).toInt();
        scheduledMinute = rawTime.substring(3, 5).toInt();
        
        Serial.println("[SUCCESS] Next upcoming class locked from database!");
        Serial.print("Class: "); Serial.println(cloudClassName);
        Serial.print("Alarm Active At: "); Serial.print(scheduledHour); Serial.print(":"); Serial.println(scheduledMinute);
      } else {
        Serial.println("[DATABASE INFO] No more classes scheduled for the rest of today!");
        cloudClassName = "No Classes Left";
        scheduledHour = 0;
        scheduledMinute = 0;
      }
    } else {
      Serial.print("[JSON PARSE FAILURE] Reason: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("[GATEWAY RESPONSE] Request failed. Connection Code: ");
    Serial.println(httpCode);
  }
  http.end();
}

void dismissAlarm() {
  isAlarmActive = false;
  stopPressed = false;
  snoozePressed = false;
  isSnoozing = false; // Unlock cloud syncing
  snoozeCount = 0;
  snoozeEndTime = 0;
  noTone(ALARM_BUZZER);
  digitalWrite(ALARM_LIGHTS, LOW);
  Serial.println("Alarm Dismissed by Student.");
}

void triggerSnooze() {
  snoozePressed = false;
  if (snoozeCount < MAX_SNOOZES) {
    snoozeCount++;
    isAlarmActive = false;
    isSnoozing = true; // BUG FIX: Lock the current alarm name in place
    snoozeEndTime = millis() + (5 * 60 * 1000); // 5-minute snooze timeout
    noTone(ALARM_BUZZER);
    digitalWrite(ALARM_LIGHTS, LOW);
    Serial.print("Alarm Snoozed. Active count: "); Serial.println(snoozeCount);
  } else {
    Serial.println("Max snoozes reached! You must wake up for class.");
  }
}

void renderMainInterface(DateTime current) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Time: ");
  if(current.hour() < 10) display.print("0"); display.print(current.hour()); display.print(":");
  if(current.minute() < 10) display.print("0"); display.print(current.minute());

  if (rtcBatteryDead) {
    display.drawRect(108, 1, 14, 7, SSD1306_WHITE);
    display.drawRect(122, 3, 2, 3, SSD1306_WHITE);
  }

  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  if (isAlarmActive) {
    display.setCursor(15, 25); display.setTextSize(2); display.print("WAKE UP!!");
    display.setTextSize(1); display.setCursor(0, 50); display.print(cloudClassName);
  } else if (isSnoozing) { // Render snooze interface when flag is active
    display.setCursor(0, 22); display.print("Snoozed: " + cloudClassName);
    display.setCursor(0, 38); display.print("Cycle: "); display.print(snoozeCount); display.print("/"); display.print(MAX_SNOOZES);
    display.setCursor(0, 52); display.print("Holding current alarm...");
  } else {
    display.setCursor(0, 22); display.print("Next: " + cloudClassName);
    display.setCursor(0, 42); display.print("Alarm: ");
    if(scheduledHour < 10) display.print("0"); display.print(scheduledHour); display.print(":");
    if(scheduledMinute < 10) display.print("0"); display.print(scheduledMinute);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(ALARM_BUZZER, OUTPUT);
  pinMode(ALARM_LIGHTS, OUTPUT);
  digitalWrite(ALARM_BUZZER, LOW);
  digitalWrite(ALARM_LIGHTS, LOW);

  pinMode(BUTTON_SNOOZE, INPUT_PULLDOWN);
  pinMode(BUTTON_STOP, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SNOOZE), handleSnooze, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_STOP), handleStop, RISING);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  if (!rtc.begin()) { for(;;); }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Network");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    renderOLEDMessage("IoT Syncing", "Connecting WiFi...");
  }
  Serial.println("\nWiFi Connected successfully!");
}

void loop() {
  DateTime now = rtc.now();
  rtcBatteryDead = rtc.lostPower();

  // BUG FIX CONTROL POINT: Cloud synchronizer block only runs if NOT currently snoozing
  if (!isSnoozing && (millis() - lastFetchTime >= fetchInterval || lastFetchTime == 0)) {
    if (WiFi.status() == WL_CONNECTED) {
      syncWithSupabase();
      lastFetchTime = millis();
    }
  }

  // GLOBAL TIMELINE LOCK VARIABLE
  static int lastTriggeredMinute = -1;

  // SYSTEM TIMELINE ENGINE: Checked inside a true 1-minute window
  if (!isAlarmActive && !isSnoozing && cloudClassName != "Loading..." && cloudClassName != "No Classes Left") {
    if (now.hour() == scheduledHour && now.minute() == scheduledMinute) {
      if (now.minute() != lastTriggeredMinute) {
        isAlarmActive = true;
        lastTriggeredMinute = now.minute(); 
        Serial.println("\n[ALARM ENGINE ACTIVATED] Wake up alert sequence running!");
      }
    }
  }

  // AUTOMATIC NEXT-CLASS STATE RESET: Clears the minute lock once the class time passes 
  if (now.minute() != scheduledMinute && lastTriggeredMinute != -1) {
    lastTriggeredMinute = -1; 
    Serial.println("[INFO] Alarm gate tracking reset. Armed for the next class entry.");
  }

  // Handle snooze expiration tracking
  if (!isAlarmActive && isSnoozing && millis() >= snoozeEndTime) {
    isAlarmActive = true;
    snoozeEndTime = 0;
    Serial.println("[ALARM RE-TRIGGER] Snooze over.");
  }

  // Run alert sequence behavior patterns
  if (isAlarmActive) {
    if (stopPressed) {
      dismissAlarm();
    } else if (snoozePressed) {
      triggerSnooze();
    } else {
      lightStateToggle = !lightStateToggle;
      
      if (lightStateToggle) {
        digitalWrite(ALARM_LIGHTS, HIGH);
        tone(ALARM_BUZZER, 2000, 200); 
      } else {
        digitalWrite(ALARM_LIGHTS, LOW);
        noTone(ALARM_BUZZER);
      }
      delay(200); 
    }
  } else {
    if (!snoozePressed && !stopPressed) {
      noTone(ALARM_BUZZER);
    }
  }

  renderMainInterface(now);
  delay(50);
}
