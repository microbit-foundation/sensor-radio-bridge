#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Simple test that establishes a serial connection to the micro:bit and sends
the commands expected to configures the sensors to stream and consumes the
received data.
"""
import sys
import time
import uuid

from serial import Serial, PARITY_NONE, STOPBITS_ONE
from serial.tools import list_ports


def find_microbit_serial_port():
    """Finds the micro:bit serial port by matching USB PID & VID."""
    comports = list_ports.comports()
    for port in comports:
        # micro:bit's DAPLink PID & VID
        if port.pid == 0x0204 and port.vid == 0x0D28:
            return port.device
    return None


def print_all_serial_received(ubit_serial):
    """
    Print all the serial data received until the PC buffers are empty.

    :param ubit_serial: The serial connection to the micro:bit.
    """
    serial_line = ubit_serial.readline()
    while serial_line:
        print(f'(DEVICE 🔙) {serial_line.decode("ascii")}', end="")
        serial_line = ubit_serial.readline()


def send_command_wait_for_response(ubit_serial, command, timeout=5):
    """
    Send a command to the micro:bit and wait for a response.

    :param ubit_serial: The serial connection to the micro:bit.
    :param command: The command string to send.
    :param timeout: The timeout in seconds to wait for a response.

    :raises Exception: If the response is does not contain the expected UUID.
    :raises Exception: If the expected response is not received within the timeout.

    :return: A tuple containing the sent command, the response, and any
        periodic messages received while waiting for the response.
        All without the new line.
    """
    # Create a 32 bit long random UUID for the command and the response
    uuid_hex = uuid.uuid4().hex[:8].upper()
    cmd = bytes(f"C[{uuid_hex}]{command}\n", "ascii")
    expected_response_start = bytes(f"R[{uuid_hex}]", "ascii")

    ubit_serial.write(cmd)

    periodic_messages = []

    timeout_time = time.time() + timeout
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(expected_response_start):
                return cmd[:-1], serial_line[:-1], periodic_messages
            if serial_line.startswith(b"P["):
                # Periodic messages that are received at a constant interval
                periodic_messages.append(serial_line[:-1])
            else:
                raise Exception(f"Unexpected response: {serial_line}")

    raise Exception(f"Timed out waiting for response to -> {cmd}")


def main():
    print("(SCRIPT) Connecting to device serial..")
    microbit_port = find_microbit_serial_port()
    if not microbit_port:
        raise Exception("Could not automatically detect micro:bit port.")
    ubit_serial = Serial(
        microbit_port, 115200, timeout=0.5, parity=PARITY_NONE,
        stopbits=STOPBITS_ONE, rtscts=False, dsrdtr=False
    )
    print("(SCRIPT) Connected, printing any received data (there shouldn't be any)...")
    time.sleep(0.1)
    print_all_serial_received(ubit_serial)
    print("(SCRIPT) Done.")

    print("(SCRIPT) Sending handshake.")
    sent_handshake, handshake_response, periodic_msgs = send_command_wait_for_response(ubit_serial, "HS[]")
    print(f"\t(SENT   ➡️) {sent_handshake}")
    if not handshake_response:
        raise Exception("Handshake failed.")
    print(f"\t(DEVICE 🔙) {handshake_response}")
    response_cmd = handshake_response.decode("ascii").split("]", 1)[1]
    if response_cmd != "HS[]":
        raise Exception("Handshake failed.")
    if len(periodic_msgs) != 0:
        raise Exception("Handshake failed.")
    print("(SCRIPT) Handshake successful.")

    print("(SCRIPT) Printing all periodic messages received now...")
    timeout_time = time.time() + 1    # 1 second timeout
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            print(f"(DEVICE 🔁) {serial_line[:-1]}")
            timeout_time = time.time() + 1    # 1 second timeout

    print("(SCRIPT ➡️) Timeout.")

    return 0


if __name__ == "__main__":
    exit_status = main()
    sys.exit(exit_status)