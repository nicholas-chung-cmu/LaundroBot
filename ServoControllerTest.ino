#include "servo_controller.h"

// Sample .ino file demonstrating ServoController usage
// Copy-paste this into Arduino IDE (or Teensyduino for Teensy 4.1)
// Assumes servo_controller.h is in the same folder or include path

// Define 8 servo pins (adjust to your wiring)
int bigA = 7;
int bigB = 8;
int bigC = 9;
int bigD = 10;
int smallA = 2;
int smallB = 3;
int smallC = 4;
int smallD = 5;

// Big: top A/B, bot A/B; Small: left A/B, right A/B
const int servoPins[8] = {bigA, bigB, bigC, bigD, smallA, smallB, smallC, smallD};  // Example pins

ServoController controller;

void setup() {
  Serial.begin(115200);
  controller.init(servoPins);  // Attach servos and set to rest angles
  Serial.println("ServoController initialized. Starting fold sequence...");
}

void loop() {
  static unsigned long lastAction = 0;
  unsigned long now = millis();

  // Simple sequence: Fold bottom (2s), then top (2s), then unfold all (2s), repeat
  if (now - lastAction > 2000) {  // Every 2 seconds
    static int step = 0;
    if (step == 0) {
      controller.foldBottom();  // Fold big bottom A/B
      Serial.println("Folding bottom...");
    } else if (step == 1) {
      controller.unfoldBottom();   // Unfold bottom
      Serial.println("Unfolding bottom...");
    } else if (step == 2) {
      controller.foldTop();     // Fold big top C/D
      Serial.println("Folding top...");
    } else if (step == 3) {
      controller.unfoldTop();   // Unfold top
      Serial.println("Unfolding top...");
    } else if (step == 4) {
      controller.foldLeft();   // Fold small left A/B
      Serial.println("Folding left...");
    } else if (step == 5) {
      controller.unfoldLeft();   // Unfold all left 
      Serial.println("Unfolding left...");
       } else if (step == 6) {
      controller.foldRight();   // Fold all small right (CD)
      Serial.println("Folding Right...");
    } else if (step == 7) {
      controller.unfoldRight();   // Unfold Right
      Serial.println("Unfolding Right...");
    }
    step = (step + 1) % 8;  // Cycle through steps
    lastAction = now;
  }

  // Update servo positions (non-blocking, every 15ms internally)
  controller.update();

  // Check if current move is complete (optional debug)
  if (controller.allMovesComplete()) {
    // Could add logic here, e.g., proceed to next step
  }

  // Small delay to prevent loop from hogging CPU
  delay(10);
}