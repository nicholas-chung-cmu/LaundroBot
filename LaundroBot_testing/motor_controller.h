

#pragma once
#include <Arduino.h>

// ── Physical Range Constants ─────────────────────────────────────────────────
// Distances are from the CENTER of the board outward, in mm.
// These mirror the constants in fold_library.py — keep them in sync.
//
// Big folder:   top folds DOWN, bottom folds UP    — 4"–8" from center
// Small folder: left folds RIGHT, right folds LEFT — 4"–9" from center
//
// Homing drives all motors INWARD until the mechanical stop at 4" (MIN).
// After homing, encoders are zeroed at MIN position (slot 0 for big folders).
// All subsequent targets are offsets from this zeroed home position.

constexpr float INCHES_TO_MM      = 25.4f;

constexpr float BIG_MIN_MM        = 4.3f * INCHES_TO_MM;   // 101.6 mm
constexpr float BIG_MAX_MM        = 8.0f * INCHES_TO_MM;   // 203.2 mm
constexpr float BIG_RANGE_MM      = BIG_MAX_MM - BIG_MIN_MM; // 101.6 mm of travel

constexpr float SMALL_MIN_MM      = 4.3f * INCHES_TO_MM;   // 101.6 mm
constexpr float SMALL_MAX_MM      = 7.4f * INCHES_TO_MM;   // 228.6 mm

// ── Calibration Constants (TODO: fill in after hardware measurement) ─────────
constexpr int   NUM_MOTORS           = 4;

// Derived range limits in encoder counts (computed from physical constants above)
// Used for bounds-checking commands before executing them.
// Both big and small folder counts are offsets from their respective home (zeroed) positions.
constexpr int   BIG_MAX_INDEX       = 10;  // TODO: define based on range
constexpr int   BIG_INCREMENT_COUNTS = 100;  // TODO: calibrate
constexpr int   BIG_MAX_COUNTS   = BIG_MAX_INDEX * BIG_INCREMENT_COUNTS;
constexpr int   SMALL_MIN_COUNTS = 0;
constexpr int   SMALL_MAX_COUNTS = 500;  // TODO: calibrate
constexpr float SMALL_COUNTS_PER_MM = 10.0f;  // TODO: calibrate
constexpr float KP = 4.75f;   // TODO
constexpr float KI = 0.25f;   // TODO
constexpr float KD = 0.25f;   // TODO
constexpr int   POSITION_DEADBAND = 5;  // encoder counts

// Stall detection
constexpr int   STALL_VELOCITY_THRESHOLD = 5;    // encoder counts/loop below this = stall
constexpr int   STALL_CONFIRM_LOOPS      = 50;   // consecutive slow loops before fault

// Motor indices
constexpr int MOTOR_BIG_TOP    = 0;
constexpr int MOTOR_BIG_BOTTOM = 1;
constexpr int MOTOR_SMALL_LEFT = 2;
constexpr int MOTOR_SMALL_RIGHT= 3;

constexpr int   HOMING_SPEED             = 80;    // TODO: slow PWM value for homing (0–255)

// ── Motor state struct ───────────────────────────────────────────────────────
struct Motor {
    volatile long encoderCount = 0;  // volatile because modified by ISRs
    int   targetCount    = 0;
    float integralError  = 0.0f;
    int   prevError      = 0;
    int   stallCounter   = 0;
    bool  moveComplete   = true;
    bool  stallDetected  = false;
    bool  homingActive   = false;
    // Pin assignments
    int pwmPin, in1Pin, in2Pin, encAPin, encBPin;
    // PID gains (can be per motor if needed)
    float kp = 4.75f, ki = 0.25f, kd = 0.25f;
    unsigned long prevTime = 0;
};

class MotorController {
public:

