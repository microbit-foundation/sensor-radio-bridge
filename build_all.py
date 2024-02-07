#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
A python script that creates the following CODAL builds from the same codebase,
with different preprocessor defines:
- LOCAL_SENSORS
- RADIO_BRIDGE
- RADIO_REMOTE
- RADIO_BRIDGE_DEV
- RADIO_REMOTE_DEV
"""
import os
import sys
import shutil
from pathlib import Path

from utils.python.codal_utils import build


ROOT_DIR = os.path.dirname(os.path.abspath(__file__))


def clean_build(root_dir, hex_name, build_flags={}):
    build_path = Path(root_dir) / 'build'
    hex_path = Path(root_dir) / 'MICROBIT.hex'
    bin_path = Path(root_dir) / 'MICROBIT.bin'
    new_hex_path = Path(root_dir) / hex_name

    # Clean the build directory
    shutil.rmtree(build_path, ignore_errors=True)
    if os.path.isfile(hex_path):
        os.remove(hex_path)
    if os.path.isfile(bin_path):
        os.remove(bin_path)

    # CODAL build system needs the build directory to be the cwd
    os.mkdir(build_path)
    os.chdir(build_path)

    # Build CODAL with the given build flags as environmental variables
    os.environ["CXXFLAGS"] = ""
    for flag, value in build_flags.items():
        os.environ["CXXFLAGS"] += f" -D{flag}={value}"
    print(f"\n{'#' * 80}\n# Building: {hex_name}\n{'#' * 80}")
    # build() exits if any system call returns a non-zero exit code
    build(clean=True, verbose=False, parallelism=os.cpu_count())

    # Rename the hex file to hex_name
    os.rename(hex_path, new_hex_path)


def main():
    os.chdir(ROOT_DIR)

    # First check if the hex files already exist to avoid overwriting them
    for hex_file in ["local-sensors.hex", "radio-bridge.hex", "radio-remote.hex"]:
        if os.path.isfile(hex_file):
            print(f"{hex_file} hex files already exist, stopping build.")
            return 1

    clean_build(ROOT_DIR, "radio-remote-dev.hex", build_flags={"PROJECT_BUILD_TYPE": "3"})
    clean_build(ROOT_DIR, "radio-bridge-dev.hex", build_flags={"PROJECT_BUILD_TYPE": "5"})
    clean_build(ROOT_DIR, "radio-remote.hex", build_flags={"PROJECT_BUILD_TYPE": "2"})
    clean_build(ROOT_DIR, "radio-bridge.hex", build_flags={"PROJECT_BUILD_TYPE": "4"})
    clean_build(ROOT_DIR, "local-sensors.hex", build_flags={"PROJECT_BUILD_TYPE": "1"})


if __name__ == '__main__':
    sys.exit(main())
