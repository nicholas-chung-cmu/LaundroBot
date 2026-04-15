// state_machine.h / state_machine.cpp
// Teensy 4.1 — System State Machine
// Responsible for: tracking robot phase, orchestrating transitions,
//                  detecting completion and faults, sending status to RPi
// Runs on: Teensy 4.1
// Depends on: motor_controller.h, servo_controller.h, sensor_handler.h,
//             serial_handler.h (for outbound messages)

#pragma once
#include "motor_controller.h"
#include "servo_controller.h"
#include "sensor_handler.h"

// ── States ────────────────────────────────────────────────────────────────────
enum State {
    STATE_IDLE,         // waiting, nothing active
    STATE_HOMING,       // driving all DC motors inward to establish zero
    STATE_SCAN,         // reading sensor array, computing garment type + offset
    STATE_READY,        // scan done, "READY" sent to RPi, awaiting sequence start
    STATE_FOLD_SEQ,     // executing fold commands from RPi one at a time
    STATE_DONE,         // sequence complete, "DONE" sent, transitioning to IDLE
    STATE_WARNING       // fault detected, "WARNING:reason" sent, awaiting RESET
};

class StateMachine {
public:

    void init(MotorController*, ServoController*, SensorHandler*);

    // ── Called each loop() ───────────────────────────────────────────────────
    void update() {
        // Switch on currentState:

        // STATE_HOMING:
        //   if motorController->allAtHome():
        //     zero all encoders
        //     transition(STATE_SCAN)
        //   else if homingTimeout exceeded:
        //     transition(STATE_WARNING, "HOMING_TIMEOUT")

        // STATE_SCAN:
        //   sensorHandler->runScan()
        //   garment = sensorHandler->getGarmentType()   // "SHIRT" or "PANTS"
        //   offset  = sensorHandler->getOffset()        // {x_mm, y_mm}
        //   send "GARMENT:<garment>" to RPi
        //   send "OFFSET:<x>,<y>" to RPi
        //   transition(STATE_READY)

        // STATE_FOLD_SEQ:
        //   if motorController->currentMoveComplete() && servoController->currentMoveComplete():
        //     send "ACK" to RPi
        //     (RPi will send next command, or DONE if sequence finished)
        //   else if moveTimeout exceeded:
        //     transition(STATE_WARNING, "MOVE_TIMEOUT")
        //   else if motorController->stallDetected():
        //     transition(STATE_WARNING, "STALL_<subsystem>_<side>")

        // STATE_DONE:
        //   send "DONE" to RPi
        //   transition(STATE_IDLE)

        // STATE_WARNING:
        //   (do nothing — wait for RESET command from RPi via serial_handler)

        // STATE_IDLE, STATE_READY:
        //   (do nothing — waiting for external trigger)
    }

    // ── Called by serial_handler when command arrives ─────────────────────────
    void onCommand(const String& cmd) {
        // Parse cmd string and dispatch:

        // "SHIFT_BIG <pos_index>"   → motorController->shiftBig(pos_index)
        //                             start move timeout timer
        // "SHIFT_SMALL <counts>"   → motorController->shiftSmall(counts)
        //                             start move timeout timer
        // "FOLD_BIG"               → servoController->fold(SUBSYSTEM_BIG)
        //                             start move timeout timer
        // "FOLD_SMALL"             → servoController->fold(SUBSYSTEM_SMALL)
        // "UNFOLD_BIG"             → servoController->unfold(SUBSYSTEM_BIG)
        // "UNFOLD_SMALL"           → servoController->unfold(SUBSYSTEM_SMALL)
        // "RESET"                  → transition(STATE_HOMING)  (full restart)

        // If currentState != STATE_READY and != STATE_FOLD_SEQ:
        //   ignore command (or send WARNING:UNEXPECTED_CMD)
    }

    void transition(State next, const String& warningReason = "");

private:
    State currentState = STATE_IDLE;
    unsigned long moveStartTime = 0;
    unsigned long MOVE_TIMEOUT_MS = 5000;   // TODO: tune per motor speed

    MotorController* motors;
    ServoController* servos;
    SensorHandler*   sensors;
};
