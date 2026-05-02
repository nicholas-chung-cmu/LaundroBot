// servo_controller.h
// Teensy 4.1 — Servo Controller
// Responsible for: controlling all 8 fold/unfold servos individually
// Runs on: Teensy 4.1
//
// Hardware layout:
//   Big folder subsystem    — 4 servos: 2 top (A+B), 2 bottom (A+B)
//   Small folder subsystem  — 4 servos: 2 left (A+B), 2 right (A+B)
//
// Each folder pair (e.g. top A + top B) always moves together.
// Mirrored partners within a pair may need opposite sign — confirm with mechanical team.
// "Fold"   → servos sweep from REST to FOLD angle
// "Unfold" → servos sweep from FOLD back to REST angle
// All movement is non-blocking; angles advance each update() call.

#pragma once
#include <PWMServo.h>

// ── Servo angle constants (TODO: tune all after physical assembly) ───────────
// Big folder geometry
constexpr int BIG_REST_ANGLE   = 0;    // TODO: degrees, servos at rest (pre-fold)
constexpr int BIG_FOLD_ANGLE   = 120;   // TODO: degrees, servos at full fold

// Small folder geometry (may differ from big due to different arm length/mount)
constexpr int SMALL_REST_ANGLE = 0;    // TODO
constexpr int SMALL_FOLD_ANGLE = 150;   // TODO

// Servo sweep pacing
// SWEEP_SPEED is how many degrees to move per update tick
// UPDATE_INTERVAL is how many milliseconds to wait between ticks
constexpr int SERVO_SWEEP_SPEED = 3;   // Degrees per step
constexpr int SERVO_UPDATE_INTERVAL = 15; // ms between steps (controls smoothness)

// ── Servo indices ────────────────────────────────────────────────────────────
constexpr int SERVO_BIG_TOP_A    = 0;
constexpr int SERVO_BIG_TOP_B    = 1;
constexpr int SERVO_BIG_BOT_A    = 2;
constexpr int SERVO_BIG_BOT_B    = 3;
constexpr int SERVO_SMALL_LEFT_A = 4;
constexpr int SERVO_SMALL_LEFT_B = 5;
constexpr int SERVO_SMALL_RIGHT_A= 6;
constexpr int SERVO_SMALL_RIGHT_B= 7;

constexpr int NUM_ROTATION_SERVOS = 8;
constexpr int rPins[NUM_ROTATION_SERVOS] = {7, 8, 9, 10, 2, 3, 4, 5};

class RotationalController {
public:

    // Pass in your array of 8 pins from the main sketch
    // Order: [BIG_TOP_A, BIG_TOP_B, BIG_BOT_A, BIG_BOT_B, SMALL_LEFT_A, SMALL_LEFT_B, SMALL_RIGHT_A, SMALL_RIGHT_B]
    // Example: {2, 3, 4, 5, 6, 7, 8, 9} for pins 2-9
    void init() {
        for (int i = 0; i < NUM_ROTATION_SERVOS; i++) {
            int rest        = restAngleFor(i);
            currentAngle[i] = rest;
            targetAngle[i]  = rest;
            moveComplete[i] = true;
            
            servos[i].attach(rPins[i]);
            servos[i].write(physicalAngleFor(i, rest));
        }
        lastUpdateTime = millis();
    }

    // ── Commands from state machine ───────────────────────────────────────────
    // Each folder is actuated individually — only the commanded servo sweeps.
    // Movement is non-blocking; actual angle advances each update() call.

    void foldBottom()  { startMove(SERVO_BIG_BOT_A,    BIG_FOLD_ANGLE);
                         startMove(SERVO_BIG_BOT_B,    BIG_FOLD_ANGLE);  }

    void foldTop()     { startMove(SERVO_BIG_TOP_A,    BIG_FOLD_ANGLE);
                         startMove(SERVO_BIG_TOP_B,    BIG_FOLD_ANGLE);  }

    void foldLeft()    { startMove(SERVO_SMALL_LEFT_A,  SMALL_FOLD_ANGLE);
                         startMove(SERVO_SMALL_LEFT_B,  SMALL_FOLD_ANGLE); }

    void foldRight()   { startMove(SERVO_SMALL_RIGHT_A, SMALL_FOLD_ANGLE);
                         startMove(SERVO_SMALL_RIGHT_B, SMALL_FOLD_ANGLE); }

