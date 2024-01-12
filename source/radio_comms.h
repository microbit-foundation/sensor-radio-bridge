
#pragma once

#include "cmsis_compiler.h"
#include "main.h"

#define RADIO_SENDER 0

#if CONFIG_DISABLED(RADIO_SENDER)
#define RADIO_BRIDGE 0
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

/**
 * @brief List of commands the bridge can send to the radio senders.
 */
typedef enum {
    RADIO_CMD_FLASH = 0,
    RADIO_CMD_LEN,
} radio_cmd_type_t;

/**
 * @brief Types for callbacks to execute when receiving cmds from the bridge.
 */
typedef void (*radio_cmd_func_t)(uint32_t value);

/**
 * @brief Structure of the command data from bridge to radio sender.
 */
typedef __PACKED_STRUCT radio_cmd_s {
    uint32_t mb_id;
    uint32_t cmd;
    uint32_t value;
} radio_cmd_t;


#if CONFIG_ENABLED(RADIO_BRIDGE)
/**
 * @brief Initialises the radio receiver, with each radio packet being passed
 * to the callback.
 *
 * @param callback The callback to pass the radio data to.
 */
void radiobridge_init(radio_data_callback_t callback, uint8_t radio_frequency);

/**
 * @brief Sends a command to the radio sender.
 *
 * @param mb_id The microbit id of the radio sender to send the command to.
 * @param cmd The command to send.
 * @param value The value to send with the command.
 */
void radiobridge_sendCommand(uint32_t mb_id, radio_cmd_type_t cmd, uint32_t value = 0);
#endif

#if CONFIG_ENABLED(RADIO_SENDER)
/**
 * @brief Runs the main loop for a the radio sender, where it just sends
 * sensor data in an infinite loop.
 */
void radiotx_mainLoop(uint8_t radio_frequency);
#endif
