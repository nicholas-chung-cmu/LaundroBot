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

constexpr float BIG_MIN_MM        = 4.0f * INCHES_TO_MM;   // 101.6 mm
constexpr float BIG_MAX_MM        = 8.0f * INCHES_TO_MM;   // 203.2 mm
constexpr float BIG_RANGE_MM      = BIG_MAX_MM - BIG_MIN_MM; // 101.6 mm of travel

constexpr float SMALL_MIN_MM      = 4.0f * INCHES_TO_MM;   // 101.6 mm
constexpr float SMALL_MAX_MM      = 9.0f * INCHES_TO_MM;   // 228.6 mm

// ── Calibration Constants (TODO: fill in after hardware measurement) ─────────
constexpr int   NUM_MOTORS           = 4;
constexpr float BIG_COUNTS_PER_MM    = 0.0f;  // TODO
constexpr float SMALL_COUNTS_PER_MM  = 0.0f;  // TODO
constexpr int   BIG_INCREMENT_COUNTS = 0;      // TODO: counts per discrete slot
                                               //   = BIG_INCREMENT_MM * BIG_COUNTS_PER_MM
constexpr int   BIG_MAX_INDEX        = 0;      // TODO: set after increment is known
                                               //   = (int)(BIG_RANGE_MM / BIG_INCREMENT_MM)

// Derived range limits in encoder counts (computed from physical constants above)
// Used for bounds-checking commands before executing them.
// Both big and small folder counts are offsets from their respective home (zeroed) positions.
constexpr int   BIG_MAX_COUNTS   = 0;  // TODO: BIG_MAX_INDEX * BIG_INCREMENT_COUNTS
constexpr int   SMALL_MIN_COUNTS = 0;  // TODO: always 0 (home = SMALL_MIN_MM after homing)
constexpr int   SMALL_MAX_COUNTS = 0;  // TODO: (SMALL_MAX_MM - SMALL_MIN_MM) * SMALL_COUNTS_PER_MM
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

constexpr int   HOMING_SPEED             = 80;    // TODO: slow PWM value for homing (0–255)

// ── Motor state struct ───────────────────────────────────────────────────────
struct Motor {
    int   encoderCount   = 0;
    int   targetCount    = 0;
    float integralError  = 0.0f;
    int   prevError      = 0;
    int   stallCounter   = 0;
    bool  moveComplete   = true;
    bool  stallDetected  = false;
    bool  homingActive   = false;
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

    void smallReset() {
        // Drive small pinion inward to SMALL_MIN_MM (4" hard limit).
        // This is target = 0 counts (home position, zeroed after homing sequence).
        // Called at the start of every fold cycle before big folder motion.
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
        for (int i = 0; i < NUM_MOTORS; i++) {
            motors[i].homingActive = true;
            motors[i].moveComplete = false;
            // Set motor direction to inward (CCW) at a slow homing speed
            // setMotorOutput(motors[i], -HOMING_SPEED);
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
