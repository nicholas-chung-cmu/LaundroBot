// sensor_handler.h / sensor_handler.cpp
// Teensy 4.1 — Sensor Handler
// Responsible for: reading the photoresistor array via multiplexer,
//                  classifying garment type, computing position offset
// Runs on: Teensy 4.1
//
// Hardware:
//   Photoresistors feed into a multiplexer (e.g. 74HC4051 or similar)
//   Mux select lines connect to Teensy digital output pins
//   Mux output (analog signal) connects to one Teensy ADC pin
//
// This module is treated as a BLACK BOX for initial software planning.
// The classification and offset algorithms are TBD pending sensor array
// layout and empirical data from hardware testing.

#pragma once
#include <Arduino.h>
#include "fold_library.h"

// ── Configuration (TODO: fill in after hardware is finalised) ────────────────
constexpr int MUX_CHANNEL_COUNT = 8;   // TODO: actual number of photoresistors
constexpr int MUX_ADC_PIN       = A0;  // TODO: Teensy ADC pin connected to mux output

// Mux select pins — 3 pins for up to 8 channels (adjust for larger mux)
constexpr int MUX_SEL_0 = 2;   // TODO
constexpr int MUX_SEL_1 = 3;   // TODO
constexpr int MUX_SEL_2 = 4;   // TODO

// ── Result struct ────────────────────────────────────────────────────────────
struct ScanResult {
    GarmentType garment;      // GARMENT_SHIRT, GARMENT_PANTS, or GARMENT_UNKNOWN
    float       centerX_mm;   // clothing centerpoint X offset from board center (mm), + = right
    float       centerY_mm;   // clothing centerpoint Y offset from board center (mm), + = up
};

class SensorHandler {
public:

    void init() {
        // Configure mux select pins as outputs
        // pinMode(MUX_SEL_0, OUTPUT);
        // pinMode(MUX_SEL_1, OUTPUT);
        // pinMode(MUX_SEL_2, OUTPUT);
        // Configure ADC pin (usually analogRead handles this automatically)
    }

    void runScan() {
        readAllChannels();
        lastResult.garment    = classifyGarment();
        computeCenter(lastResult.centerX_mm, lastResult.centerY_mm);
        Serial.println("Sensor scan complete: readings, garment=" + String(lastResult.garment) + ", centerX=" + String(lastResult.centerX_mm) + ", centerY=" + String(lastResult.centerY_mm));
    }

    GarmentType getGarmentType() { return lastResult.garment;     }
    float       getCenterX_mm()  { return lastResult.centerX_mm;  }
    float       getCenterY_mm()  { return lastResult.centerY_mm;  }

private:
    int        rawReadings[MUX_CHANNEL_COUNT];
    ScanResult lastResult;

    void readAllChannels() {
        // Dummy readings for testing
        for (int i = 0; i < MUX_CHANNEL_COUNT; i++) {
            rawReadings[i] = random(100, 900);  // dummy analog values
        }
        Serial.println("Dummy sensor readings: " + String(rawReadings[0]) + ", " + String(rawReadings[1]) + ", ...");
    }

    GarmentType classifyGarment() {
        // Dummy classification
        return GARMENT_SHIRT;  // or random
    }

    void computeCenter(float& outX_mm, float& outY_mm) {
        // Dummy center
        outX_mm = 10.0f;
        outY_mm = 5.0f;
    }
};
