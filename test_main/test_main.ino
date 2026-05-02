// main.cpp
// Teensy 4.1 — Entry Point
// Responsible for: initialising all subsystems, running the main control loop
// Runs on: Teensy 4.1
// Depends on: state_machine.h, serial_handler.h, motor_controller.h,
//             servo_controller.h, sensor_handler.h

#include "test_translation_controller.h"
#include "test_rotation_controller.h"
#include "test_actuator_interfacer.h"
#include "test_sensor_handler.h"
#include "test_state_machine.h"
#include "test_serial_handler.h"

// ── Global subsystem instances ───────────────────────────────────────────────
// These live forever in memory, preventing the Teensy from crashing!
TranslationalController tController;
RotationalController    rController;
ActuatorInterfacer      actuatorInterfacer;
SensorHandler           sensorHandler;
StateMachine            stateMachine;
SerialHandler           serialHandler;

void setup() {
    // Initialise USB serial for RPi communication
    Serial.begin(9600);
    Serial8.begin(9600);

    tController.init();
    rController.init();

    actuatorInterfacer.init(&tController, &rController);

    Serial.println("starting state machine");
    stateMachine.init(&actuatorInterfacer, &sensorHandler);

    Serial.println("starting serial handler");
    serialHandler.init(&stateMachine);
}

void loop() {
    serialHandler.update();

    stateMachine.update();

    rController.update();
    tController.update();

    // Small delay to maintain consistent loop timing
    //    TODO: consider using a fixed-rate timer interrupt instead of delay
    // delayMicroseconds(500);   // ~2 kHz loop rate — adjust as needed
}
