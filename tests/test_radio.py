#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Simple test script to flash two micro:bits with the remote and bridge
programmes, and configure the bridge micro:bit to communicate with the
remote micro:bit.
"""
import os
import sys
import time
import argparse

THIS_FOLDER = os.path.dirname(os.path.abspath(__file__))
PARENT_FOLDER = os.path.dirname(THIS_FOLDER)
sys.path.append(THIS_FOLDER)

from test_protocol import connect_serial, test_cmd


def flash_file(mb_path, hex_path):
    with open(hex_path, "rb") as hex_file:
        hex_bytes = hex_file.read()

    # Wait for the  micro:bit to be mounted
    print("Waiting for micro:bit to be mounted...")
    if not os.path.isfile(os.path.join(mb_path, "DETAILS.TXT")):
        time.sleep(1)
        print("Still waiting...")
    time.sleep(2)
    print("Flashing...")

    with open(os.path.join(mb_path, "radio-programme.hex"), "wb") as hex_write:
        hex_write.write(hex_bytes)
        hex_write.flush()
        os.fsync(hex_write.fileno())
    time.sleep(0.1)


def main(mb_path, remote_mb_file_path, bridge_mb_file_path):
    # First step is to flash the remote micro:bit
    print("\nFlashing remote micro:bit:")
    flash_file(mb_path, remote_mb_file_path)
    print("Done.")

    # And get its unique ID
    print("\nReading the remote micro:bit ID... (not yet implemented)")
    # TODO: Currently hardcoded to a known micro:bit value
    remote_microbit_id = 3130793464
    print("Done.")

    input("\n👉 Unplug remote micro:bit and plug bridge micro:bit\n⌨️ Press enter to continue...")

    # Now flash second micro:bit
    print("\nFlashing bridge micro:bit:")
    flash_file(mb_path, bridge_mb_file_path)
    print("Done.")

    ubit_serial = connect_serial()

    # Send remote micro:bit ID, but first check it's not set already
    print("\nConfiguring bridge micro:bit:")
    bridge_mb_id, _ = test_cmd(ubit_serial, "Read bridge ID", "MBID[]", check_value=False)
    received_remote_mb_id, _ = test_cmd(ubit_serial, "Read remote ID", "RMBID[]", check_value=False)
    if bridge_mb_id != received_remote_mb_id:
        raise Exception("Remote micro:bit ID already set to a value.")

    test_cmd(ubit_serial, "Set remote ID", f"RMBID[{remote_microbit_id}]")
    received_remote_mb_id, _ = test_cmd(ubit_serial, "Read remote ID", "RMBID[]", check_value=False)
    if received_remote_mb_id != str(remote_microbit_id):
        raise Exception("Remote micro:bit ID not set correctly.")

    # Check that the remote micro:bit ID has been set saved to NVM
    input("\n👉 Press the reset button on the bridge micro:bit\n⌨️ Press enter to continue...")
    received_remote_mb_id, _ = test_cmd(ubit_serial, "Read remote ID", "RMBID[]", check_value=False)
    if received_remote_mb_id != str(remote_microbit_id):
        raise Exception("Remote micro:bit ID not set correctly.")
    print("Correctly retrieved remote micro:bit ID from NVM.")

    # Now check the radio frequency has been correctly configured
    radio_frequency, _ = test_cmd(ubit_serial, "Read radio frequency", "RF[]", check_value=False)
    if int(radio_frequency) != ((remote_microbit_id & 0xffffffff) % 83):
        print(radio_frequency)
        print((remote_microbit_id & 0xffffffff) % 83)
        raise Exception("Radio frequency not internally set correctly in device.")

    # Start streaming and check that after stop nothing else is sent
    print("\nReceiving periodic data for one second and stop:")
    test_cmd(ubit_serial, "Start", "ZSTART[AB]", "ZSTART[]")
    periodic_msg_received = False
    timeout_time = time.time() + 1
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P") and len(serial_line) == 14:
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
                periodic_msg_received = True
            else:
                raise Exception(f"Message received is not periodic type: {serial_line}")
        time.sleep(0.001)
    test_cmd(ubit_serial, "Stop", "STOP[]", check_value=False)
    if not periodic_msg_received:
        raise Exception("No periodic message received.")

    # No additional periodic messages should be received
    input("\n👉Disconnect battery pack from remote micro:bit\n⌨️ Press enter to continue...")
    _ , peridic_msgs = test_cmd(ubit_serial, "Start", "START[AB]", "START[]")
    if len(peridic_msgs) > 0:
        raise Exception(f"Received periodic message, which should have not been sent: {peridic_msgs}")
    serial_line = ubit_serial.readline()
    if len(serial_line) > 0:
        raise Exception(f"Received unexpected message, which should have not been sent: {serial_line}")
    print("Done, no unexpected periodic messages received.")

    # Now just stream indefinitely
    input("\n👉 Connect the battery pack to the remote micro:bit\n⌨️ Press enter to continue...")
    timeout_time = time.time() + 1
    periodic_msg_received = False
    while time.time() < timeout_time:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P["):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
                periodic_msg_received = True
            else:
                raise Exception(f"Message received is not periodic type: {serial_line}")
        time.sleep(0.001)
    if not periodic_msg_received:
        raise Exception("No periodic message received.")

    print("\n✅ All tests passed.")

    return 0


if __name__ == "__main__":
    # Add two command line arguments for the file paths, --remote and --bridge
    remote_hex_default = os.path.join(PARENT_FOLDER, "radio-remote-dev.hex")
    bridge_hex_default = os.path.join(PARENT_FOLDER, "radio-bridge-dev.hex")
    parser = argparse.ArgumentParser(description="Flash and configure two micro:bits.")
    parser.add_argument("--microbit", type=str, default="/Volumes/MICROBIT/",
                        help="Path to micro:bit MICROBIT USB drive.")
    parser.add_argument("--remote", type=str, default=remote_hex_default,
                        help="Remote micro:bit hex file path.")
    parser.add_argument("--bridge", type=str, default=bridge_hex_default,
                        help="Bridge micro:bit hex file path.")
    args = parser.parse_args()

    print(f"micro:bit drive path: {args.microbit}")
    print(f"Remote micro:bit file: {args.remote}")
    print(f"Bridge micro:bit file: {args.bridge}")

    exit_code = main(args.microbit, args.remote, args.bridge)
    sys.exit(exit_code)
