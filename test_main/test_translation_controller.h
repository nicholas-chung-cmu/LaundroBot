// translation_servo_controller.h
// Teensy 4.1 — Folder Translation Servo Controller
// Responsible for: nonblocking control of 4 translation servos
// Runs on: Teensy 4.1
//
// Hardware layout:
//   Two servos drive the horizontal translation side
//   Two servos drive the vertical translation side
//
// Each side moves in the same physical direction for a target translation angle.
// All movement is nonblocking and advances via update() each loop cycle.

#pragma once
#include <PWMServo.h>
#include "test_sequencer.h"

constexpr int TRANSLATION_HORIZONTAL_A = 0;
constexpr int TRANSLATION_HORIZONTAL_B = 1;
constexpr int TRANSLATION_VERTICAL_A   = 2;
constexpr int TRANSLATION_VERTICAL_B   = 3;
constexpr int NUM_TRANSLATION_SERVOS   = 4;
constexpr int tPins[NUM_TRANSLATION_SERVOS] = {25, 29, 24, 28};

// Translation sweep pacing
constexpr int TRANSLATION_SWEEP_SPEED     = 3;   // degrees per update tick
constexpr int TRANSLATION_UPDATE_INTERVAL = 15;  // ms between ticks

// Big folder interval constants
constexpr float RADIUS_IN = 3.2;
constexpr float DEG_PER_MM = 360 / PI / RADIUS_IN / INCHES_TO_MM;
constexpr float DEG_PER_INCREMENT = BIG_INCREMENT_MM * DEG_PER_MM;

class TranslationalController {
public:
    // Pass in an array of 4 pins:
    // [HORIZONTAL_A, HORIZONTAL_B, VERTICAL_A, VERTICAL_B]
    void init() {
        for (int i = 0; i < NUM_TRANSLATION_SERVOS; i++) {
            currentAngle[i] = 0;
            targetAngle[i]  = 0;
            moveComplete[i] = true;
            servos[i].attach(tPins[i]);
            servos[i].write(currentAngle[i]);
        }
        lastUpdateTime = millis();
    }

    // Set a new target angle for the horizontal translation side.
    // Both horizontal servos will move together in the same direction.
    void setHorizontalTarget(int angle) {
        angle = constrain(angle, 0, 180);
        targetAngle[TRANSLATION_HORIZONTAL_A] = angle;
        targetAngle[TRANSLATION_HORIZONTAL_B] = angle;
        moveComplete[TRANSLATION_HORIZONTAL_A] = false;
        moveComplete[TRANSLATION_HORIZONTAL_B] = false;
    }

    // Set a new target angle for the vertical translation side.
    // Both vertical servos will move together in the same direction.
    void setVerticalTarget(int slot) {
        slot = constrain(slot, 0, BIG_MAX_INDEX);
        int angle = (int) (DEG_PER_INCREMENT * slot);
        targetAngle[TRANSLATION_VERTICAL_A] = angle;
        targetAngle[TRANSLATION_VERTICAL_B] = angle;
        moveComplete[TRANSLATION_VERTICAL_A] = false;
        moveComplete[TRANSLATION_VERTICAL_B] = false;
    }

    // Immediately stop both translation sides.
    void stopAll() {
        for (int i = 0; i < NUM_TRANSLATION_SERVOS; i++) {
            targetAngle[i] = currentAngle[i];
            moveComplete[i] = true;
        }
    }

    // Called every loop() to advance translation servo angles.
    void update() {
        unsigned long now = millis();
        if (now - lastUpdateTime < TRANSLATION_UPDATE_INTERVAL) return;
        lastUpdateTime = now;

        for (int i = 0; i < NUM_TRANSLATION_SERVOS; i++) {
            if (moveComplete[i]) continue;
            int delta = targetAngle[i] - currentAngle[i];
            if (abs(delta) <= TRANSLATION_SWEEP_SPEED) {
                currentAngle[i] = targetAngle[i];
                moveComplete[i] = true;
            } else {
                currentAngle[i] += (delta > 0) ? TRANSLATION_SWEEP_SPEED : -TRANSLATION_SWEEP_SPEED;
            }
            servos[i].write(currentAngle[i]);
            Serial.println("tservo: " + String(currentAngle[i]));
        }
    }

    bool horizontalMoveComplete() const {
        return moveComplete[TRANSLATION_HORIZONTAL_A] && moveComplete[TRANSLATION_HORIZONTAL_B];
    }

    bool verticalMoveComplete() const {
        return moveComplete[TRANSLATION_VERTICAL_A] && moveComplete[TRANSLATION_VERTICAL_B];
    }

    bool allMovesComplete() const {
        for (int i = 0; i < NUM_TRANSLATION_SERVOS; i++) {
            if (!moveComplete[i]) return false;
        }
        return true;
    }

    int horizontalCurrentAngle() const {
        return currentAngle[TRANSLATION_HORIZONTAL_A];
    }

    int verticalCurrentAngle() const {
        return currentAngle[TRANSLATION_VERTICAL_A];
    }

private:
    PWMServo servos[NUM_TRANSLATION_SERVOS];
    int currentAngle[NUM_TRANSLATION_SERVOS];
    int targetAngle[NUM_TRANSLATION_SERVOS];
    bool moveComplete[NUM_TRANSLATION_SERVOS];
    unsigned long lastUpdateTime;
};