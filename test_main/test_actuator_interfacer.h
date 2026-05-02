#include "core_pins.h"
#pragma once
#include <Arduino.h>
#include "test_sequencer.h"
#include "test_translation_controller.h" 
#include "test_rotation_controller.h"

// ─── Hardware Pin Definitions (Placeholders) ──────────────────────────────────

// Motor identifiers (could be pins, I2C addresses, or simple ID numbers 
// depending on how motor_controller.h is written)
constexpr uint8_t PIN_SERVO_TRANS_TOP    = 1;  
constexpr uint8_t PIN_SERVO_TRANS_BOTTOM = 1;  
constexpr uint8_t PIN_SERVO_TRANS_LEFT   = 1;  
constexpr uint8_t PIN_SERVO_TRANS_RIGHT  = 1;  

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
    TranslationalController* tServos;
    RotationalController*    rServos;

    // State tracking to convert absolute sequencer targets into relative deltas
    int current_big_slot = 0;
    int current_small_degrees = 0;

    float move_length_millis = 1500;
    float move_start_time;

public:
    void init(TranslationalController* t, RotationalController* r) {
        tServos = t;
        rServos = r;
    }

    // Resets the state trackers. Call this after your physical homing sequence!
    void resetState() {
        current_big_slot = 0;
        current_small_degrees = 0;
    }


    bool allAtHome() {
        return true; //tServos->allAtHome();
    }

    void zeroEncoders() {
        //tServos->zeroEncoders();
        resetState();
    }

    // Movement completion
    bool currentMoveComplete() {
        if (millis() >= move_start_time + move_length_millis){
            return true;
        }
        return false; // tServos->currentMoveComplete() && rServos->allMovesComplete();
    }

    // Stop all
    void stopAll() {
        tServos->stopAll();
        rServos->stopAll();
    }

    // Executes a single FoldStep by calculating deltas and calling black-box functions
    void executeStep(const FoldStep& step) {
        move_start_time = millis();
        printStepType(step.type);
        Serial.println(String(step.arg));
        switch (step.type) {
            
            // ─── Small SINbsystem Translation ───
            case STEP_SMALL_RESET: {
                tServos->setHorizontalTarget(0);
                current_small_degrees = 0;
                break;
            }
            case STEP_SHIFT_SMALL: {
                tServos->setHorizontalTarget(step.arg);
                current_small_degrees = step.arg;
                break;
            }

            // ─── Big Subsystem Translation ───
            case STEP_SHIFT_BIG: {
                //tServos->setVerticalTarget(step.arg);
                current_big_slot = step.arg;
                break;
            }

            // ─── Folding Servos (Actuate) ───
            case STEP_FOLD_BIG_BOTTOM:
                rServos->foldBottom();
                break;
            case STEP_FOLD_BIG_TOP:
                rServos->foldTop();
                break;
            case STEP_FOLD_SMALL_LEFT:
                rServos->foldLeft();
                break;
            case STEP_FOLD_SMALL_RIGHT:
                rServos->foldRight();
                break;

            // ─── Folding Servos (Return to Rest) ───
            case STEP_UNFOLD_BIG:
                rServos->unfoldBig();
                break;
            case STEP_UNFOLD_SMALL:
                rServos->unfoldSmall();
                break;
        }
    }

};