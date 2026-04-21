// main.ino
// Teensy 4.1 — Entry Point
// Responsible for: initialising all subsystems, running the main control loop
// Runs on: Teensy 4.1
// Depends on: state_machine.h, serial_handler.h, motor_controller.h,
//             servo_controller.h, sensor_handler.h

#include "state_machine.h"
#include "serial_handler.h"
#include "motor_controller.h"
#include "servo_controller.h"
#include "sensor_handler.h"

// ── Global subsystem instances ───────────────────────────────────────────────
StateMachine  stateMachine;
SerialHandler serialHandler;
MotorController motorController;
ServoController servoController;
SensorHandler sensorHandler;

void setup() {
    // Initialise USB serial for RPi communication
    Serial.begin(115200);

    // Initialise motor controller
    //   - configure encoder pins for all 4 DC motors
    //   - set PID gains (kP, kI, kD) — TODO: tune after assembly
    //   - attach motor driver PWM + direction pins
    motorController.init();

    // Initialise servo controller
    //   - attach all 8 servo PWM pins
    //   - move all servos to rest angles (defined in servo_controller.h)
    servoController.init();

    // Initialise sensor handler
    //   - configure multiplexer select pins
    //   - configure ADC pins for photoresistor array
    sensorHandler.init();

    // Initialise serial handler
    //   - bind references to other subsystems so it can route commands
    serialHandler.init(&stateMachine);

    // Run startup sequence: HOME → SCAN
    stateMachine.transition(STATE_HOMING);
}

void loop() {
    // 1. Read and process any incoming serial commands from RPi
    serialHandler.update();

    // 2. Run PID loops for all DC encoder motors (must run every cycle)
    motorController.update();

    // 3. Update state machine (checks for completed moves, timeouts, faults)
    stateMachine.update();

    // 4. Small delay to maintain consistent loop timing
    //    TODO: consider using a fixed-rate timer interrupt instead of delay
    // delayMicroseconds(500);   // ~2 kHz loop rate — adjust as needed
}
