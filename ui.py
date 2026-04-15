# ui.py
# Raspberry Pi — User Interface Module
# Responsible for: displaying controls, capturing user input, showing status feedback
# Runs on: Raspberry Pi
# Depends on: sequencer.py (via callback), serial_dispatcher.py (for status updates)

# ─── Imports ────────────────────────────────────────────────────────────────
import tkinter as tk
from sequencer import Sequencer
from serial_dispatcher import SerialDispatcher

# ─── Constants ──────────────────────────────────────────────────────────────
SIZES = ["S", "M", "L"]

# ─── UI Class ───────────────────────────────────────────────────────────────
class FoldingRobotUI:

    def __init__(self, root):
        self.root = root
        self.selected_size = tk.StringVar(value="M")
        self.status_text = tk.StringVar(value="Ready")

        self.dispatcher = SerialDispatcher(
            port="/dev/ttyUSB0",        # TODO: confirm serial port
            baud=115200,
            on_status=self.handle_status_update
        )
        self.sequencer = Sequencer(self.dispatcher)

        self._build_ui()

    def _build_ui(self):
        # Size selector — radio buttons for S, M, L
        for size in SIZES:
            # create radio button bound to self.selected_size

        # Start button — calls self.on_start_pressed
        # Status label — bound to self.status_text
        pass

    def on_start_pressed(self):
        # Disable start button to prevent re-entry during fold
        # Read selected size from self.selected_size
        # Call self.sequencer.start(size)
        pass

    def handle_status_update(self, status: str):
        # Called by dispatcher when a message arrives from Teensy
        # Parse status string:
        #   "GARMENT:SHIRT" or "GARMENT:PANTS" → store garment type, pass to sequencer
        #   "OFFSET:x,y"                        → pass offset to sequencer
        #   "READY"                             → update status label, enable start button
        #   "ACK"                               → update progress indicator (step N of M done)
        #   "DONE"                              → show completion message, re-enable start
        #   "WARNING:reason"                    → show warning string, re-enable start button
        #                                         sequencer.reset() to clear state
        pass
