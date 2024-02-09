#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
A simple script to start streaming data from the micro:bit bridge to the
computer.

This script is a quicker way to test streaming data after running the
test_radio.py script.
"""
import os
import sys
import time

THIS_FOLDER = os.path.dirname(os.path.abspath(__file__))
sys.path.append(THIS_FOLDER)

from test_protocol import connect_serial, test_cmd


def main():
    ubit_serial = connect_serial()

    test_cmd(ubit_serial, "Start", "START[AB]", "START[]")
    while True:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P["):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
            else:
                print(f"\t(DEVICE ❌) {serial_line[:-1]}")
        time.sleep(0.001)

    return 0


if __name__ == "__main__":
    sys.exit(main())
