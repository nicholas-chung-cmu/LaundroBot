// state_machine.h
// Teensy 4.1 — System State Machine
// Responsible for: tracking robot phase, running the fold sequence internally,
//                  detecting faults, sending READY/DONE/WARNING to RPi
// Runs on: Teensy 4.1
// Depends on: fold_library.h, motor_controller.h, servo_controller.h,
//             sensor_handler.h, serial_handler.h

#pragma once
#include <Arduino.h>
#include "sequencer.h"
#include "motor_controller.h"
#include "servo_controller.h"
#include "sensor_handler.h"

// ─── States ───────────────────────────────────────────────────────────────────

enum State {
    STATE_IDLE,       // startup only — transitions immediately to HOMING
    STATE_HOMING,     // big folders drive to BIG_MIN_MM, small folders drive to SMALL_MIN_MM
                      // encoders zeroed when done. Only runs once at power-on.
    STATE_SCAN,       // read sensor array — garment type and centerpoint
    STATE_READY,      // scan done, "READY" sent to RPi, waiting for START:<size>
    STATE_FOLD_SEQ,   // executing fold sequence step by step (fully internal)
    STATE_WARNING     // fault or invalid placement — "WARNING:<reason>" sent, waiting for RESET
};

// ─── State Machine ────────────────────────────────────────────────────────────

class StateMachine {
public:

    void init(MotorController* m, ServoController* s, SensorHandler* sens) {
        motors  = m;
        servos  = s;
        sensors = sens;
    }

    // ── Called every loop() ───────────────────────────────────────────────────
    void update() {
        switch (currentState) {

        case STATE_SCAN:
            // Synchronous single mux sweep — completes within one loop() call.
            sensors->runScan();
            detectedGarment = sensors->getGarmentType();
            centerX_mm      = sensors->getCenterX_mm();
            centerY_mm      = sensors->getCenterY_mm();
            transition(STATE_READY);
            break;

        case STATE_READY:
            // Waiting for onStartCommand(). Nothing to poll.
            break;

        case STATE_FOLD_SEQ:
            if (currentStepComplete()) {
                stepIndex++;
                if (stepIndex >= sequence.count) {
                    transition(STATE_DONE);
                } else {
                    executeStep(sequence.steps[stepIndex]);
                    stateStartMs = millis();   // reset timeout for new step
                }
            } else if (elapsed() > MOVE_TIMEOUT_MS) {
                transition(STATE_WARNING, "MOVE_TIMEOUT");
            } else if (motors->stallDetected()) {
                char reason[32];
                snprintf(reason, sizeof(reason), "STALL_%s", motors->stalledMotorName());
                transition(STATE_WARNING, reason);
            }
            break;

        case STATE_WARNING:
            // Waiting for onResetCommand(). No motion.
            break;

        case STATE_IDLE:
        default:
            break;
        }
    }

    // ── Called by serial_handler when "START:<size>" received ─────────────────
    void onStartCommand(FoldSize size) {
        if (currentState != STATE_READY) return;

        sequence = buildSequence(detectedGarment, size, centerX_mm, centerY_mm);

        if (!sequence.valid) {
            transition(STATE_WARNING, sequence.errorReason);
            return;
        }

        stepIndex    = 0;
        currentState = STATE_FOLD_SEQ;
        stateStartMs = millis();
        executeStep(sequence.steps[0]);
    }

    // ── Called by serial_handler when "RESET" received ────────────────────────
    void onResetCommand() {
        motors->stopAll();
        servos->stopAll();
        // Re-scan without re-homing — big folders hold position between cycles,
        // small folders will reset at the start of the next fold sequence.
        transition(STATE_SCAN);
    }

private:
    State        currentState    = STATE_IDLE;
    FoldSequence sequence;
    int          stepIndex       = 0;
    unsigned long stateStartMs   = 0;

    GarmentType  detectedGarment = GARMENT_UNKNOWN;
    float        centerX_mm      = 0.0f;
    float        centerY_mm      = 0.0f;

    MotorController* motors;
    ServoController* servos;
    SensorHandler*   sensors;

    // Timeouts — TODO: tune to ~2× worst-case move duration
    static constexpr unsigned long HOMING_TIMEOUT_MS = 10000;
    static constexpr unsigned long MOVE_TIMEOUT_MS   = 5000;

    unsigned long elapsed() { return millis() - stateStartMs; }

    void transition(State next, const char* warningReason = nullptr) {
        currentState = next;
        stateStartMs = millis();

        switch (next) {
        case STATE_HOMING:
            motors->startHoming();   // drives all motors inward
            break;

        case STATE_READY:
            // serial_handler::send("READY");
            break;

        case STATE_DONE:
            // serial_handler::send("DONE");
            // After DONE, immediately re-scan for next garment.
            // Big folders stay where they are (no re-home needed).
            // Small folders will reset at the top of the next sequence.
            detectedGarment = GARMENT_UNKNOWN;
            transition(STATE_SCAN);
            break;

        case STATE_WARNING:
            motors->stopAll();
            servos->stopAll();
            if (warningReason) {
                // char buf[64];
                // snprintf(buf, sizeof(buf), "WARNING:%s", warningReason);
                // serial_handler::send(buf);
            }
            break;

        default:
            break;
        }
    }

    void executeStep(const FoldStep& step) {
        switch (step.type) {

        case STEP_SMALL_RESET:
            // Drive small pinion inward to SMALL_MIN_MM (4" hard limit).
            // This clears the path for big folder movement and happens every cycle.
            motors->smallReset();
            break;

        case STEP_SHIFT_BIG:
            // Move big pinion to absolute slot index from home.
            // Both top and bottom folders move symmetrically.
            motors->shiftBig(step.arg);
            break;

        case STEP_SHIFT_SMALL:
            // Move small pinion to absolute encoder count from home.
            // Both left and right folders move symmetrically.
            motors->shiftSmall(step.arg);
            break;

        case STEP_FOLD_BIG_BOTTOM:
            servos->foldBottom();
            break;

        case STEP_FOLD_BIG_TOP:
            servos->foldTop();
            break;

        case STEP_FOLD_SMALL_LEFT:
            servos->foldLeft();
            break;

        case STEP_FOLD_SMALL_RIGHT:
            servos->foldRight();
            break;

        case STEP_UNFOLD_BIG:
            servos->unfoldBig();
            break;

        case STEP_UNFOLD_SMALL:
            servos->unfoldSmall();
            break;
        }
    }

    bool currentStepComplete() {
        return motors->currentMoveComplete() && servos->allMovesComplete();
    }
};