    void init() {
        // Define pin assignments (example pins, adjust as needed)
        motors[MOTOR_BIG_TOP].pwmPin = 2; motors[MOTOR_BIG_TOP].in1Pin = 3; motors[MOTOR_BIG_TOP].in2Pin = 4; motors[MOTOR_BIG_TOP].encAPin = 5; motors[MOTOR_BIG_TOP].encBPin = 6;
        motors[MOTOR_BIG_BOTTOM].pwmPin = 7; motors[MOTOR_BIG_BOTTOM].in1Pin = 8; motors[MOTOR_BIG_BOTTOM].in2Pin = 9; motors[MOTOR_BIG_BOTTOM].encAPin = 10; motors[MOTOR_BIG_BOTTOM].encBPin = 11;
        motors[MOTOR_SMALL_LEFT].pwmPin = 12; motors[MOTOR_SMALL_LEFT].in1Pin = 13; motors[MOTOR_SMALL_LEFT].in2Pin = 14; motors[MOTOR_SMALL_LEFT].encAPin = 15; motors[MOTOR_SMALL_LEFT].encBPin = 16;
        motors[MOTOR_SMALL_RIGHT].pwmPin = 17; motors[MOTOR_SMALL_RIGHT].in1Pin = 18; motors[MOTOR_SMALL_RIGHT].in2Pin = 19; motors[MOTOR_SMALL_RIGHT].encAPin = 20; motors[MOTOR_SMALL_RIGHT].encBPin = 21;

        for (int i = 0; i < NUM_MOTORS; i++) {
            Motor& m = motors[i];
            pinMode(m.pwmPin, OUTPUT);
            pinMode(m.in1Pin, OUTPUT);
            pinMode(m.in2Pin, OUTPUT);
            pinMode(m.encAPin, INPUT);
            pinMode(m.encBPin, INPUT);
            analogWrite(m.pwmPin, 0);
            // Attach interrupts
            // Note: In real implementation, define ISR functions
            // attachInterrupt(digitalPinToInterrupt(m.encAPin), encoderISR_A[i], CHANGE);
            // attachInterrupt(digitalPinToInterrupt(m.encBPin), encoderISR_B[i], CHANGE);
            m.prevTime = millis();
        }
    }

    // ── Called every loop() ─────────────────────────────────────────────────
    void update() {
        for (int i = 0; i < NUM_MOTORS; i++) {
            Motor& m = motors[i];
            if (m.moveComplete) continue;

            unsigned long now = millis();
            float dt = (now - m.prevTime) / 1000.0f;
            if (dt <= 0) continue;
            m.prevTime = now;

            int error = m.targetCount - m.encoderCount;

            if (abs(error) <= POSITION_DEADBAND) {
                setMotorOutput(m, 0);
                m.moveComplete = true;
                m.stallCounter = 0;
                m.integralError = 0;  // reset integral
                continue;
            }

            // PID calculation
            float proportional = m.kp * error;
            m.integralError += error * dt;
            m.integralError = constrain(m.integralError, -100.0f, 100.0f);  // prevent windup
            float integral = m.ki * m.integralError;
            float derivative = m.kd * (error - m.prevError) / dt;
            float output = proportional + integral + derivative;
            output = constrain(output, -255.0f, 255.0f);
            m.prevError = error;

            // Set direction and PWM
            setMotorOutput(m, (int)output);

            // Stall detection
            // Note: velocity calculation needs previous encoder count
            // For simplicity, skip or implement properly
            // m.stallCounter etc.
        }
    }

    // ── Commands from state machine ─────────────────────────────────────────

    void smallReset() {
        // Drive small pinion inward to SMALL_MIN_MM (4" hard limit).
        // This is target = 0 counts (home position, zeroed after homing sequence).
        // Called at the start of every fold cycle before big folder motion.
        Serial.println("SMALL_RESET");
        motors[MOTOR_SMALL_LEFT].targetCount   = 0;
        motors[MOTOR_SMALL_RIGHT].targetCount  = 0;
        motors[MOTOR_SMALL_LEFT].moveComplete  = false;
        motors[MOTOR_SMALL_RIGHT].moveComplete = false;
        motors[MOTOR_SMALL_LEFT].stallCounter  = 0;
        motors[MOTOR_SMALL_RIGHT].stallCounter = 0;
    }

    void startHoming() {
        // Drive all 4 motors inward (toward board center) until allAtHome() is true.
        // allAtHome() is determined by stop detection (black box).
        // Called once at power-on. Small folders will reset each cycle via smallReset().
        // Big folders hold position between cycles — no per-cycle re-home needed.
        Serial.println("HOMING_START");
        for (int i = 0; i < NUM_MOTORS; i++) {
            motors[i].homingActive = true;
            motors[i].moveComplete = false;
            // Set motor direction to inward (CCW) at a slow homing speed
            setMotorOutput(motors[i], -HOMING_SPEED);
        }
    }
        // Clamp posIndex defensively — RPi should have validated this already,
        // but a corrupted serial message could produce an out-of-range value.
        posIndex = constrain(posIndex, 0, BIG_MAX_INDEX);

