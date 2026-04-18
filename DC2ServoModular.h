#ifndef DC2ServoModular_H
#define DC2ServoModular_H

#include "Arduino.h"

class DC2Servo {
  public:
    // Constructor — pin order: PWM, IN1, IN2, EncA, EncB
    // PID gains and counts per rev have defaults so they're optional
    DC2Servo(int pinPWM, int pinIN1, int pinIN2, int pinEncA, int pinEncB,
             float countsPerRev = 1632.67f,
             float kp = 4.75f, float ki = 0.25f, float kd = 0.25f);

    void  begin(void (*isrA)(), void (*isrB)()); // call in setup()
    void  writeAngle(float degrees);             // set target angle
    int   update();                              // run PID, call in loop()
    void  printData() const;                     // print debug info to Serial

    float getTargetAngle()  const;   // returns target angle in degrees
    float getCurrentAngle() const;   // returns measured angle in degrees

    // Called by encoder ISRs — do not call directly
    void handleEncA();
    void handleEncB();

  private:
    int   _pinPWM, _pinIN1, _pinIN2, _pinEncA, _pinEncB;
    float _countsPerRev, _degPerCount;
    float _kp, _ki, _kd;

    volatile long _encoderCount;  // modified by ISRs, must be volatile
    float _currentAngle, _targetAngle;
    float _integralTerm, _prevError;
    unsigned long _prevTime;
    int   _lastPWM;

    void _drive(int pwm, bool forward);  // internal motor driver helper
};

#endif