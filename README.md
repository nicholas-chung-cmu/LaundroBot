# Laundry Folding Robot — Software Architecture

## Overview

The robot is split across two processors with a strict division of responsibility:

- **Raspberry Pi** — user interface, fold logic, and sequence coordination
- **Teensy 4.1** — real-time motor control, sensor reading, and fault detection

They communicate over a UART serial link using simple newline-terminated ASCII messages.

---

## Hardware Summary

| Subsystem | Folders | DC Motors | Servos | Motion type |
|-----------|---------|-----------|--------|-------------|
| Big folder | Top + Bottom | 2 (one per folder, mirrored pair) | 4 (two per folder, mirrored pair) | Discrete position slots |
| Small folder | Left + Right | 2 (one per folder, mirrored pair) | 4 (two per folder, mirrored pair) | Continuous position |

DC motors function as servos via encoder feedback. CW rotation shifts folders outward from center; CCW shifts them inward. Big folder positions are discrete fixed increments. Small folder positions are continuous.

---

## File Structure

```
rpi/
  ui.py                 — Tkinter UI: size selector, start button, status display
  fold_library.py       — All garment/size parameters and sequence order
  sequencer.py          — Builds and steps through the fold command list
  serial_dispatcher.py  — UART send/receive, routes inbound messages

teensy/
  main.cpp              — setup() and loop() entry point
  state_machine.h       — System phase tracking and transition logic
  motor_controller.h    — PID loops for all 4 DC encoder motors
  servo_controller.h    — PWM control for all 8 fold servos
  serial_handler.h      — Parses RPi commands, sends status strings
  sensor_handler.h      — Mux + photoresistor reading, garment classification
```

---

## System Startup Sequence

Every power-on or reset follows this fixed sequence before a fold can occur:

```
Power on
  └─ Teensy: HOMING
       All 4 DC motors drive inward until stop detected
       Encoders zeroed
  └─ Teensy: SCAN
       Reads photoresistor array via multiplexer
       Classifies garment type (SHIRT / PANTS)
       Computes position offset (x mm, y mm)
       Sends "GARMENT:SHIRT" and "OFFSET:12,-5" to RPi
  └─ Teensy: sends "READY"
  └─ RPi: enables Start button, shows "Ready" in UI
```

The user then selects size (S/M/L) and presses Start.

---

## Fold Execution Flow

```
User selects size, presses Start
  └─ RPi sequencer.py calls fold_library.build_sequence()
       Looks up fold order for garment type (SHIRT or PANTS)
       Looks up big folder position slot for (garment, size)
       Looks up small folder distance for (garment, size)
       Applies sensor offset to small folder distance
       Returns ordered list of command steps

  └─ RPi sends one command at a time, waits for ACK before next:

       SHIFT_BIG <pos_index>     → Big DC motors move to discrete slot
       SHIFT_SMALL <counts>      → Small DC motors move to encoder target
       FOLD_BIG                  → Big folder servos sweep to fold angle
       FOLD_SMALL                → Small folder servos sweep to fold angle
       UNFOLD_BIG                → Big folder servos return to rest
       UNFOLD_SMALL              → Small folder servos return to rest

  └─ Teensy sends "ACK" after each command completes
  └─ Teensy sends "DONE" after last ACK
  └─ RPi shows completion, re-enables Start button
```

---

## Serial Protocol

All messages are ASCII strings terminated with `\n`. No binary framing.

### RPi → Teensy

| Command | Argument | Meaning |
|---------|----------|---------|
| `SHIFT_BIG` | `<pos_index>` | Move big folders to discrete slot 0–3 |
| `SHIFT_SMALL` | `<encoder_counts>` | Move small folders to continuous target |
| `FOLD_BIG` | — | Actuate big folder servo pairs |
| `FOLD_SMALL` | — | Actuate small folder servo pairs |
| `UNFOLD_BIG` | — | Return big folder servos to rest |
| `UNFOLD_SMALL` | — | Return small folder servos to rest |
| `RESET` | — | Abort, return to homing sequence |

### Teensy → RPi

| Message | Meaning |
|---------|---------|
| `GARMENT:SHIRT` / `GARMENT:PANTS` | Garment classification from SCAN |
| `OFFSET:<x>,<y>` | Position offset in mm, e.g. `OFFSET:12,-5` |
| `READY` | Homing + scan complete, awaiting sequence |
| `ACK` | Last command completed successfully |
| `DONE` | Full sequence complete |
| `WARNING:<reason>` | Fault detected — see Fault Handling below |

---

## Fold Library

`fold_library.py` is the single source of truth for all tunable parameters.

**Fold order** — which subsystem folds first, per garment type:
```python
FOLD_ORDER = {
    "SHIRT": ["BIG", "SMALL"],
    "PANTS": ["SMALL", "BIG"],
}
```