    void unfoldTop()   { startMove(SERVO_BIG_TOP_A,    BIG_REST_ANGLE);
                         startMove(SERVO_BIG_TOP_B,    BIG_REST_ANGLE);  }

    void unfoldBottom() { startMove(SERVO_BIG_BOT_A,    BIG_REST_ANGLE);
                          startMove(SERVO_BIG_BOT_B,    BIG_REST_ANGLE);  }

    void unfoldLeft()  { startMove(SERVO_SMALL_LEFT_A,  SMALL_REST_ANGLE);
                         startMove(SERVO_SMALL_LEFT_B,  SMALL_REST_ANGLE); }

    void unfoldRight() { startMove(SERVO_SMALL_RIGHT_A, SMALL_REST_ANGLE);
                         startMove(SERVO_SMALL_RIGHT_B, SMALL_REST_ANGLE); }

    // Convenience functions to unfold all servos in a subsystem
    void unfoldBig()   { unfoldTop(); unfoldBottom(); }

    void unfoldSmall() { unfoldLeft(); unfoldRight(); }

    void stopAll()     { for (int i = 0; i < NUM_ROTATION_SERVOS; i++) {
                             targetAngle[i]  = currentAngle[i];
                             moveComplete[i] = true; } }

    // ── Called every loop() ──────────────────────────────────────────────────
    void update() {
        unsigned long now = millis();
        
        // Non-blocking timer: only move servos if the interval has passed
        if (now - lastUpdateTime >= SERVO_UPDATE_INTERVAL) {
            lastUpdateTime = now;

            // For each servo, advance currentAngle toward targetAngle by SERVO_SWEEP_SPEED.
            // Note: mirrored servos within a pair may need opposite sign — one sweeps
            // +degrees while its partner sweeps -degrees to produce a symmetric fold.
            // TODO: confirm sign convention with mechanical team and negate as needed.
            for (int i = 0; i < NUM_ROTATION_SERVOS; i++) {
                if (moveComplete[i]) continue;
                
                int delta = targetAngle[i] - currentAngle[i];
                if (abs(delta) <= SERVO_SWEEP_SPEED) {
                    currentAngle[i] = targetAngle[i];
                    moveComplete[i] = true;
                } else {
                    currentAngle[i] += (delta > 0) ? SERVO_SWEEP_SPEED : -SERVO_SWEEP_SPEED;
                }
                
                // Physically update the servo using mirrored motion for B-side servos
                servos[i].write(physicalAngleFor(i, currentAngle[i]));
                //Serial.println("rservo: " + String(currentAngle[i]));
            }
        }
    }

    bool allMovesComplete() {
        for (int i = 0; i < NUM_ROTATION_SERVOS; i++) {
            if (!moveComplete[i]) return false;
        }
        return true;
    }

private:
    PWMServo servos[NUM_ROTATION_SERVOS];
    int   currentAngle[NUM_ROTATION_SERVOS];
    int   targetAngle[NUM_ROTATION_SERVOS];
    bool  moveComplete[NUM_ROTATION_SERVOS];
    
    unsigned long lastUpdateTime; // Tracks timing for the sweep steps

    void startMove(int servoIndex, int angle) {
        targetAngle[servoIndex]  = angle;
        moveComplete[servoIndex] = false;
    }

    int restAngleFor(int i) {
        return (i < 4) ? BIG_REST_ANGLE : SMALL_REST_ANGLE;
    }

    // ── Mirror mapping for B-side servos ──────────────────────────────────────
    // B-side servos are mechanically mirrored relative to A-side servos.
    // To achieve symmetric folding, B servos move in the opposite physical direction.
    // Example: A servo at logical 0° → physical 0°, B servo at logical 0° → physical 180°.
    // Both reach the same folder angle (e.g., 90°) but from opposite ends.
    // This ensures paired servos (A+B) fold symmetrically at the same speed.

    bool isMirroredServo(int i) const {
        return i == SERVO_BIG_TOP_B || i == SERVO_BIG_BOT_B ||
               i == SERVO_SMALL_LEFT_B || i == SERVO_SMALL_RIGHT_B;
    }

    int physicalAngleFor(int i, int logicalAngle) const {
        return isMirroredServo(i) ? 180 - logicalAngle : logicalAngle;
    }
};