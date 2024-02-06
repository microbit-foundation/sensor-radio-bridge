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


def test_cmd(ubit_serial, cmd_name, cmd, expected_response=None, check_value=True, periodic_error=True):
    """Test a command by sending it to the micro:bit and checking the response.

    :param ubit_serial: The serial connection to the micro:bit.
    :param cmd_name: The name of the command being tested.
    :param cmd: The command string to send.
    :param expected_response: The expected response to the command, defaults
                              to the same value as the command itself.
    :param check_value: Set to false to skip checking the response value.
    :param periodic_error: Set to false to skip checking for periodic messages.

    :raises Exception: If the response is not received, the expected response
                       doesn't match, or periodic messages are received.

    :return: Tuple with the response value from the command in string format,
             and a list of any periodic messages received.
    """
    print(f"\nSending {cmd_name} command.")
    sent_cmd, response, periodic_msgs = send_command(ubit_serial, cmd, wait_response=True)
    print(f"\t(SENT   ➡️) {sent_cmd}")
    if not response:
        raise Exception(f"{cmd_name} command failed.")
    print(f"\t(DEVICE 🔙) {response}")

    if expected_response is None:
        expected_response = cmd

    response_cmd = response.decode("ascii").split("]", 1)[1]
    if check_value:
        if response_cmd != expected_response:
            raise Exception(f"{cmd_name} command failed, expected response "
                            f"'{expected_response}' but got '{response_cmd}'")
    else:
        response_start = f'{response_cmd.split("[", 1)[0]}['
        if not response_cmd.startswith(response_start):
            raise Exception(f"{cmd_name} command failed, expected response to "
                            f"start with '{response_start}' but got '{response_cmd}'")
    if periodic_error and len(periodic_msgs) != 0:
        raise Exception(f"Periodic messages received, {cmd_name} command failed.")
    print(f"{cmd_name} command successful.")

    return response_cmd.split("[", 1)[1][:-1], periodic_msgs


def test_radio_frequency(ubit_serial):
    # First read the current radio frequency
    original_radio_frequency, _ = test_cmd(
        ubit_serial, "Radio Frequency (read)", "RF[]", check_value=False
    )
    # Send a different radio frequency
    random_freq = original_radio_frequency
    while random_freq == original_radio_frequency:
        random_freq = random.randint(0, 83)
    test_cmd(ubit_serial, "Radio Frequency (set)", f"RF[{random_freq}]")
    # Now read the radio frequency again
    test_cmd(ubit_serial, "Radio Frequency (read)", "RF[]", f"RF[{random_freq}]")


def test_remote_microbit_id(ubit_serial):
    # First read the configure micro:bit ID, this is likely the default value
    original_remote_mb_id, _ = test_cmd(
        ubit_serial, "Remote micro:bit ID (read)", "RMBID[]", check_value=False
    )
    # Now set the Remote micro:bit ID value received
    test_cmd(ubit_serial, "Remote micro:bit ID (set)", f"RMBID[{original_remote_mb_id}]")
    # Now try to set a random Remote micro:bit ID, which should not work
    random_id = original_remote_mb_id
    while random_id == original_remote_mb_id:
        # signed 32 bit integer min and max values
        random_id = random.randint(pow(2, 31) * -1, pow(2, 31) - 1)
    test_cmd(ubit_serial, "Remote micro:bit ID (set)", f"RMBID[{random_id}]", f"ERROR[2]")
    # And check that the remote micro:bit ID hasn't changed
    test_cmd(ubit_serial, "Remote micro:bit ID (set)", "RMBID[]", f"RMBID[{original_remote_mb_id}]")


def test_start_stop(ubit_serial):
    """
    Test the start command to stream data for 1 second and stop.

    :param ubit_serial: The serial connection to the micro:bit.
    """
    test_cmd(ubit_serial, "Start", "START[PABFMLTS]", "START[]")

    print("Printing all periodic messages received for 1 second...")
    timeout_time = time.time() + 1
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P["):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
            else:
                print(f"\t(DEVICE ❌) {serial_line[:-1]}")
                # raise Exception(f"Message received is not periodic type: {serial_line}")
        time.sleep(0.001)

    _ , periodic_msgs = test_cmd(ubit_serial, "Stop", "STOP[]", periodic_error=False)
    print("Additional periodic messages while processing STOP command:")
    for msg in periodic_msgs:
        print(f"\t(DEVICE 🔁) {msg}")


def test_zstart_stop(ubit_serial):
    """
    Test the compact start command to stream data for 1 second and stop.

    :param ubit_serial: The serial connection to the micro:bit.
    """
    _ , periodic_msgs = test_cmd(ubit_serial, "Start", "ZSTART[]")

    print("Printing all periodic messages received for 1 second...")
    timeout_time = time.time() + 1  # 1 second timeout
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P"):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
            else:
                print(f"\t(DEVICE ❌) {serial_line[:-1]}")
                # raise Exception(f"Message received is not periodic type: {serial_line}")
        time.sleep(0.001)

    _ , periodic_msgs = test_cmd(ubit_serial, "Stop", "STOP[]", periodic_error=False)
    print("Additional periodic messages while processing STOP command:")
    for msg in periodic_msgs:
        print(f"\t(DEVICE 🔁) {msg}")


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
    serial_line = ubit_serial.readline()
    while serial_line:
        print(f'(DEVICE 🔙) {serial_line.decode("ascii")}', end="")
        serial_line = ubit_serial.readline()
    print("Done.")

    ERROR_CODE = 1

    test_cmd(ubit_serial, "Handshake", "HS[]", "HS[1]")

    test_radio_frequency(ubit_serial)
    test_cmd(ubit_serial, "Radio Frequency (error)", f"RF[84]", f"ERROR[{ERROR_CODE}]")

    test_remote_microbit_id(ubit_serial)

    test_cmd(ubit_serial, "ID", "MBID[]", check_value=False)
    test_cmd(ubit_serial, "ID (error)", "MBID[123]", f"ERROR[{ERROR_CODE}]")

    test_cmd(ubit_serial, "Periodic", "PER[20]")
    test_cmd(ubit_serial, "Period (error)", "PER[5]", f"ERROR[{ERROR_CODE}]")

    test_cmd(ubit_serial, "Software Version ", "SWVER[]", "SWVER[0.1.0]")
    # TODO: Check for error if trying to set a value with the SWVER command
    # test_cmd(ubit_serial, "Software Version (error)", "SWVER[0.1.1]", f"ERROR[{ERROR_CODE}]")

    test_cmd(ubit_serial, "Hardware Version", "HWVER[]", "HWVER[2]")
    # TODO: Check for error if trying to set a value with the SWVER command
    # test_cmd(ubit_serial, "Hardware Version (error)", "HWVER[3]", f"ERROR[{ERROR_CODE}]")

    test_start_stop(ubit_serial)
    test_cmd(ubit_serial, "Start (error)", "START[PABFMLTSZ]", f"ERROR[{ERROR_CODE}]")

    test_zstart_stop(ubit_serial)
    # TODO: Once implemented, check error response for ZSTART command

    return 0


if __name__ == "__main__":
    exit_status = main()
    sys.exit(exit_status)
