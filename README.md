# Sensor data serial bridge

[![Native Build Status](https://github.com/microbit-foundation/sensor-radio-bridge/actions/workflows/build.yml/badge.svg)](https://github.com/microbit-foundation/sensor-radio-bridge/actions/workflows/build.yml)

This is a simple initial version of a micro:bit C++ programme to receive
accelerometer data via radio and bridge it to a computer via serial.

In its current initial state it needs some basic configuration and it
sends the data from the current micro:bit in the following format:

```
P[12345678]AX[408]AY[748]AZ[-1288]BA[0]BB[1]
P[12345679]AX[520]AY[-172]AZ[-448]BA[0]BB[1]
P[1234567A]AX[-412]AY[436]AZ[340]BA[0]BB[1]
P[1234567B]AX[-408]AY[504]AZ[-788]BA[0]BB[1]
P[1234567C]AX[-480]AY[592]AZ[-736]BA[0]BB[1]
```

Where:
- P = Unique identifier of the protocol message
- AX = Accelerometer X axis
- AY = Accelerometer X axis
- AZ = Accelerometer X axis
- BA = Button A state
- BB = Button B state

More information about the protocol will be defined soon.


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
  python build_all.py
  ```
- The multiple hex files will be placed in the root folder.

