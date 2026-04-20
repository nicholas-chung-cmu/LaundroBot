#include "DC2Servo.h"

// ─────────────────────────────────────────────────────────────────
// Constructor
// Initializes all pin assignments, PID gains, and internal state.
// Uses an initializer list to set every member before the body runs.
// ─────────────────────────────────────────────────────────────────
DC2Servo::DC2Servo(int pinPWM, int pinIN1, int pinIN2, int pinEncA, int pinEncB,
                   float countsPerRev, float kp, float ki, float kd)
  : _pinPWM(pinPWM), _pinIN1(pinIN1), _pinIN2(pinIN2),
    _pinEncA(pinEncA), _pinEncB(pinEncB),
    _countsPerRev(countsPerRev), _degPerCount(360.0f / countsPerRev),
    _encoderCount(0), _currentAngle(0), _targetAngle(0),
    _kp(kp), _ki(ki), _kd(kd),
    _integralTerm(0), _prevError(0), _prevTime(0), _lastPWM(0)
{}

// ─────────────────────────────────────────────────────────────────
// begin()
// Call once in setup(). Configures all pins, sets PWM resolution
// to 8-bit (required on Teensy 4.1 which defaults to 10-bit),
// attaches encoder ISRs, and seeds the PID timer.
// ─────────────────────────────────────────────────────────────────
void DC2Servo::begin(void (*isrA)(), void (*isrB)()) {
  // Teensy 4.1 defaults to 10-bit PWM (0–1023).
  // Force 8-bit (0–255) to match the rest of the code.
  analogWriteResolution(8);

  // Motor driver pins
  pinMode(_pinPWM, OUTPUT);
  pinMode(_pinIN1, OUTPUT);
  pinMode(_pinIN2, OUTPUT);
  analogWrite(_pinPWM, 0);   // ensure motor starts stopped

  // Encoder pins — plain INPUT because the signal is actively driven
  // through the level shifter; no internal pull-up/down needed.
  pinMode(_pinEncA, INPUT);
  pinMode(_pinEncB, INPUT);

  // CHANGE fires the ISR on both rising and falling edges,
  // giving full quadrature resolution (4× counts per cycle).
  attachInterrupt(digitalPinToInterrupt(_pinEncA), isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_pinEncB), isrB, CHANGE);

  // Seed timer with micros() for sub-millisecond PID dt accuracy
  _prevTime = micros();
}

// ─────────────────────────────────────────────────────────────────
// writeAngle()
// Set a new target angle in degrees. The motor will drive toward
// this on the next update() call.
// ─────────────────────────────────────────────────────────────────
void DC2Servo::writeAngle(float degrees) {
  _targetAngle = degrees;
}

// ─────────────────────────────────────────────────────────────────
// getTargetAngle() / getCurrentAngle()
// Simple getters for external monitoring.
// ─────────────────────────────────────────────────────────────────
float DC2Servo::getTargetAngle() const {
  return _targetAngle;
}

float DC2Servo::getCurrentAngle() const {
  return _currentAngle;
}

// ─────────────────────────────────────────────────────────────────
// update()
// Main PID loop — call every iteration of loop().
// Reads encoder, computes PID output, drives motor.
// Returns the PWM value applied (0–255).
// ─────────────────────────────────────────────────────────────────
int DC2Servo::update() {
  // micros() gives better dt resolution than millis() for fast motors
  unsigned long now = micros();
  float dt = (now - _prevTime) / 1000000.0f;  // convert to seconds
  if (dt <= 0) return _lastPWM;               // skip if no time has passed
  _prevTime = now;

  // Safely copy encoder count — disable interrupts briefly to prevent
  // a race condition where the ISR modifies _encoderCount mid-read.
  noInterrupts();
  long count = _encoderCount;
  interrupts();

  // Convert encoder count to degrees
  _currentAngle = count * _degPerCount;

  // ── PID Calculation ─────────────────────────────────────────────
  float error = _targetAngle - _currentAngle;

  // Integral: accumulate error over time, clamped to prevent windup
  _integralTerm = constrain(_integralTerm + error * dt, -100.0f, 100.0f);

  // Derivative: rate of change of error
  float deriv = (error - _prevError) / dt;
  _prevError = error;

  // Combine P, I, D terms into a single control signal
  float signal = _kp * error + _ki * _integralTerm + _kd * deriv;

  // Map control signal magnitude to PWM range (0–255)
  int pwm = constrain((int)abs(signal), 0, 255);

  // Deadband: within 1 degree of target, stop and reset integral
  // to prevent the motor fighting around the setpoint.
  if (abs(error) < 1.0f) {
    _integralTerm = 0;
    _drive(0, true);
    return _lastPWM = 0;
  }

  // Drive motor in the direction indicated by the sign of the signal
  _drive(pwm, signal > 0);
  return _lastPWM = pwm;
}

