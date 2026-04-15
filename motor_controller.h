// motor_controller.h / motor_controller.cpp
// Teensy 4.1 — DC Encoder Motor Controller
// Responsible for: PID position control for all 4 DC encoder motors,
//                  shift command execution, stall detection
// Runs on: Teensy 4.1
//
// Hardware layout:
//   Big folder subsystem    — 2 DC motors (top + bottom), mirrored pair
//   Small folder subsystem  — 2 DC motors (left + right), mirrored pair
//
//   CW  rotation → folders shift OUTWARD  from center
//   CCW rotation → folders shift INWARD   toward center
//
// Big folder motion:    DISCRETE — moves to a named position slot (index 0..N)
//                       Slot encoder count = index * BIG_INCREMENT_COUNTS
// Small folder motion:  CONTINUOUS — moves to an arbitrary encoder count target

#pragma once

// ── Constants (TODO: tune all after hardware calibration) ────────────────────
constexpr int NUM_MOTORS           = 4;
constexpr int BIG_INCREMENT_COUNTS = 0;  // encoder counts per discrete position step
constexpr int MAX_BIG_INDEX        = 3;  // highest valid position slot (0 = home)

// PID gains — same initial values for all motors; tune individually if needed
constexpr float KP = 0.0f;   // TODO
constexpr float KI = 0.0f;   // TODO
constexpr float KD = 0.0f;   // TODO

// Stall detection
constexpr int   STALL_VELOCITY_THRESHOLD = 5;    // encoder counts/loop below this = stall
constexpr int   STALL_CONFIRM_LOOPS      = 50;   // consecutive slow loops before fault

// Motor indices
constexpr int MOTOR_BIG_TOP    = 0;
constexpr int MOTOR_BIG_BOTTOM = 1;
constexpr int MOTOR_SMALL_LEFT = 2;
constexpr int MOTOR_SMALL_RIGHT= 3;

// ── Motor state struct ───────────────────────────────────────────────────────
struct Motor {
    int   encoderCount   = 0;
    int   targetCount    = 0;
    float integralError  = 0.0f;
    int   prevError      = 0;
    int   stallCounter   = 0;
    bool  moveComplete   = true;
    bool  stallDetected  = false;
    // TODO: add pin assignments (PWM pin, direction pin, encoder pins A/B)
};

class MotorController {
public:

    void init() {
        // For each motor:
        //   - attach encoder interrupt pins (CHANGE interrupt on both A and B)
        //   - configure PWM pin as output
        //   - configure direction pin as output
        //   - zero encoderCount, set moveComplete = true
    }

    // ── Called every loop() ─────────────────────────────────────────────────
    void update() {
        // For each motor in motors[]:
        //   if moveComplete: skip
        //
        //   error = targetCount - encoderCount
        //
        //   if abs(error) <= POSITION_DEADBAND:
        //     stop motor (PWM = 0)
        //     moveComplete = true
        //     stallCounter = 0
        //     continue
        //
        //   // PID calculation
        //   proportional = KP * error
        //   motor.integralError += error
        //   integral    = KI * motor.integralError
        //   derivative  = KD * (error - motor.prevError)
        //   output      = clamp(proportional + integral + derivative, -255, 255)
        //   motor.prevError = error
        //
        //   // Set direction and PWM
        //   setMotorOutput(motor, output)
        //
        //   // Stall detection
        //   velocity = abs(encoderCount - previousEncoderCount)
        //   if velocity < STALL_VELOCITY_THRESHOLD:
        //     motor.stallCounter++
        //     if motor.stallCounter >= STALL_CONFIRM_LOOPS:
        //       motor.stallDetected = true
        //       stop motor
        //   else:
        //     motor.stallCounter = 0
    }

    // ── Commands from state machine ─────────────────────────────────────────

    void shiftBig(int posIndex) {
        // Validate: clamp posIndex to [0, MAX_BIG_INDEX]
        // Compute target: posIndex * BIG_INCREMENT_COUNTS
        // Both big motors move symmetrically (mirrored):
        //   MOTOR_BIG_TOP    target = +computed_count  (outward = up)
        //   MOTOR_BIG_BOTTOM target = -computed_count  (outward = down, so negated)
        //   (or reverse sign convention — confirm with mechanical team)
        // Set moveComplete = false for both
        // Reset stall counters
    }

    void shiftSmall(int encoderCounts) {
        // Both small motors move symmetrically (mirrored):
        //   MOTOR_SMALL_LEFT  target = +encoderCounts
        //   MOTOR_SMALL_RIGHT target = -encoderCounts
        // Set moveComplete = false for both
        // Reset stall counters
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
        // Set direction pin HIGH/LOW based on sign of output
        // Write abs(output) to PWM pin
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
