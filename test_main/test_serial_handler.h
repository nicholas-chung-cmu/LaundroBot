// serial_handler.h
// Teensy 4.1 — Serial Communication Handler
// Responsible for: parsing inbound messages from RPi via UART (Serial8),
//                  sending READY / DONE / WARNING outbound
// Runs on: Teensy 4.1
// Depends on: state_machine.h, fold_library.h
//
// Inbound (RPi → Teensy on Serial8):
//   "Small\n"          triggers fold sequence SIZE_S
//   "Medium\n"         triggers fold sequence SIZE_M
//   "Large\n"          triggers fold sequence SIZE_L
//   "Stop\n"           abort and re-home
//
// Outbound (Teensy → RPi on Serial8):
//   "READY\n"          homed and scanned, waiting for START
//   "DONE\n"           fold sequence complete
//   "WARNING:<reason>" fault or placement error

#pragma once
#include <Arduino.h>

class SerialHandler {
public:

    void init(StateMachine* sm) {
        stateMachine = sm;
        inputBuffer  = "";
        // Note: Ensure Serial.begin() and Serial8.begin() are called in main.cpp setup()
    }

    // ── Called every loop() ───────────────────────────────────────────────────
    void update() {
        // Accumulate incoming bytes from Raspberry Pi via Serial8
        // Non-blocking: builds the string until it sees a newline
        while (Serial8.available()) {
            char c = (char)Serial8.read();

            Serial.print(c);
            if (c == '\n') {
                inputBuffer.trim(); // Clean up hidden \r or spaces
                parse(inputBuffer);
                inputBuffer = "";
            } else if (c != '\r') {
                inputBuffer += c;
            }
        }
        
        // ── Debug mode: single character input simulation via PC ────────────
        // Press 's', 'm', 'l', or 'x' in the PC serial monitor to simulate RPi commands
        debugKeyboardInput();
    }

    // ── Debug: simulate RPi commands via keyboard ────────────────────────────
    void debugKeyboardInput() {
        if (Serial.available()) {
            char key = (char)Serial.read();
            
            // Consume any trailing whitespace
            if (key == '\r' || key == '\n') return;
            
            switch (key) {
                case 's':
                case 'S':
                    Serial.println("[DEBUG] Simulating 'Small' via 's' key");
                    stateMachine->onStartCommand(SIZE_S);
                    break;
                    
                case 'm':
                case 'M':
                    Serial.println("[DEBUG] Simulating 'Medium' via 'm' key");
                    stateMachine->onStartCommand(SIZE_M);
                    break;
                    
                case 'l':
                case 'L':
                    Serial.println("[DEBUG] Simulating 'Large' via 'l' key");
                    stateMachine->onStartCommand(SIZE_L);
                    break;

                case 'x':
                case 'X':
                    Serial.println("[DEBUG] Simulating 'Stop' via 'x' key");
                    stateMachine->onResetCommand();
                    break;
                    
                default:
                    // Ignore other keys
                    break;
            }
        }
    }

    // ── Outbound — called by state_machine via transition() ───────────────────
    static void send(const char* message) {
        // Send actual message to the Raspberry Pi
        Serial8.println(message);
        
        // Mirror the message to the PC Monitor for debugging
        Serial.print("[UART SEND to RPi] ");
        Serial.println(message);
    }

private:
    StateMachine* stateMachine;
    String        inputBuffer;

    void parse(const String& line) {
        // Ignore empty lines
        if (line.length() == 0) return;

        String clothing_size = line[0];
        String clothing_type = line[1];

        if (clothing_type == "S") {
            stateMachine->detectedGarment = GARMENT_SHIRT;
        } else {
            stateMachine->detectedGarment = GARMENT_PANTS;
        }

        if (clothing_size == "S") {
            Serial.println("Received from RPi: Small");
            stateMachine->onStartCommand(SIZE_S);
        } 
        else if (clothing_size == "M") {
            Serial.println("Received from RPi: Medium");
            stateMachine->onStartCommand(SIZE_M);
        } 
        else if (clothing_size == "L") {
            Serial.println("Received from RPi: Large");
            stateMachine->onStartCommand(SIZE_L);
        } 
        else if (line == "Stop") {
            Serial.println("Received from RPi: Stop");
            stateMachine->onResetCommand();
        } 
        else {
            // Unknown command — log but do not halt
            Serial.print("WARNING: Unknown command received: ");
            Serial.println(line);
            send("WARNING:UNKNOWN_CMD");
        }
    }
};