// ─────────────────────────────────────────────────────────────────
// printData()
// Prints a CSV-friendly debug line to Serial.
// Format: time , target , current , error , pwm
// ─────────────────────────────────────────────────────────────────
void DC2Servo::printData() const {
  Serial.print(millis() / 1000.0f, 2);
  Serial.print(" , target: ");   Serial.print(_targetAngle, 2);
  Serial.print(" , current: ");  Serial.print(_currentAngle, 2);
  Serial.print(" , error: ");    Serial.print(_targetAngle - _currentAngle, 2);
  Serial.print(" , pwm: ");      Serial.println(_lastPWM);
}

// ─────────────────────────────────────────────────────────────────
// handleEncA() / handleEncB()
// ISR handlers — call these from your sketch-level ISR wrappers.
// Direction is determined by comparing the state of both channels.
// ─────────────────────────────────────────────────────────────────
void DC2Servo::handleEncA() {
  bool a = digitalRead(_pinEncA);
  bool b = digitalRead(_pinEncB);
  if (a == b) _encoderCount++; else _encoderCount--;
}

void DC2Servo::handleEncB() {
  bool a = digitalRead(_pinEncA);
  bool b = digitalRead(_pinEncB);
  if (a != b) _encoderCount++; else _encoderCount--;
}

// ─────────────────────────────────────────────────────────────────
// _drive()
// Private helper — sets motor direction and applies PWM.
// forward=true drives one way, forward=false reverses.
// ─────────────────────────────────────────────────────────────────
void DC2Servo::_drive(int pwm, bool forward) {
  digitalWrite(_pinIN1, forward ? HIGH : LOW);
  digitalWrite(_pinIN2, forward ? LOW  : HIGH);
  analogWrite(_pinPWM, pwm);
}
// update(): the main PID loop — call this every iteration of loop().
// Reads the encoder, computes PID output, and drives the motor.
// Returns the PWM value that was applied (0–255).
int DC2Servo::update() {
  // Calculate time elapsed since last update in seconds
  unsigned long now = millis();
  float dt = (now - _prevTime) / 1000.0f;
  if (dt <= 0) return _lastPWM;  // skip if no time has passed
  _prevTime = now;

  // Safely read encoder count (disable interrupts briefly to avoid
  // a race condition where the ISR modifies _encoderCount mid-read)
  noInterrupts();
  long count = _encoderCount;
  interrupts();

  // Convert encoder count to degrees
  _currentAngle = count * _degPerCount;

  // --- PID Calculation ---
  float error = _targetAngle - _currentAngle;

  // Integral: accumulate error over time, clamped to prevent windup
  _integralTerm = constrain(_integralTerm + error * dt, -100.0f, 100.0f);

  // Derivative: rate of change of error
  float deriv = (error - _prevError) / dt;
  _prevError = error;

  // Combine P, I, D terms into control signal
  float signal = _kp * error + _ki * _integralTerm + _kd * deriv;

  // Convert control signal magnitude to a PWM value (0–255)
  int pwm = constrain((int)abs(signal), 0, 255);

  // Deadband: if error is less than 1 degree, stop the motor
  // and reset integral to prevent fighting around the target
  if (abs(error) < 1.0f) {
    _integralTerm = 0;
    _drive(0, true);
    return _lastPWM = 0;
  }

  // Drive motor in the correct direction based on sign of control signal
  _drive(pwm, signal > 0);
  return _lastPWM = pwm;
}

// printData(): prints a CSV-style line to Serial for debugging/plotting.
// Format: time , target , current , error , pwm
void DC2Servo::printData() const {
  Serial.print(millis() / 1000.0f, 2);
  Serial.print(" , target: ");   Serial.print(_targetAngle, 2);
  Serial.print(" , current: ");  Serial.print(_currentAngle, 2);
  Serial.print(" , error: ");    Serial.print(_targetAngle - _currentAngle, 2);
  Serial.print(" , pwm: ");      Serial.println(_lastPWM);
}

// handleEncA(): ISR for encoder channel A.
// Determines direction by comparing A and B states.
void DC2Servo::handleEncA() {
  bool a = digitalRead(_pinEncA);
  bool b = digitalRead(_pinEncB);
  if (a == b) _encoderCount++; else _encoderCount--;
}

// handleEncB(): ISR for encoder channel B.
// Determines direction by comparing A and B states (logic is inverted vs A).
void DC2Servo::handleEncB() {
  bool a = digitalRead(_pinEncA);
  bool b = digitalRead(_pinEncB);
  if (a != b) _encoderCount++; else _encoderCount--;
}

// _drive(): private helper to set motor direction and speed.
// forward=true spins one way, forward=false reverses it.
void DC2Servo::_drive(int pwm, bool forward) {
  digitalWrite(_pinIN1, forward ? HIGH : LOW);
  digitalWrite(_pinIN2, forward ? LOW  : HIGH);
  analogWrite(_pinPWM, pwm);
}
