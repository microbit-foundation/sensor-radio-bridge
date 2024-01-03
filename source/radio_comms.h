
#pragma once

#include "main.h"
#include "serial_bridge_protocol.h"

#define RADIO_SENDER 0

#if CONFIG_DISABLED(RADIO_SENDER)
#define RADIO_RECEIVER 1
#endif

static const uint8_t RADIO_GROUP_DEFAULT = 42;

#if CONFIG_ENABLED(RADIO_SENDER)
void radio_send_main_loop();
#endif
