#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Simple test script to flash two micro:bits with the remote and bridge
programmes, and configure the bridge micro:bit to communicate with the
remote micro:bit.
"""
import os
import time
import argparse

from serial import Serial, PARITY_NONE, STOPBITS_ONE

from test_protocol import find_microbit_serial_port, test_cmd


def flash_file(mb_path, hex_path):
    with open(hex_path, "rb") as hex_file:
        hex_bytes = hex_file.read()
    with open(os.path.join(mb_path, "input.hex"), "wb") as hex_write:
        hex_write.write(hex_bytes)
        hex_write.flush()
        os.fsync(hex_write.fileno())
    time.sleep(0.1)


def main(mb_path, remote_mb_file_path, bridge_mb_file_path):
    # First step is to flash the remote micro:bit
    print("Flashing remote micro:bit...")
    flash_file(mb_path, remote_mb_file_path)

    # And get its unique ID
    print("Reading the remote micro:bit ID... (not yet implemented)")
    # TODO: Currently hardcoded to a known micro:bit value
    remote_microbit_id = -1676136584

    input("Unplug remote micro:bit and plug bridge micro:bit\nPress enter to continue...")
    # Wait for the bridge micro:bit to be mounted
    if not os.path.isdir(mb_path):
        print("Waiting for bridge micro:bit to be mounted...")
        time.sleep(1)

    # Now flash second micro:bit
    print("Flashing bridge micro:bit...")
    flash_file(mb_path, bridge_mb_file_path)

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

    # Send remote micro:bit ID, but first check it's not set already
    print("Configuring bridge micro:bit...")
    bridge_mb_id, _ = test_cmd(ubit_serial, "Read bridge ID", "MBID[]", check_value=False)
    received_remote_mb_id, _ = test_cmd(ubit_serial, "Read remote ID", "RMBID[]", check_value=False)
    if bridge_mb_id != received_remote_mb_id:
        raise Exception("Remote micro:bit ID already set to a value.")
    test_cmd(ubit_serial, "Set remote ID", f"RMBID[{remote_microbit_id}]")
    received_remote_mb_id, _ = test_cmd(ubit_serial, "Read remote ID", "RMBID[]", check_value=False)
    if received_remote_mb_id != str(remote_microbit_id):
        raise Exception("Remote micro:bit ID not set correctly.")

    # Now check the radio frequency has been correctly configured
    radio_frequency, _ = test_cmd(ubit_serial, "Read radio frequency", "RF[]", check_value=False)
    if int(radio_frequency) != ((remote_microbit_id & 0xffffffff) % 83):
        print(radio_frequency)
        print((remote_microbit_id & 0xffffffff) % 83)
        raise Exception("Radio frequency not internally set correctly in device.")

    # And not start streaming
    test_cmd(ubit_serial, "Start", "START[AB]", "START[]")
    while True:
        serial_line = ubit_serial.readline()
        if len(serial_line) > 0:
            if serial_line.startswith(b"P["):
                print(f"\t(DEVICE 🔁) {serial_line[:-1]}")
            else:
                print(f"\t(DEVICE ❌) {serial_line[:-1]}")
                # raise Exception(f"Message received is not periodic type: {serial_line}")
        time.sleep(0.001)

    return 0


if __name__ == "__main__":
    # Add two command line arguments for the file paths, --remote and --bridge
    parser = argparse.ArgumentParser(description="Flash and configure two micro:bits.")
    parser.add_argument("--microbit", type=str, default="/Volumes/MICROBIT/",
                        help="Path to micro:bit MICROBIT USB drive.")
    parser.add_argument("--remote", type=str, default="radio-remote-dev.hex",
                        help="Remote micro:bit hex file path.")
    parser.add_argument("--bridge", type=str, default="radio-bridge-dev.hex",
                        help="Bridge micro:bit hex file path.")
    args = parser.parse_args()

    print(f"micro:bit drive path: {args.microbit}")
    print(f"Remote micro:bit file: {args.remote}")
    print(f"Bridge micro:bit file: {args.bridge}")

    main(args.microbit, args.remote, args.bridge)
