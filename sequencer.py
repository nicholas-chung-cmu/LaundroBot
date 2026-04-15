# sequencer.py
# Raspberry Pi — Fold Sequencer
# Responsible for: receiving scan results, building the command sequence,
#                  feeding commands one-at-a-time to the dispatcher
# Runs on: Raspberry Pi
# Depends on: fold_library.py, serial_dispatcher.py

from fold_library import build_sequence

class Sequencer:

    def __init__(self, dispatcher):
        self.dispatcher = dispatcher
        self._reset_state()

    def _reset_state(self):
        self.garment   = None       # "SHIRT" or "PANTS", set after SCAN
        self.offset_x  = 0         # mm, left/right position correction from sensors
        self.offset_y  = 0         # mm, top/bottom position correction from sensors
        self.size      = None       # "S", "M", or "L", set by user before start
        self.sequence  = []         # list of command dicts from fold_library
        self.step_index = 0         # current position in sequence

    # ── Called by UI after SCAN results arrive ──────────────────────────────

    def set_garment(self, garment: str):
        # Store garment type received from Teensy ("SHIRT" or "PANTS")
        self.garment = garment

    def set_offset(self, x: int, y: int):
        # Store position offset received from Teensy (in mm)
        self.offset_x = x
        self.offset_y = y

    # ── Called by UI when user presses Start ─────────────────────────────────

    def start(self, size: str):
        # Validate that garment type and offset have been received
        # If not, raise error / show warning — scan not complete yet

        self.size = size
        self.sequence = build_sequence(
            self.garment, self.size, self.offset_x, self.offset_y
        )
        self.step_index = 0
        self._send_next()

    # ── Called by dispatcher when ACK received ───────────────────────────────

    def on_ack(self):
        # Advance step index
        self.step_index += 1

        if self.step_index < len(self.sequence):
            self._send_next()
        # else: sequence complete — dispatcher will surface the DONE message to UI

    # ── Internal ─────────────────────────────────────────────────────────────

    def _send_next(self):
        step = self.sequence[self.step_index]
        cmd  = step["cmd"]

        # Translate command dict to ASCII string and hand to dispatcher
        if cmd == "SHIFT_BIG":
            self.dispatcher.send(f"SHIFT_BIG {step['pos_index']}")

        elif cmd == "SHIFT_SMALL":
            self.dispatcher.send(f"SHIFT_SMALL {step['counts']}")

        elif cmd in ("FOLD_BIG", "FOLD_SMALL", "UNFOLD_BIG", "UNFOLD_SMALL"):
            self.dispatcher.send(cmd)

        else:
            # Unknown command — log error, call reset
            self.reset()

    def reset(self):
        # Called on WARNING or unknown command
        # Clear all state, ready for a fresh start command
        self._reset_state()
