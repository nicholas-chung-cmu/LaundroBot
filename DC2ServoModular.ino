#include "DC2ServoModular.h"

//#include <Servo.h>

// Servo r1Servo;
// Servo l1Servo;
// Servo r2Servo;
// Servo l2Servo;
/*// In setup()
DC2Servo motor(9, 8, 7, 2, 3);  // Pins, defaults
motor.begin(isrA, isrB);
motor.writeAngle(0);  // Start at 0°

// In loop()
motor.update();       // Run PID constantly (dt~0.0001s, PWM 0-255)
motor.printData();    // Debug: "0.00 , target: 45.00 , current: 0.00 , error: 45.00 , pwm: 191"

// Manual angle changes (e.g., based on time, button, or serial)
unsigned long now = micros();
if (now > 2000000 && now < 4000000) {  // 2-4s: Go to 45°
  motor.writeAngle(45.0f);  // Sets target; PID drives to 45° (error=45°, PWM~191)
} else if (now > 4000000) {  // After 4s: Go to 90°
  motor.writeAngle(90.0f);  // Sets target; PID drives to 90° (error=90°, PWM=255)
}
// No translate() call needed—angles set manually
*/

DC2Servo transMotor(4, 22, 23, /*encA=*/0, /*encB=*/1); // pin 9 freed up for servos

void isrA() { transMotor.handleEncA(); }
void isrB() { transMotor.handleEncB(); }

bool  dir = 0;
unsigned long lastMove = 0; // moved to global so it persists between calls

void setup() {
  Serial.begin(9600);
  transMotor.begin(isrA, isrB);
  transMotor.writeAngle(0);

  // r1Servo.attach(11);
  // l1Servo.attach(10);
  // r2Servo.attach(5);
  // l2Servo.attach(6);

  // l1Servo.write(0);   r1Servo.write(180);
  // l2Servo.write(0);   r2Servo.write(180);

  delay(5000);

}

unsigned long lastTranslate = 0;
unsigned long lastFold = 1000; // offset by 1s so it fires after translate

void loop() {
//   transMotor.update();       // Run PID constantly (dt~0.0001s, PWM 0-255)
// transMotor.printData();    // Debug: "0.00 , target: 45.00 , current: 0.00 , error: 45.00 , pwm: 191"
// // Manual angle changes (e.g., based on time, button, or serial)
// unsigned long now = micros();
// if (now > 2000000 && now < 4000000) {  // 2-4s: Go to 45°
//   transMotor.writeAngle(45.0f);  // Sets target; PID drives to 45° (error=45°, PWM~191)
// } else if (now > 4000000) {  // After 4s: Go to 90°
//   transMotor.writeAngle(90.0f);  // Sets target; PID drives to 90° (error=90°, PWM=255)

translate(transMotor, dir, lastMove, 20.0f);  // Every 2s: Starts ramp to 0° ↔ 90°

transMotor.update();                          // PID: dt~0.0001s, error calc, PWM 0-255
                                         // Example: error=90°, P=427.5, PWM=255 (full speed forward)
transMotor.printData();                       // Every 100ms: Prints "2.00 , target: 90.00 , current: 0.00 , error: 90.00 , pwm: 255"

}

void translate(DC2Servo &motor, bool &dir, unsigned long &lastMove, float angle) {
  unsigned long now = millis();
  if (now - lastMove >= 2000) {  // Changed to 2000ms as in original
    float target = motor.getCurrentAngle() + (dir ? -angle : angle);
    motor.startTrapezoidalRamp(target, 1000*1);  // Trapezoidal ramp over 0.5 seconds
    dir = !dir;
    lastMove = now;
  }
}

  //  unsigned long now = millis();
  // if (now - lastMove >= 2000) {
  //   float move = dir ? -90.0f : 90.0f;
  //   transMotor.writeAngle(transMotor.getTargetAngle() + move);
  //   dir = !dir;  // flip direction for next time
  //   lastMove = now;
  // }
  // transMotor.update();
  // transMotor.printData();
  // servoFold(l1Servo, r1Servo);  /// gotta write as one function and then put in loop
  // servoFold(l2Servo, r2Servo);



// void servoFold(Servo &leftHandServo, Servo &rightHandServo) {
//   int rPos = 20;
//   int lPos = 160;
//   unsigned long prev = 0;
//   int interval = 15;
//   int folderPos = 0;
//   int increment = 3;

//   while (folderPos < 120) {
//     unsigned long current = millis();
//     if (current - prev >= interval) {
//       rightHandServo.write(rPos);
//       leftHandServo.write(lPos);
//       // Serial.print("Left Servo: ");
//       // Serial.print(lPos);
//       // Serial.print(" Right Servo: ");
//       // Serial.println(rPos);
//       rPos += increment;
//       lPos -= increment;
//       folderPos += increment;
//       prev = millis();
//     }
//   }
// }

// void translate(DC2Servo &motor, bool &dir) {
//   unsigned long now = millis();
//   if (now - lastMove >= 1000) {
//     float move = dir ? -90.0f : 90.0f;
//     motor.writeAngle(motor.getTargetAngle() + move);
//     dir = !dir;
//     lastMove = now;
//   }
//}