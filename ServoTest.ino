//#include <Servo.h>
#include <PWMServo.h>
PWMServo r1Servo;   // Create servo object
PWMServo l1Servo;   // Create servo object
PWMServo r2Servo;   // Create servo object
PWMServo l2Servo;   // Create servo object

int pos = 180;  // Variable to store servo position

void setup() {
  r1Servo.attach(9);  // Attach servo to pin 9
  l1Servo.attach(10);
  l1Servo.write(0);
  r1Servo.write(180);

  r2Servo.attach(5);  // Attach servo to pin 9
  l2Servo.attach(6);
  l2Servo.write(0);
  r2Servo.write(180);
  Serial.begin(9600);
  delay(10000);
  //pinMode(A9,INPUT);

}

void loop() {

  servoFold(l1Servo, r1Servo);  /// gotta write as one function and then put in loop
  servoFold(l2Servo, r2Servo);
  Serial.println("test");
}




void servoFold(PWMServo &leftHandServo, PWMServo &rightHandServo) {
  int rPos = 20;
  int lPos = 160;
  int prev = 0;
  int interval = 15;
  int folderPos = 0; 
  int increment = 3;
  int time2move = interval*12;
  unsigned long start = millis();
  unsigned long now = 0;

while(now - start <= time2move){
  now = millis();
}
if(now - start >= time2move){

  while (folderPos < 160) {//160 to give it more drive to get to 120 in the real world 
    unsigned long current = millis(); // start internal delay
    if (current - prev >= interval) { // check if time elapsed is 15ms
      rightHandServo.write(rPos); // write motor postitions
      leftHandServo.write(lPos);
      //Serial.println("left Servo: ");// print positions
      //Serial.print(lPos);
      //Serial.print(" Right Servo ");
      //Serial.print(rPos);
      rPos += increment; // increase motor pos in different directions 
      lPos -= increment;
      folderPos += increment;// increment for while loop
      prev = millis(); // set new delay 
    }
  }
}
}


