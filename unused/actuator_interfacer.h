#pragma once
#include <Arduino.h>
#include "sequencer.h"

// Assume these headers provide your black-box driving functions
#include "motor_controller.h" 
#include "servo_controller.h" 

// ─── Servo angle constants (from servo_controller.h) ──────────────────────────
// Big folder geometry
constexpr int BIG_REST_ANGLE   = 0;    // degrees, servos at rest (pre-fold)
constexpr int BIG_FOLD_ANGLE   = 90;   // degrees, servos at full fold

// Small folder geometry (may differ from big due to different arm length/mount)
constexpr int SMALL_REST_ANGLE = 0;    // TODO
constexpr int SMALL_FOLD_ANGLE = 90;   // TODO

// ─── Hardware Pin Definitions (Placeholders) ──────────────────────────────────

// Motor identifiers (could be pins, I2C addresses, or simple ID numbers 
// depending on how motor_controller.h is written)
constexpr uint8_t ID_MOTOR_BIG   = 1;  
constexpr uint8_t ID_MOTOR_SMALL = 2; 

// Servo controllers (Folding) - 2 paired servos per fold
constexpr uint8_t PIN_SERVO_BIG_BOTTOM_1 = 3;
constexpr uint8_t PIN_SERVO_BIG_BOTTOM_2 = 4;

constexpr uint8_t PIN_SERVO_BIG_TOP_1    = 5;
constexpr uint8_t PIN_SERVO_BIG_TOP_2    = 6;

constexpr uint8_t PIN_SERVO_SMALL_LEFT_1 = 7;
constexpr uint8_t PIN_SERVO_SMALL_LEFT_2 = 8;

constexpr uint8_t PIN_SERVO_SMALL_RIGHT_1 = 9;
constexpr uint8_t PIN_SERVO_SMALL_RIGHT_2 = 10;

// ─── Actuator Interfacer Class ────────────────────────────────────────────────

class ActuatorInterfacer {
private:
    MotorController* motors;
    ServoController* servos;

    // State tracking to convert absolute sequencer targets into relative deltas
    int current_big_slot = 0;
    int current_small_counts = 0;

public:
    ActuatorInterfacer(MotorController* m, ServoController* s) : motors(m), servos(s) {}

    // Resets the state trackers. Call this after your physical homing sequence!
    void resetState() {
        current_big_slot = 0;
        current_small_counts = 0;
    }

    // Homing methods
    void startHoming() {
        motors->startHoming();
    }

    bool allAtHome() {
        return motors->allAtHome();
    }

    void zeroEncoders() {
        motors->zeroEncoders();
        resetState();
    }

    // Movement completion
    bool currentMoveComplete() {
        return motors->currentMoveComplete() && servos->allMovesComplete();
    }

    // Stall detection
    bool stallDetected() {
        return motors->stallDetected();
    }

    String stalledMotorName() {
        return motors->stalledMotorName();
    }

    // Stop all
    void stopAll() {
        motors->stopAll();
        servos->stopAll();
    }

    // Executes a single FoldStep by calculating deltas and calling black-box functions
    void executeStep(const FoldStep& step) {
        
        switch (step.type) {
            
            // ─── Small Subsystem Translation ───
            case STEP_SMALL_RESET: {
                motors->smallReset();
                current_small_counts = 0;
                break;
            }
            case STEP_SHIFT_SMALL: {
                motors->shiftSmall(step.arg);
                current_small_counts = step.arg;
                break;
            }

            // ─── Big Subsystem Translation ───
            case STEP_SHIFT_BIG: {
                motors->shiftBig(step.arg);
                current_big_slot = step.arg;
                break;
            }

            // ─── Folding Servos (Actuate) ───
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

            // ─── Folding Servos (Return to Rest) ───
            case STEP_UNFOLD_BIG:
                servos->unfoldBig();
                break;
            case STEP_UNFOLD_SMALL:
                servos->unfoldSmall();
                break;
        }
    }

private:
    MotorController* motors;
    ServoController* servos;

    // State tracking to convert absolute sequencer targets into relative deltas
    int current_big_slot = 0;
    int current_small_counts = 0;
};