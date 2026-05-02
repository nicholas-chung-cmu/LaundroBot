# serial_dispatcher.py
# Raspberry Pi — Serial Dispatcher
# Responsible for: sending the START command to Teensy,
#                  receiving READY / DONE / WARNING messages back
# Runs on: Raspberry Pi
# Depends on: pyserial
#
# Message protocol (full set):
#
#   RPi → Teensy:
#       "START:<size>\n"     size is "S", "M", or "L"
#                            e.g. "START:M\n"
#
#   Teensy → RPi:
#       "READY\n"            homing + scan complete, robot ready for start command
#       "DONE\n"             fold sequence finished successfully
#       "WARNING:<reason>\n" fault or invalid placement — robot has halted
#                            reasons: STALL_BIG_TOP, STALL_BIG_BOTTOM,
#                                     STALL_SMALL_LEFT, STALL_SMALL_RIGHT,
#                                     MOVE_TIMEOUT, HOMING_TIMEOUT,
#                                     PLACEMENT_OUT_OF_RANGE, UNKNOWN_GARMENT

import serial
import threading

class SerialDispatcher:

    def __init__(self, port: str, baud: int, on_message):
        """
        port       — serial port, e.g. "/dev/ttyUSB0"
        baud       — must match Teensy (115200)
        on_message — callback(message_str) called for every inbound line
        """
        self.on_message  = on_message
        self._input_buf  = ""

        # self.ser = serial.Serial(port, baud, timeout=1)

        # Start background listener thread
        # self._thread = threading.Thread(target=self._listen, daemon=True)
        # self._thread.start()

    def send(self, command: str):
        """
        Send a newline-terminated ASCII command to the Teensy.
        Currently only called with "START:<size>".
        """
        # self.ser.write(f"{command}\n".encode())
        pass

    def _listen(self):
        """
        Background thread. Reads lines from Teensy and calls on_message.
        Runs continuously for the life of the application.
        """
        while True:
            # raw = self.ser.readline()
            # if not raw:
            #     continue
            # line = raw.decode().strip()
            # if line:
            #     self.on_message(line)
            pass
