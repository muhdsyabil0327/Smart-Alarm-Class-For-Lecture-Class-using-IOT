// Configuration Pins from Phase 3 & 4
const int ALARM_BUZZER = 25;
const int ALARM_LIGHTS = 26;
const int BUTTON_SNOOZE = 14;
const int BUTTON_STOP   = 12;

// Volatile flags updated inside the background Hardware Interrupt Service Routines
volatile bool snoozeTriggered = false;
volatile bool stopTriggered = false;

// Interrupt functions stored inside the ESP32 RAM execution matrix
void IRAM_ATTR checkSnooze() {
  snoozeTriggered = true;
}

void IRAM_ATTR checkStop() {
  stopTriggered = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("\n--- UniMAP FYP: Phase 4 Button Interrupt Test ---");

  pinMode(ALARM_BUZZER, OUTPUT);
  pinMode(ALARM_LIGHTS, OUTPUT);
  digitalWrite(ALARM_BUZZER, LOW);
  digitalWrite(ALARM_LIGHTS, LOW);

  // Configure your 3-pin modules to detect a raw incoming data signal
  pinMode(BUTTON_SNOOZE, INPUT_PULLDOWN);
  pinMode(BUTTON_STOP, INPUT_PULLDOWN);

  // Link the physical pins to our background interrupt functions
  attachInterrupt(digitalPinToInterrupt(BUTTON_SNOOZE), checkSnooze, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_STOP), checkStop, RISING);

  Serial.println("System armed. Simulating an active ringing morning alarm...");
}

void loop() {
  // Simulate an active ringing alarm by default
  Serial.println("[ALARM] Ringing! Press SNOOZE or STOP button to test...");
  
  // Flash lights and beep buzzer synchronously
  digitalWrite(ALARM_BUZZER, HIGH);
  digitalWrite(ALARM_LIGHTS, HIGH);
  
  // Instead of using a blocking delay(1000), we check the buttons every 10ms
  for (int i = 0; i < 50; i++) {
    if (snoozeTriggered) {
      Serial.println("\n[SUCCESS] Snooze Button Intercept Confirmed!");
      blinkConfirmation(2); // Double blink to show it worked
      snoozeTriggered = false;
      break; 
    }
    if (stopTriggered) {
      Serial.println("\n[SUCCESS] Stop/Dismiss Button Intercept Confirmed!");
      blinkConfirmation(4); // Quad blink to show it worked
      stopTriggered = false;
      break;
    }
    delay(10); 
  }

  digitalWrite(ALARM_BUZZER, LOW);
  digitalWrite(ALARM_LIGHTS, LOW);
  delay(500); // Brief quiet space before repeating the alarm ring
}

// Visual confirmation routine when button is pushed successfully
void blinkConfirmation(int count) {
  digitalWrite(ALARM_BUZZER, LOW);
  digitalWrite(ALARM_LIGHTS, LOW);
  delay(200);
  for (int i = 0; i < count; i++) {
    digitalWrite(ALARM_LIGHTS, HIGH);
    delay(100);
    digitalWrite(ALARM_LIGHTS, LOW);
    delay(100);
  }
  delay(1000); // Give user a 1-second pause to see the reset happen
}
