# serial_dispatcher.py
# Raspberry Pi — Serial Command Dispatcher
# Responsible for: sending ASCII commands to Teensy over UART,
#                  receiving and routing status messages back to UI and sequencer
# Runs on: Raspberry Pi
# Depends on: pyserial

import serial
import threading

class SerialDispatcher:

    def __init__(self, port: str, baud: int, on_status):
        """
        port      — serial port string, e.g. "/dev/ttyUSB0"
        baud      — baud rate, must match Teensy (e.g. 115200)
        on_status — callback(status_str) called on every inbound message from Teensy
        """
        self.on_status = on_status
        self._lock = threading.Lock()   # prevents overlapping writes
        self._waiting_for_ack = False

        # Open serial port
        # self.ser = serial.Serial(port, baud, timeout=1)

        # Start background thread to listen for inbound messages
        # self._listener_thread = threading.Thread(target=self._listen, daemon=True)
        # self._listener_thread.start()

    # ── Sending ──────────────────────────────────────────────────────────────

    def send(self, command: str):
        """
        Send a newline-terminated ASCII command to Teensy.
        Blocks if a previous command is still awaiting ACK.
        Thread-safe.

        Valid commands:
            SHIFT_BIG <pos_index>      — move big folder to discrete position slot
            SHIFT_SMALL <counts>       — move small folder to encoder count target
            FOLD_BIG                   — actuate big folder servo pair
            FOLD_SMALL                 — actuate small folder servo pair
            UNFOLD_BIG                 — return big folder servos to rest
            UNFOLD_SMALL               — return small folder servos to rest
            RESET                      — abort sequence, return to IDLE
        """
        with self._lock:
            self._waiting_for_ack = True
            # write command + newline to serial port
            # self.ser.write(f"{command}\n".encode())

    # ── Receiving (background thread) ────────────────────────────────────────

    def _listen(self):
        """
        Runs in background thread.
        Reads newline-terminated lines from Teensy and routes them.

        Inbound message format:
            "GARMENT:SHIRT"     — garment classification result from SCAN
            "GARMENT:PANTS"
            "OFFSET:x,y"        — position offset in mm (e.g. "OFFSET:12,-5")
            "READY"             — Teensy homed and scanned, awaiting sequence
            "ACK"               — last command completed successfully
            "DONE"              — full fold sequence complete
            "WARNING:reason"    — fault detected; reason is a short string
                                  e.g. "WARNING:STALL_BIG_TOP"
        """
        while True:
            # line = self.ser.readline().decode().strip()
            # if not line: continue
            line = ""   # placeholder

            if line.startswith("ACK"):
                self._waiting_for_ack = False
                # notify sequencer via on_status so it can advance to next step

            self.on_status(line)    # forward everything to UI callback

    def send_reset(self):
        # Send RESET command regardless of ACK state
        # Used by UI when WARNING received or user aborts
        with self._lock:
            self._waiting_for_ack = False
            # self.ser.write(b"RESET\n")