**Big folder positions** — discrete slot index per (garment, size):
```python
BIG_POSITIONS = {
    "SHIRT": {"S": 1, "M": 2, "L": 3},
    "PANTS": {"S": 1, "M": 2, "L": 3},
}
```
Slot encoder count = `index × BIG_INCREMENT_COUNTS` (constant defined in `motor_controller.h`).

**Small folder distances** — continuous target in mm per (garment, size):
```python
SMALL_DISTANCES_MM = {
    "SHIRT": {"S": 0, "M": 0, "L": 0},   # TODO: fill in after calibration
    "PANTS": {"S": 0, "M": 0, "L": 0},
}
```
The sensor offset (`offset_x`) is added to this distance before dispatch. The resulting mm value is multiplied by `SMALL_COUNTS_PER_MM` to produce the encoder count sent in `SHIFT_SMALL`.

---

## Teensy State Machine

```
IDLE ──────────────────────────────────────────────────┐
  │ (power on / RESET received)                         │
  ▼                                                     │
HOMING — all DC motors drive inward, encoders zeroed   │
  │                                                     │
  ▼                                                     │
SCAN — read sensors, send GARMENT and OFFSET to RPi    │
  │                                                     │
  ▼                                                     │
READY — send "READY", wait for first command from RPi  │
  │                                                     │
  ▼                                                     │
FOLD_SEQUENCE — execute commands, send ACK per step    │
  │                          │                          │
  │                     fault/timeout                   │
  ▼                          ▼                          │
DONE ──────────────────── WARNING ─────────────────────┘
send "DONE"               send "WARNING:<reason>"
return to IDLE            wait for RESET
```

---

## Fault Handling

If a fault occurs during any move (stall detected, timeout exceeded), the Teensy:
1. Stops all motors and servos immediately
2. Sends `WARNING:<reason>` to the RPi
3. Enters WARNING state — ignores all further commands except `RESET`

The RPi on receiving `WARNING`:
1. Displays the warning reason in the UI
2. Calls `sequencer.reset()` to clear command state
3. Re-enables the Start button only after `RESET` is sent and `READY` is received again

Warning reason strings:
- `STALL_BIG_TOP` / `STALL_BIG_BOTTOM` / `STALL_SMALL_LEFT` / `STALL_SMALL_RIGHT`
- `MOVE_TIMEOUT` — motor did not reach target within timeout window
- `HOMING_TIMEOUT` — homing sequence did not complete
- `UNEXPECTED_CMD` — command received in wrong state

---

## Key TODOs Before First Run

All of these require physical hardware to measure or tune:

**fold_library.py**
- `SMALL_DISTANCES_MM` — measure actual fold widths for each garment/size combination
- `BIG_POSITIONS` — confirm slot indices map to correct physical positions
- `FOLD_ORDER` — verify shirt and pants sequences produce correct folds

**motor_controller.h**
- `BIG_INCREMENT_COUNTS` — measure encoder counts per discrete position step
- `BIG_COUNTS_PER_MM` / `SMALL_COUNTS_PER_MM` — calibrate via measured travel
- PID gains `KP`, `KI`, `KD` — tune on bench with load
- `STALL_VELOCITY_THRESHOLD` and `STALL_CONFIRM_LOOPS` — set after observing normal motion
- `MOVE_TIMEOUT_MS` — set to ~2× the expected worst-case move duration
- Encoder ISR pin assignments and direction sign convention

**servo_controller.h**
- `BIG_FOLD_ANGLE` / `SMALL_FOLD_ANGLE` — measure required sweep angle per subsystem
- `SERVO_SWEEP_SPEED` — tune for smooth, reliable folding without disturbing the garment

**sensor_handler.h**
- `MUX_CHANNEL_COUNT` and pin assignments
- `classifyGarment()` — implement algorithm based on array layout and empirical data
- `computeOffset()` — implement based on physical sensor spacing in mm

**serial_dispatcher.py**
- Confirm serial port string (`/dev/ttyUSB0` or similar) on the RPi

---

## Development Tips

- **Test the Teensy in isolation first** using the Arduino Serial Monitor. You can manually type commands (`SHIFT_BIG 2\n`, `FOLD_BIG\n`) and watch the motors respond before the RPi is involved.
- **Test the RPi in isolation** by replacing `serial_dispatcher.py` with a mock that prints commands to console and immediately returns fake ACK messages. This lets you validate the fold sequence logic without any hardware.
- **Tune one motor at a time** during PID calibration — disable the others in `motor_controller.update()` temporarily.
- **The fold library is the easiest place to iterate** — changing distances and order requires no firmware recompile, only editing `fold_library.py` on the RPi.
