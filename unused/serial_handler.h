// serial_handler.h
// Teensy 4.1 — Serial Communication Handler
// Responsible for: parsing inbound messages from RPi,
//                  sending READY / DONE / WARNING outbound
// Runs on: Teensy 4.1
// Depends on: state_machine.h, fold_library.h
//
// Inbound (RPi → Teensy):
//   "START:<size>\n"   size is S, M, or L — triggers fold sequence
//   "RESET\n"          abort and re-home
//
// Outbound (Teensy → RPi):
//   "READY\n"          homed and scanned, waiting for START
//   "DONE\n"           fold sequence complete
//   "WARNING:<reason>" fault or placement error

#pragma once
#include <Arduino.h>
#include "sequencer.h"
#include "state_machine.h"

class SerialHandler {
public:

    void init(StateMachine* sm) {
        stateMachine = sm;
        inputBuffer  = "";
        // Serial.begin(115200);  — done in main.cpp setup()
    }

    // ── Called every loop() ───────────────────────────────────────────────────
    void update() {
        // Accumulate incoming bytes into inputBuffer.
        // On newline: parse the complete line and clear buffer.
        //
        // while (Serial.available()) {
        //     char c = (char)Serial.read();
        //     if (c == '\n') {
        //         parse(inputBuffer);
        //         inputBuffer = "";
        //     } else if (c != '\r') {
        //         inputBuffer += c;
        //     }
        // }
    }

    // ── Outbound — called by state_machine via transition() ───────────────────
    static void send(const char* message) {
        // Serial.println(message);
    }

private:
    StateMachine* stateMachine;
    String        inputBuffer;

    void parse(const String& line) {
        if (line.startsWith("START:")) {
            // Extract size character after the colon
            char sizeChar = line.charAt(6);   // "START:M" → 'M'
            FoldSize size;
            switch (sizeChar) {
                case 'S': size = SIZE_S; break;
                case 'M': size = SIZE_M; break;
                case 'L': size = SIZE_L; break;
                default:
                    send("WARNING:INVALID_SIZE");
                    return;
            }
            stateMachine->onStartCommand(size);

        } else if (line == "RESET") {
            stateMachine->onResetCommand();

        } else {
            // Unknown command — log but do not halt
            send("WARNING:UNKNOWN_CMD");
        }
    }
};
