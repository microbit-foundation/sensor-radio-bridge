#pragma once

#include "MicroBit.h"

extern MicroBit uBit;

#define PROJECT_VERSION "0.3.0"

// The project type build flags will be configured via the build_all.py script
// no need to edit any of the macros below
#define BUILD_LOCAL_SENSORS     6
#define BUILD_LOCAL_SENSORS_DEV 1
#define BUILD_RADIO_REMOTE      2
#define BUILD_RADIO_REMOTE_DEV  3
#define BUILD_RADIO_BRIDGE      4
#define BUILD_RADIO_BRIDGE_DEV  5


#ifndef PROJECT_BUILD_TYPE
#define PROJECT_BUILD_TYPE      BUILD_LOCAL_SENSORS_DEV
#endif

#if PROJECT_BUILD_TYPE == BUILD_LOCAL_SENSORS
#define RADIO_REMOTE            0
#define RADIO_BRIDGE            0
#define DEV_MODE                0
#define IMG_RUNNING             IMG_SQUARE
#elif PROJECT_BUILD_TYPE == BUILD_LOCAL_SENSORS_DEV
#define RADIO_REMOTE            0
#define RADIO_BRIDGE            0
#define DEV_MODE                1
#define IMG_RUNNING             IMG_STAIRS
#elif PROJECT_BUILD_TYPE == BUILD_RADIO_REMOTE
#define RADIO_REMOTE            1
#define RADIO_BRIDGE            0
#define DEV_MODE                0
#define IMG_RUNNING             IMG_HAPPY
#elif PROJECT_BUILD_TYPE == BUILD_RADIO_REMOTE_DEV
#define RADIO_REMOTE            1
#define RADIO_BRIDGE            0
#define DEV_MODE                1
#define IMG_RUNNING             IMG_HAPPY
#elif PROJECT_BUILD_TYPE == BUILD_RADIO_BRIDGE
#define RADIO_REMOTE            0
#define RADIO_BRIDGE            1
#define DEV_MODE                0
#define IMG_RUNNING             IMG_DIAMOND
#elif PROJECT_BUILD_TYPE == BUILD_RADIO_BRIDGE_DEV
#define RADIO_REMOTE            0
#define RADIO_BRIDGE            1
#define DEV_MODE                1
#define IMG_RUNNING             IMG_DIAMOND
#else
#error "Invalid build type"
#endif

#define IMG_WAITING             IMG_DOT
