// serial_handler.h / serial_handler.cpp
// Teensy 4.1 — Serial Communication Handler
// Responsible for: reading newline-terminated ASCII commands from RPi,
//                  routing them to the state machine,
//                  sending status/response strings back to RPi
// Runs on: Teensy 4.1
// Depends on: state_machine.h

#pragma once
#include "state_machine.h"

class SerialHandler {
public:

    void init(StateMachine* sm) {
        this->stateMachine = sm;
        // Serial already begun in main.cpp setup()
        inputBuffer = "";
    }

    // ── Called every loop() ─────────────────────────────────────────────────
    void update() {
        // Read all available bytes from Serial into inputBuffer
        // When a '\n' is found:
        //   extract the complete line (trim whitespace)
        //   pass to parseAndRoute()
        //   clear inputBuffer

        // while (Serial.available()) {
        //     char c = Serial.read();
        //     if (c == '\n') {
        //         parseAndRoute(inputBuffer);
        //         inputBuffer = "";
        //     } else {
        //         inputBuffer += c;
        //     }
        // }
    }

    // ── Outbound messages (called by state machine) ──────────────────────────

    static void send(const String& message) {
        // Write message + newline to Serial
        // Serial.println(message);
        //
        // Valid outbound messages:
        //   "GARMENT:SHIRT"     — garment classification result
        //   "GARMENT:PANTS"
        //   "OFFSET:<x>,<y>"   — position offset in mm, e.g. "OFFSET:12,-5"
        //   "READY"             — homing + scan complete, awaiting sequence
        //   "ACK"               — last command executed successfully
        //   "DONE"              — full fold sequence complete
        //   "WARNING:<reason>"  — fault; reason examples:
        //                           STALL_BIG_TOP
        //                           STALL_SMALL_LEFT
        //                           MOVE_TIMEOUT
        //                           HOMING_TIMEOUT
        //                           UNEXPECTED_CMD
    }

private:
    StateMachine* stateMachine;
    String inputBuffer;

    void parseAndRoute(const String& line) {
        // Tokenise line by first space:
        //   token[0] = command keyword
        //   token[1] = argument (may be absent)
        //
        // Route:
        //   "SHIFT_BIG <n>"    → stateMachine->onCommand(line)
        //   "SHIFT_SMALL <n>"  → stateMachine->onCommand(line)
        //   "FOLD_BIG"         → stateMachine->onCommand(line)
        //   "FOLD_SMALL"       → stateMachine->onCommand(line)
        //   "UNFOLD_BIG"       → stateMachine->onCommand(line)
        //   "UNFOLD_SMALL"     → stateMachine->onCommand(line)
        //   "RESET"            → stateMachine->onCommand(line)
        //   (unknown)          → send("WARNING:UNEXPECTED_CMD")
    }
};
