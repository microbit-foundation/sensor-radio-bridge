#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Simple test that establishes a serial connection to the micro:bit and sends
the commands expected to configure the sensors data to start streaming.

It prints all sent and received data to the terminal for inspection.
"""
import sys
import time
import uuid
import random

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



def send_command(ubit_serial, command, wait_response=True, timeout=5):
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
    # Create a random UUID for the command and the response,
    # randomise the size to be from one byte to 4 bytes
    if "id_size" not in send_command.__dict__: send_command.id_size = 0
    send_command.id_size = send_command.id_size + 1 if send_command.id_size < 8 else 1
    uuid_hex = uuid.uuid4().hex[:send_command.id_size].upper()

    cmd = bytes(f"C[{uuid_hex}]{command}\n", "ascii")
    expected_response_start = bytes(f"R[{uuid_hex}]", "ascii")

    ubit_serial.write(cmd)

    if not wait_response:
        return cmd[:-1], None, []

    periodic_messages = []

    timeout_time = time.time() + timeout
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(expected_response_start):
                return cmd[:-1], serial_line[:-1], periodic_messages
            elif serial_line.startswith(b"t["):
                # find the index that starts with the expected response
                index = serial_line.find(expected_response_start)
                if index != -1:
                    return cmd[:-1], serial_line[index:-1], periodic_messages
                else:
                    raise Exception(f"Unexpected response: {serial_line}")
            if serial_line.startswith(b"P"):
                # Periodic messages that are received at a constant interval
                periodic_messages.append(serial_line[:-1])
            else:
                raise Exception(f"Unexpected response: {serial_line}")

    raise Exception(f"Timed out waiting for response to -> {cmd}")


def main():
    print("Connecting to device serial..")
    microbit_port = find_microbit_serial_port()
    if not microbit_port:
        raise Exception("Could not automatically detect micro:bit port.")
    ubit_serial = Serial(
        microbit_port, 115200, timeout=0.5, parity=PARITY_NONE,
        stopbits=STOPBITS_ONE, rtscts=False, dsrdtr=False
    )
    print("Connected, printing any received data (there shouldn't be any)...")
    time.sleep(0.1)
    print_all_serial_received(ubit_serial)
    print("Done.")

    print("Sending handshake.")
    sent_handshake, handshake_response, periodic_msgs = send_command(ubit_serial, "HS[]", wait_response=True)
    print(f"\t(SENT   ➡️) {sent_handshake}")
    if not handshake_response:
        raise Exception("Handshake failed.")
    print(f"\t(DEVICE 🔙) {handshake_response}")
    # Parsing the response to check if it's a handshake response with "1" as the version
    response_cmd = handshake_response.decode("ascii").split("]", 1)[1]
    if response_cmd != "HS[1]":
        raise Exception("Handshake failed.")
    if len(periodic_msgs) != 0:
        raise Exception("Periodic messages received, handshake failed.")
    print("Handshake successful.")

    print("Sending radio frequency command.")
    random_freq = random.randint(0, 255)
    random_freq = 42
    sent_radio_freq, radio_freq_response, periodic_msgs = send_command(ubit_serial, f"RF[{random_freq}]", wait_response=True)
    print(f"\t(SENT   ➡️) {sent_radio_freq}")
    if not radio_freq_response:
        raise Exception("Set Radio Frequency command failed.")
    print(f"\t(DEVICE 🔙) {radio_freq_response}")
    # Parsing the response for the start command
    response_cmd = radio_freq_response.decode("ascii").split("]", 1)[1]
    if response_cmd != f"RF[{random_freq}]":
        raise Exception("Radio Frequency command  failed.")
    if len(periodic_msgs) != 0:
        raise Exception("Periodic messages received, start command  failed.")
    print("Radio Frequency command successful.")

    print("Sending periodic message period in milliseconds.")
    sent_period, period_response, periodic_msgs = send_command(ubit_serial, "PER[20]", wait_response=True)
    print(f"\t(SENT   ➡️) {sent_period}")
    if not period_response:
        raise Exception("Set Period command failed.")
    print(f"\t(DEVICE 🔙) {period_response}")
    # Parsing the response for the start command
    response_cmd = period_response.decode("ascii").split("]", 1)[1]
    if response_cmd != "PER[20]":
        raise Exception("Period command  failed.")
    if len(periodic_msgs) != 0:
        raise Exception("Periodic messages received, period command  failed.")
    print("Period command successful.")

    print("Sending start command.")
    sent_start, start_response, periodic_msgs = send_command(ubit_serial, "START[PABFMLTS]", wait_response=True)
    print(f"\t(SENT   ➡️) {sent_start}")
    if not start_response:
        raise Exception("Start command failed.")
    print(f"\t(DEVICE 🔙) {start_response}")
    # Parsing the response for the start command
    response_cmd = start_response.decode("ascii").split("]", 1)[1]
    if response_cmd != "START[]":
        raise Exception("Start command  failed.")
    if len(periodic_msgs) != 0:
        raise Exception("Periodic messages received, start command  failed.")
    print("Start command successful.")

    print("Printing all periodic messages received for 1 second...")
    timeout_time = time.time() + 1
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P["):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
            else:
                print(f"\t(DEVICE ❌) {serial_line[:-1]}")
                #raise Exception(f"Message received is not periodic type: {serial_line}")
        time.sleep(0.0005)

    print("Sending stop command.")
    sent_stop, stop_response, periodic_msgs = send_command(ubit_serial, "STOP[]", wait_response=True)
    print(f"\t(SENT   ➡️) {sent_stop}")
    if not stop_response:
        raise Exception("Stop command failed.")
    print(f"\t(DEVICE 🔙) {stop_response}")
    # Parsing the response for the start command
    response_cmd = stop_response.decode("ascii").split("]", 1)[1]
    if response_cmd != "STOP[]":
        raise Exception("Stop command failed.")
    print("Additional periodic messages while processing STOP command:")
    for msg in periodic_msgs:
        print(f"\t(DEVICE 🔁) {msg}")

    print("Sending compressed start command.")
    sent_zstart, zstart_response, periodic_msgs = send_command(ubit_serial, "ZSTART[]", wait_response=True)
    print(f"\t(SENT   ➡️) {sent_zstart}")
    if not zstart_response:
        raise Exception("ZStart command failed.")
    print(f"\t(DEVICE 🔙) {zstart_response}")
    # Parsing the response for the start command
    response_cmd = zstart_response.decode("ascii").split("]", 1)[1]
    if response_cmd != "ZSTART[]":
        raise Exception("ZStart command  failed.")
    if len(periodic_msgs) != 0:
        raise Exception("Periodic messages received, zstart command  failed.")
    print("Start command successful.")

    print("Printing all periodic messages...")
    timeout_time = time.time() + 1  # 1 second timeout
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P"):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
            else:
                print(f"\t(DEVICE ❌) {serial_line[:-1]}")
                #raise Exception(f"Message received is not periodic type: {serial_line}")
            timeout_time = time.time() + 100    # 1 second timeout

        time.sleep(0.001)

    print("(SCRIPT ➡️) Timeout.")

    return 1


if __name__ == "__main__":
    exit_status = main()
    sys.exit(exit_status)
