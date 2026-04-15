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

// ── Configuration (TODO: fill in after hardware is finalised) ────────────────
constexpr int MUX_CHANNEL_COUNT = 8;   // TODO: actual number of photoresistors
constexpr int MUX_ADC_PIN       = A0;  // TODO: Teensy ADC pin connected to mux output

// Mux select pins — 3 pins for up to 8 channels (adjust for larger mux)
constexpr int MUX_SEL_0 = 2;   // TODO
constexpr int MUX_SEL_1 = 3;   // TODO
constexpr int MUX_SEL_2 = 4;   // TODO

// ── Result struct ────────────────────────────────────────────────────────────
struct ScanResult {
    String garmentType;  // "SHIRT" or "PANTS"
    int    offsetX_mm;   // left/right offset from center (positive = right)
    int    offsetY_mm;   // top/bottom offset from center (positive = up)
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
        // Step through each mux channel:
        //   set mux select pins to binary representation of channel index
        //   wait for mux settling time (small delayMicroseconds)
        //   read ADC value and store in rawReadings[channel]
        //
        // Then run classification and offset estimation on rawReadings[]

        readAllChannels();
        lastResult.garmentType = classifyGarment();
        computeOffset(lastResult.offsetX_mm, lastResult.offsetY_mm);
    }

    ScanResult getResult() {
        return lastResult;
    }

    String getGarmentType() { return lastResult.garmentType; }
    int    getOffsetX()     { return lastResult.offsetX_mm;  }
    int    getOffsetY()     { return lastResult.offsetY_mm;  }

private:
    int        rawReadings[MUX_CHANNEL_COUNT];
    ScanResult lastResult;

    void readAllChannels() {
        // For channel = 0 to MUX_CHANNEL_COUNT - 1:
        //   write channel bits to MUX_SEL_0/1/2
        //   delayMicroseconds(10)  // allow mux to settle
        //   rawReadings[channel] = analogRead(MUX_ADC_PIN)
    }

    String classifyGarment() {
        // TODO: implement classification algorithm based on sensor array layout
        //
        // Placeholder logic — replace with empirically derived thresholds:
        //   Analyse pattern in rawReadings[] (which sensors are covered/uncovered)
        //   Shirts and pants have different silhouettes → different coverage patterns
        //   Return "SHIRT" or "PANTS"
        return "SHIRT";
    }

    void computeOffset(int& outX, int& outY) {
        // TODO: implement offset estimation
        //
        // Placeholder logic — replace after sensor array positions are known:
        //   Use weighted centroid of covered sensors vs expected center
        //   to estimate how far off-center the garment is placed
        //   Convert sensor-index delta to mm using physical sensor spacing
        outX = 0;
        outY = 0;
    }
};
