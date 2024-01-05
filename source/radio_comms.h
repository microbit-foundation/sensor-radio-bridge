
#pragma once

#include "cmsis_compiler.h"
#include "main.h"

#define RADIO_SENDER 0

#if CONFIG_DISABLED(RADIO_SENDER)
#define RADIO_RECEIVER 1
#endif

static const uint8_t RADIO_FREQ_DEFAULT = 42;

/**
 * @brief Data sent over radio.
 */
typedef __PACKED_STRUCT radio_sensor_data_s {
    uint32_t mb_id;
    int32_t accelerometer_x;
    int32_t accelerometer_y;
    int32_t accelerometer_z;
    uint8_t button_a;
    uint8_t button_b;
    uint8_t button_logo;
} radio_sensor_data_t;

/**
 * @brief Type definition for the callback with the received radio data.
 *
 * @param sensor_data Pointer to the radio data received.
 *                    Data must be copied from this pointer as it will be
 *                    destroyed after the callback.
 */
typedef void (*radio_data_callback_t)(radio_sensor_data_t *sensor_data);


#if CONFIG_ENABLED(RADIO_RECEIVER)
/**
 * @brief Initialises the radio receiver, with each radio packet being passed
 * to the callback.
 *
 * @param callback The callback to pass the radio data to.
 */
void radio_receive_init(radio_data_callback_t callback, uint8_t radio_frequency);
#endif

#if CONFIG_ENABLED(RADIO_SENDER)
/**
 * @brief Runs the main loop for a the radio sender, where it just sends
 * sensor data in an infinite loop.
 */
void radio_send_main_loop(uint8_t radio_frequency);
#endif
