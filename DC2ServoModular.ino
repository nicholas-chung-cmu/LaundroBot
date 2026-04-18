#include "DC2ServoModular.h"
#include <Servo.h>

Servo r1Servo;
Servo l1Servo;
Servo r2Servo;
Servo l2Servo;

DC2Servo transMotor(9, 8, 7, /*encA=*/2, /*encB=*/3); // pin 9 freed up for servos

void isrA() { transMotor.handleEncA(); }
void isrB() { transMotor.handleEncB(); }

bool dir = false;
unsigned long lastMove = 0; // moved to global so it persists between calls

void setup() {
  Serial.begin(9600);
  transMotor.begin(isrA, isrB);
  transMotor.writeAngle(0);

  r1Servo.attach(11);
  l1Servo.attach(10);
  r2Servo.attach(5);
  l2Servo.attach(6);

  l1Servo.write(0);   r1Servo.write(180);
  l2Servo.write(0);   r2Servo.write(180);

  delay(5000);
}

unsigned long lastTranslate = 0;
unsigned long lastFold = 1000; // offset by 1s so it fires after translate

void loop() {
   unsigned long now = millis();
  if (now - lastMove >= 2000) {
    float move = dir ? -90.0f : 90.0f;
    transMotor.writeAngle(transMotor.getTargetAngle() + move);
    dir = !dir;  // flip direction for next time
    lastMove = now;
  }
  transMotor.update();
  transMotor.printData();
  servoFold(l1Servo, r1Servo);  /// gotta write as one function and then put in loop
  servoFold(l2Servo, r2Servo);

}

void servoFold(Servo &leftHandServo, Servo &rightHandServo) {
  int rPos = 20;
  int lPos = 160;
  unsigned long prev = 0;
  int interval = 15;
  int folderPos = 0;
  int increment = 3;

  while (folderPos < 120) {
    unsigned long current = millis();
    if (current - prev >= interval) {
      rightHandServo.write(rPos);
      leftHandServo.write(lPos);
      // Serial.print("Left Servo: ");
      // Serial.print(lPos);
      // Serial.print(" Right Servo: ");
      // Serial.println(rPos);
      rPos += increment;
      lPos -= increment;
      folderPos += increment;
      prev = millis();
    }
  }
}

void translate(DC2Servo &motor, bool &dir) {
  unsigned long now = millis();
  if (now - lastMove >= 1000) {
    float move = dir ? -90.0f : 90.0f;
    motor.writeAngle(motor.getTargetAngle() + move);
    dir = !dir;
    lastMove = now;
  }
}