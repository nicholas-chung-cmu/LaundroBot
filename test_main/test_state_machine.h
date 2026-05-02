// state_machine.h
// Teensy 4.1 — System State Machine
// Responsible for: tracking robot phase, running the fold sequence internally,
//                  detecting faults, sending READY/DONE/WARNING to RPi
// Runs on: Teensy 4.1
// Depends on: fold_library.h, actuator_interfacer.h,
//             sensor_handler.h, serial_handler.h

#pragma once
#include <Arduino.h>

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

    GarmentType  detectedGarment = GARMENT_SHIRT;
    float        centerX_mm      = 0.0f;
    float        centerY_mm      = 0.0f;

    void init(ActuatorInterfacer* a, SensorHandler* s) {
        actuators = a; // Save the permanent pointer
        sensors = s;
    }

    // ── Called every loop() ───────────────────────────────────────────────────
    void update() {
        switch (currentState) {

        case STATE_HOMING:
            if (actuators->allAtHome()) {
                actuators->zeroEncoders();
                transition(STATE_SCAN);
            } else if (elapsed() > HOMING_TIMEOUT_MS) {
                transition(STATE_WARNING, "HOMING_TIMEOUT");
            }
            break;

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
                Serial.println("step done");
                stepIndex++;
                if (stepIndex >= sequence.count) {
                    transition(STATE_IDLE);
                } else {
                    executeStep(sequence.steps[stepIndex]);
                    stateStartMs = millis();   // reset timeout for new step
                }
            } 
            else if (elapsed() > MOVE_TIMEOUT_MS) {
                Serial.println("move timeout");
                transition(STATE_WARNING, "MOVE_TIMEOUT");
            } 
            // else if (actuators->stallDetected()) {
            //     char reason[32];
            //     snprintf(reason, sizeof(reason), "STALL_%s", actuators->stalledMotorName());
            //     transition(STATE_WARNING, reason);
            // }
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
        if (currentState != STATE_READY) {
            Serial.println("canceling start");
            return;
        }
        Serial.println("start build sequence");
        sequence = buildSequence(detectedGarment, size, centerX_mm, centerY_mm);

        if (!sequence.valid) {
            transition(STATE_WARNING, sequence.errorReason);
            Serial.println("sequence invalid");
            return;
        }

        stepIndex    = 0;
        currentState = STATE_FOLD_SEQ;
        stateStartMs = millis();
        executeStep(sequence.steps[0]);
        Serial.println("step 0 done");
    }

    // ── Called by serial_handler when "RESET" received ────────────────────────
    void onResetCommand() {
        actuators->stopAll();
        // Re-scan without re-homing — big folders hold position between cycles,
        // small folders will reset at the start of the next fold sequence.
        transition(STATE_SCAN);
    }

private:
    State        currentState    = STATE_READY;
    FoldSequence sequence;
    int          stepIndex       = 0;
    unsigned long stateStartMs   = 0;

    ActuatorInterfacer* actuators;
    SensorHandler*   sensors;

    // Timeouts — TODO: tune to ~2× worst-case move duration
    static constexpr unsigned long HOMING_TIMEOUT_MS = 10000;
    static constexpr unsigned long MOVE_TIMEOUT_MS   = 5000;

    unsigned long elapsed() { return millis() - stateStartMs; }

    void transition(State next, const char* warningReason = nullptr) {
        currentState = next;
        stateStartMs = millis();

        switch (next) {
        // case STATE_HOMING:
        //     actuators->startHoming();   // drives all motors inward
        //     break;

        case STATE_READY:
            // serial_handler::send("READY");
            Serial.println("ready for next input");
            break;

        case STATE_IDLE:
            // serial_handler::send("DONE");
            // After DONE, immediately re-scan for next garment.
            // Big folders stay where they are (no re-home needed).
            // Small folders will reset at the top of the next sequence.
            detectedGarment = GARMENT_UNKNOWN;
            transition(STATE_SCAN);
            break;

        case STATE_WARNING:
            actuators->stopAll();
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
        actuators->executeStep(step);
    }

    bool currentStepComplete() {
        return actuators->currentMoveComplete();
    }
};
