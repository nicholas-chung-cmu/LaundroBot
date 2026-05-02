# ui.py
# Raspberry Pi — User Interface
# Responsible for: size selection, start trigger, status display
# Runs on: Raspberry Pi
# Depends on: serial_dispatcher.py
#
# The RPi's only job in the fold process is:
#   1. Let the user pick a size and press Start
#   2. Send one message to the Teensy: "START:<size>"
#   3. Wait for "DONE" or "WARNING:<reason>" and update the display
#
# All sequencing, sensor reading, and motor control happens on the Teensy.

import tkinter as tk
from serial_dispatcher import SerialDispatcher

SIZES = ["S", "M", "L"]

class FoldingRobotUI:

    def __init__(self, root):
        self.root = root
        self.selected_size = tk.StringVar(value="M")
        self.status_text   = tk.StringVar(value="Waiting for robot...")

        self.dispatcher = SerialDispatcher(
            port="/dev/ttyUSB0",       # TODO: confirm port (run `ls /dev/ttyUSB*`)
            baud=115200,
            on_message=self.handle_message
        )

        self._build_ui()
        self._set_controls_enabled(False)   # disabled until Teensy sends READY

    def _build_ui(self):
        # Size selector — radio buttons for S, M, L
        for size in SIZES:
            # tk.Radiobutton(
            #     self.root, text=size, variable=self.selected_size, value=size
            # ).pack()
            pass

        # Start button
        # self.start_btn = tk.Button(
        #     self.root, text="Start", command=self.on_start_pressed
        # )
        # self.start_btn.pack()

        # Status label
        # tk.Label(self.root, textvariable=self.status_text).pack()
        pass

    def on_start_pressed(self):
        size = self.selected_size.get()         # "S", "M", or "L"
        self._set_controls_enabled(False)
        self.status_text.set("Folding...")
        self.dispatcher.send(f"START:{size}")   # only message RPi ever sends

    def handle_message(self, message: str):
        # All inbound messages from Teensy are handled here.
        #
        # Expected messages:
        #   "READY"              — Teensy has homed and scanned, fold can begin
        #   "DONE"               — fold sequence completed successfully
        #   "WARNING:<reason>"   — fault or placement error; reason is a short string
        #                          e.g. "WARNING:STALL_BIG_TOP"
        #                               "WARNING:PLACEMENT_OUT_OF_RANGE"
        #                               "WARNING:UNKNOWN_GARMENT"
        #
        # Note: no ACK messages — the Teensy handles sequencing internally.

        if message == "READY":
            self.status_text.set("Ready — select size and press Start")
            self._set_controls_enabled(True)

        elif message == "DONE":
            self.status_text.set("Done!")
            self._set_controls_enabled(True)

        elif message.startswith("WARNING:"):
            reason = message.split(":", 1)[1]
            self.status_text.set(f"Warning: {reason} — reposition garment and try again")
            self._set_controls_enabled(True)

    def _set_controls_enabled(self, enabled: bool):
        # Enable or disable the size selector and start button
        # state = tk.NORMAL if enabled else tk.DISABLED
        # self.start_btn.config(state=state)
        # for widget in size_radio_buttons: widget.config(state=state)
        pass


if __name__ == "__main__":
    root = tk.Tk()
    root.title("Laundry Folding Robot")
    app = FoldingRobotUI(root)
    root.mainloop()
