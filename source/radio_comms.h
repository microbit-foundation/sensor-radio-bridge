
#pragma once

#include "cmsis_compiler.h"
#include "main.h"

#define RADIO_SENDER 0

#if CONFIG_DISABLED(RADIO_SENDER)
#define RADIO_BRIDGE 1
#endif

static const uint8_t RADIO_FREQ_DEFAULT = 42;

/**
 * @brief List of radio packet types
 */
typedef enum {
    RADIO_PKT_SENSOR_DATA = 0,
    RADIO_PKT_CMD,
    RADIO_PKT_RESPONSE,
    RADIO_PKT_TYPE_LEN,
} radio_packet_type_t;

typedef enum {
    RADIO_CMD_INVALID = 0,
    RADIO_CMD_HELLO,
    RADIO_CMD_BLINK,
    RADIO_CMD_DISPLAY,
    RADIO_CMD_TYPE_LEN,
} radio_cmd_type_t;

typedef __PACKED_STRUCT radio_sensor_data_s {
    int32_t accelerometer_x;
    int32_t accelerometer_y;
    int32_t accelerometer_z;
    uint8_t button_a;
    uint8_t button_b;
    uint8_t button_logo;
    uint8_t padding;
} radio_sensor_data_t;

typedef __PACKED_STRUCT radio_cmd_s {
    int32_t unused1;
    int32_t unused2;
    int32_t unused3;
    int32_t unused4;
} radio_cmd_t;

typedef __PACKED_STRUCT radio_cmd_display_s {
    uint8_t row1;
    uint8_t row2;
    uint8_t row3;
    uint8_t row4;
    uint8_t row5;
    uint8_t padding1;
    uint16_t padding2;
    uint32_t padding3;
    uint32_t padding4;
} radio_cmd_display_t;

/**
 * @brief Data sent over radio.
 */
typedef __PACKED_STRUCT radio_packet_s {
    uint8_t packet_type;
    uint8_t cmd_type;
    uint16_t padding;
    uint32_t id;
    uint32_t mb_id;
    union {
        radio_cmd_t cmd_data;
        radio_cmd_display_s cmd_display;
        radio_sensor_data_t sensor_data;
    };
} radio_packet_t;


/**
 * @brief Type definition for the callback with the received radio data.
 *
 * @param sensor_data Pointer to the radio data received.
 *                    Data must be copied from this pointer as it will be
 *                    destroyed after the callback.
 */
typedef void (*radio_data_callback_t)(radio_packet_t *radio_packet);

/**
 * @brief Types for callbacks to execute when receiving cmds from the bridge.
 */
typedef void (*radio_cmd_func_t)(radio_cmd_t *value);


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
void radiobridge_sendCommand(uint32_t mb_id, radio_cmd_type_t cmd, radio_cmd_t *value = NULL);
#endif

#if CONFIG_ENABLED(RADIO_SENDER)
/**
 * @brief Runs the main loop for a the radio sender, where it just sends
 * sensor data in an infinite loop.
 */
void radiotx_mainLoop(uint8_t radio_frequency);
#endif
