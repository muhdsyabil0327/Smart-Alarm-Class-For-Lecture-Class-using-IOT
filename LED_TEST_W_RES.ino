void setup() {
  Serial.begin(115200);
  Serial.println("--- Starting Parallel 3.3V Red LED Test ---");
}

void loop() {
  // Test Case A: Check standard Pin 26 assignment
  Serial.println("[CHECK] Testing Board Layout Label D26...");
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
  delay(1500); 
  digitalWrite(26, LOW);
  delay(500);

  // Test Case B: Check alternative Pin 13 tracking layout
  Serial.println("[CHECK] Testing Board Layout Label D13...");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(1500); 
  digitalWrite(13, LOW);
  delay(500);
  
  // Test Case C: Check alternative Pin 4 tracking layout
  Serial.println("[CHECK] Testing Board Layout Label D4...");
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(1500); 
  digitalWrite(4, LOW);
  delay(1500);
}