        // Both big motors move symmetrically (mirrored):
        //   MOTOR_BIG_TOP    target = +posIndex * BIG_INCREMENT_COUNTS (outward = away from center)
        //   MOTOR_BIG_BOTTOM target = +posIndex * BIG_INCREMENT_COUNTS (same magnitude, opposite physical direction)
        //   Sign convention: confirm with mechanical team which motor direction is "outward"
        int target = posIndex * BIG_INCREMENT_COUNTS;
        Serial.println("BIG: " + String(target));
        motors[MOTOR_BIG_TOP].targetCount    = target;
        motors[MOTOR_BIG_BOTTOM].targetCount = target;   // TODO: negate if motors are wired opposing
        motors[MOTOR_BIG_TOP].moveComplete    = false;
        motors[MOTOR_BIG_BOTTOM].moveComplete = false;
        motors[MOTOR_BIG_TOP].stallCounter    = 0;
        motors[MOTOR_BIG_BOTTOM].stallCounter = 0;
    }

    void shiftSmall(int encoderCounts) {
        // Clamp defensively to valid small folder range
        encoderCounts = constrain(encoderCounts, SMALL_MIN_COUNTS, SMALL_MAX_COUNTS);
        Serial.println("SMALL: " + String(encoderCounts));
        // Both small motors move symmetrically (mirrored):
        //   MOTOR_SMALL_LEFT  target = +encoderCounts (outward = rightward)
        //   MOTOR_SMALL_RIGHT target = +encoderCounts (outward = leftward, so same magnitude)
        //   TODO: confirm sign convention with mechanical team
        motors[MOTOR_SMALL_LEFT].targetCount   = encoderCounts;
        motors[MOTOR_SMALL_RIGHT].targetCount  = encoderCounts;   // TODO: negate if opposing
        motors[MOTOR_SMALL_LEFT].moveComplete  = false;
        motors[MOTOR_SMALL_RIGHT].moveComplete = false;
        motors[MOTOR_SMALL_LEFT].stallCounter  = 0;
        motors[MOTOR_SMALL_RIGHT].stallCounter = 0;
    }

    // ── Status queries ───────────────────────────────────────────────────────

    bool allAtHome() {
        // Returns true if all encoders are at or near 0
        // Used during homing to detect completion
        // "Near 0" = within POSITION_DEADBAND counts
        return false; // placeholder
    }

    bool currentMoveComplete() {
        // Returns true if all motors that were commanded have reached their targets
        return motors[0].moveComplete &&
               motors[1].moveComplete &&
               motors[2].moveComplete &&
               motors[3].moveComplete;
    }

    bool stallDetected() {
        // Returns true if any motor has flagged a stall
        for (int i = 0; i < NUM_MOTORS; i++) {
            if (motors[i].stallDetected) return true;
        }
        return false;
    }

    String stalledMotorName() {
        // Returns human-readable name of first stalled motor for WARNING message
        // e.g. "BIG_TOP", "SMALL_LEFT"
        return "";  // placeholder
    }

    void zeroEncoders() {
        // Set all encoderCount to 0 — called after homing completes
        Serial.println("ZERO_ENCODERS");
        for (int i = 0; i < NUM_MOTORS; i++) {
            motors[i].encoderCount  = 0;
            motors[i].targetCount   = 0;
            motors[i].moveComplete  = true;
        }
    }

private:
    Motor motors[NUM_MOTORS];

    void setMotorOutput(Motor& m, int output) {
        // output in [-255, 255]
        // Positive → CW (outward), negative → CCW (inward)
        // Set direction and PWM
        int pwm = abs(output);
        bool forward = output > 0;
        digitalWrite(m.in1Pin, forward ? HIGH : LOW);
        digitalWrite(m.in2Pin, forward ? LOW : HIGH);
        analogWrite(m.pwmPin, pwm);
    }

    void handleEncA(int motorIndex) {
        Motor& m = motors[motorIndex];
        bool a = digitalRead(m.encAPin);
        bool b = digitalRead(m.encBPin);
        if (a == b) m.encoderCount++; else m.encoderCount--;
    }

    void handleEncB(int motorIndex) {
        Motor& m = motors[motorIndex];
        bool a = digitalRead(m.encAPin);
        bool b = digitalRead(m.encBPin);
        if (a != b) m.encoderCount++; else m.encoderCount--;
    }

    // ── Encoder ISRs (one per motor, attached in init()) ─────────────────────
    // Each ISR reads the A and B encoder pins to determine direction,
    // then increments or decrements the motor's encoderCount.
    // Example for motor 0:
    //
    // void FASTRUN encoderISR_BigTop() {
    //     bool a = digitalReadFast(ENC_BIG_TOP_A);
    //     bool b = digitalReadFast(ENC_BIG_TOP_B);
    //     motors[MOTOR_BIG_TOP].encoderCount += (a == b) ? +1 : -1;
    // }
};
