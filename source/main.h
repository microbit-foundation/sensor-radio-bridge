#pragma once

#include "MicroBit.h"

extern MicroBit uBit;

#define PROJECT_VERSION "0.1.0"

// The project type build flags will be configured via the build_all.py script
// no need to edit any of the macros below
#define BUILD_LOCAL_SENSORS     1
#define BUILD_RADIO_BRIDGE      2
#define BUILD_RADIO_REMOTE      3

#ifndef PROJECT_BUILD_TYPE
#define PROJECT_BUILD_TYPE      BUILD_LOCAL_SENSORS
#endif

#if PROJECT_BUILD_TYPE == BUILD_LOCAL_SENSORS
#define RADIO_REMOTE            0
#define RADIO_BRIDGE            0
#elif PROJECT_BUILD_TYPE == BUILD_RADIO_BRIDGE
#define RADIO_REMOTE            0
#define RADIO_BRIDGE            1
#elif PROJECT_BUILD_TYPE == BUILD_RADIO_REMOTE
#define RADIO_REMOTE            1
#define RADIO_BRIDGE            0
#else
#error "Invalid build type"
#endif
