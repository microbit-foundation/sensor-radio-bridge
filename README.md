# Sensor data serial bridge

[![Native Build Status](https://github.com/microbit-foundation/sensor-radio-bridge/actions/workflows/build.yml/badge.svg)](https://github.com/microbit-foundation/sensor-radio-bridge/actions/workflows/build.yml)

Description TBD.


## Installation

### Build dependencies

You need some open source pre-requisites to build this repo. You can either install these tools yourself, or use the docker image provided below.

- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
- [Git](https://git-scm.com)
- [CMake](https://cmake.org/download/)
- [Python 3](https://www.python.org/downloads/)
- [Ninja](https://ninja-build.org/) (Windows only)


### Build steps

- Clone this repository
  ```
  git clone https://github.com/microbit-foundation/sensor-radio-bridge
  cd sensor-radio-bridge
  ```
- In the root of this repository run:
  ```
  python build.py
  ```
- The hex file will be built `MICROBIT.hex` and placed in the root folder.
