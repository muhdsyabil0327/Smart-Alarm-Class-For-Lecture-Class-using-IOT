// Phase 3 Configuration Pins
const int ALARM_BUZZER = 25;
const int ALARM_LIGHTS = 26;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor window to open
  
  Serial.println("\n--- UniMAP FYP: Phase 3 Output Component Test ---");
  
  // Initialize transistor driver control pins as outputs
  pinMode(ALARM_BUZZER, OUTPUT);
  pinMode(ALARM_LIGHTS, OUTPUT);
  
  // Force all outputs LOW (off) initially for circuit stability
  digitalWrite(ALARM_BUZZER, LOW);
  digitalWrite(ALARM_LIGHTS, LOW);
  
  Serial.println("Drivers initialized. Starting execution loop...");
}

void loop() {
  // Step 1: Isolate and test the Buzzer transistor path
  Serial.println("[TEST] Sounding Active Buzzer (LEDs OFF)");
  digitalWrite(ALARM_BUZZER, HIGH); // Sends 3.3V to 2N2222 Base to turn it ON
  digitalWrite(ALARM_LIGHTS, LOW);  // Keeps LEDs turned OFF
  delay(1000);                      // Hold sound for 1 second

  // Step 2: Silence the buzzer and create a small gap
  digitalWrite(ALARM_BUZZER, LOW);
  delay(500);

  // Step 3: Isolate and test the 4-LED Cluster transistor path
  Serial.println("[TEST] Flashing 4-LED Array (Buzzer OFF)");
  digitalWrite(ALARM_BUZZER, LOW);  // Keeps Buzzer turned OFF
  digitalWrite(ALARM_LIGHTS, HIGH); // Sends 3.3V to 2N2222 Base to turn it ON
  delay(1000);                      // Keep lights glowing for 1 second

  // Step 4: Turn everything off before repeating the sequence
  digitalWrite(ALARM_LIGHTS, LOW);
  Serial.println("[INFO] Cool-down phase. Standby...\n");
  delay(2000);                      // Wait 2 seconds before looping
}
