// servo_controller.h / servo_controller.cpp
// Teensy 4.1 — Servo Controller
// Responsible for: controlling all 8 fold/unfold servos,
//                  executing fold and unfold commands as mirrored pairs
// Runs on: Teensy 4.1
//
// Hardware layout:
//   Big folder subsystem    — 4 servos (2 top, 2 bottom), driven as one mirrored pair
//   Small folder subsystem  — 4 servos (2 left, 2 right), driven as one mirrored pair
//
// Within each folder, the two servos move together simultaneously.
// "Fold"   → servos rotate from REST angle to FOLD angle
// "Unfold" → servos rotate from FOLD angle back to REST angle

#pragma once
#include <Servo.h>

// ── Subsystem identifiers ────────────────────────────────────────────────────
enum Subsystem { SUBSYSTEM_BIG, SUBSYSTEM_SMALL };

// ── Servo angle constants (TODO: tune all after physical assembly) ───────────
// Big folder geometry
constexpr int BIG_REST_ANGLE   = 0;    // TODO: degrees, servos at rest (pre-fold)
constexpr int BIG_FOLD_ANGLE   = 90;   // TODO: degrees, servos at full fold

// Small folder geometry (may differ from big due to different arm length/mount)
constexpr int SMALL_REST_ANGLE = 0;    // TODO
constexpr int SMALL_FOLD_ANGLE = 90;   // TODO

// Servo sweep speed — degrees per update tick
// Lower = slower/gentler fold; higher = faster
constexpr int SERVO_SWEEP_SPEED = 2;   // TODO: tune

// ── Servo indices ────────────────────────────────────────────────────────────
constexpr int SERVO_BIG_TOP_A    = 0;
constexpr int SERVO_BIG_TOP_B    = 1;
constexpr int SERVO_BIG_BOT_A   = 2;
constexpr int SERVO_BIG_BOT_B   = 3;
constexpr int SERVO_SMALL_LEFT_A = 4;
constexpr int SERVO_SMALL_LEFT_B = 5;
constexpr int SERVO_SMALL_RIGHT_A= 6;
constexpr int SERVO_SMALL_RIGHT_B= 7;

constexpr int NUM_SERVOS = 8;

class ServoController {
public:

    void init() {
        // Attach all 8 Servo objects to their PWM pins
        // TODO: assign pin numbers to each servo index
        // Move all servos to REST angle immediately
        for (int i = 0; i < NUM_SERVOS; i++) {
            // servos[i].attach(SERVO_PINS[i]);
            // servos[i].write(restAngleFor(i));
        }
    }

    // ── Commands from state machine ─────────────────────────────────────────

    void fold(Subsystem sub) {
        // Begin sweeping the relevant servo group from rest to fold angle.
        // Both folders in the subsystem move together (e.g. top AND bottom for BIG).
        // Movement is non-blocking — actual angle advances each update() call.
        //
        // Set targetAngle[group] = FOLD_ANGLE for subsystem
        // Set movingToFold[sub] = true
        // Set moveComplete[sub] = false
    }

    void unfold(Subsystem sub) {
        // Same as fold() but target = REST_ANGLE
        // Set movingToFold[sub] = false
        // Set moveComplete[sub] = false
    }

    // ── Called every loop() ─────────────────────────────────────────────────
    void update() {
        // For each subsystem (BIG, SMALL):
        //   if moveComplete[sub]: skip
        //
        //   Advance currentAngle toward targetAngle by SERVO_SWEEP_SPEED degrees
        //   Write new angle to both servos in the subsystem pair
        //   If currentAngle == targetAngle: moveComplete[sub] = true
        //
        // Note: mirrored pairs may need one servo to move in the opposite direction
        //   (e.g. top folder sweeps +degrees, bottom folder sweeps -degrees)
        //   Handle this by negating the angle delta for the mirrored member.
        //   TODO: confirm sign convention with mechanical team.
    }

    bool currentMoveComplete(Subsystem sub) {
        // Returns true if the commanded subsystem has reached its target angle
        return moveComplete[(int)sub];
    }

    bool allMovesComplete() {
        return moveComplete[SUBSYSTEM_BIG] && moveComplete[SUBSYSTEM_SMALL];
    }

private:
    Servo servos[NUM_SERVOS];
    int   currentAngle[2] = {BIG_REST_ANGLE, SMALL_REST_ANGLE};
    int   targetAngle[2]  = {BIG_REST_ANGLE, SMALL_REST_ANGLE};
    bool  moveComplete[2] = {true, true};
    bool  movingToFold[2] = {false, false};

    int restAngleFor(int servoIndex) {
        // Return BIG_REST_ANGLE for servos 0-3, SMALL_REST_ANGLE for 4-7
        return (servoIndex < 4) ? BIG_REST_ANGLE : SMALL_REST_ANGLE;
    }
};
