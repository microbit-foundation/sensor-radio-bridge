
#pragma once

#include "cmsis_compiler.h"
#include "main.h"

#define MAX_RADIO_FREQUENCY 83

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
    int32_t unused[4];
} radio_cmd_t;

typedef __PACKED_STRUCT radio_cmd_display_s {
    uint8_t row1;
    uint8_t row2;
    uint8_t row3;
    uint8_t row4;
    uint8_t row5;
    uint8_t padding[11];
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

static_assert(sizeof(radio_cmd_t) == 16, "radio_cmd_t should be 16 bytes");
static_assert(sizeof(radio_cmd_t) == sizeof(radio_cmd_display_t),
    "radio_cmd_display_t should be same size as radio_cmd_t");
static_assert(sizeof(radio_sensor_data_s) == sizeof(radio_sensor_data_s),
    "radio_sensor_data_s should be same size as radio_cmd_t");
static_assert(sizeof(radio_packet_t) == 28, "radio_packet_t should be 28 bytes");

/**
 * @brief Type definition for the callback with the received radio data.
 *
 * @param sensor_data Pointer to the radio data received.
 *                    Data must be copied from this pointer as it will be
 *                    destroyed after the callback.
 */
typedef void (*radio_data_callback_t)(const radio_packet_t *radio_packet);


#if CONFIG_ENABLED(RADIO_BRIDGE)
/**
 * @brief Initialises the radio receiver, with each radio packet being passed
 * to the callback.
 *
 * @param callback The callback to pass the radio data to.
 */
void radiobridge_init(const radio_data_callback_t callback, const uint8_t radio_frequency);

/**
 * @brief Changes the radio frequency of the remote micro:bits and the
 *        bridge micro:bits.
 * 
 * TODO: Currently this function only changes the bridge radio frequency,
 *       need to design a plan to change the remote micro:bit as well.
 *
 * @param radio_frequency The new radio frequency to set.
 *
 * @return MICROBIT_OK if the radio frequency was set successfully to all
 *         micro:bits, or a MICROBIT error value otherwise.
 */
int radiobridge_setRadioFrequencyAllMbs(const uint8_t radio_frequency);

/**
 * @brief Sends a command to the radio sender.
 *
 * @param mb_id The microbit id of the radio sender to send the command to.
 * @param cmd The command to send.
 * @param value The value to send with the command.
 */
void radiobridge_sendCommand(const uint32_t mb_id, const radio_cmd_type_t cmd, const radio_cmd_t *value = NULL);

/**
 * @brief Sets the provided remote micro:bit ID as the active one.
 * 
 * @param mb_id The micro:bit ID to set as active.
 */
void radiobridge_setActiveRemoteMbId(const uint32_t mb_id) ;

/**
 * @return The micro:bit ID for the active remote micro:bit.
 */
uint32_t radiobridge_getActiveRemoteMbId();

/**
 * @brief Updates the list of micro:bit IDs that have been seen recently.
 * 
 * @param mb_id The micro:bit ID to mark as active.
 */
void radiobridge_updateRemoteMbIds(const uint32_t mb_id);

/**
 * @brief Switches the active remote micro:bit to the next available remote
 * micro:bit.
 * On switch it sends a command to the new active micro:bit to flash the
 * display.
 */
void radiobridge_switchNextRemoteMicrobit();
#endif

#if CONFIG_ENABLED(RADIO_REMOTE)
/**
 * @brief Runs the main loop for a the radio sender, where it just sends
 * sensor data in an infinite loop.
 */
void radiotx_mainLoop();
#endif

/**
 * @brief Calculates the radio frequency based on the micro:bit unique ID.
 *
 * @param id The micro:bit ID to get the frequency from.
 *
 * @return The frequency value, between 0 and MAX_RADIO_FREQUENCY.
 */
inline uint8_t radio_getFrequencyFromId(const uint32_t id) {
    return id % MAX_RADIO_FREQUENCY;
}
