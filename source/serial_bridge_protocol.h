#pragma once

// TODO: Should only include ManagedString.h, but that doesn't compile
#include "MicroBit.h"


/** Error codes */
#define SBP_SUCCESS                 (0)
#define SBP_ERROR                   (-1)
#define SBP_ERROR_LEN               (-2)
#define SBP_ERROR_ENCODING          (-3)
#define SBP_ERROR_PROTOCOL_FORMAT   (-4)
#define SBP_ERROR_MSG_TYPE          (-5)
#define SBP_ERROR_CMD_TYPE          (-6)
#define SBP_ERROR_CMD_VALUE         (-7)

#define SBP_MSG_SEPARATOR           "\n"

#define SBP_PROTOCOL_VERSION        "1"

typedef struct sbp_state_s sbp_state_t;

/**
 * Each protocol message starts with a single character to identify the type.
 */
typedef enum sbp_msg_type_e {
    SBP_MSG_COMMAND,
    SBP_MSG_RESPONSE,
    SBP_MSG_PERIODIC,
    SBP_MSG_TYPE_LEN,
} sbp_msg_type_t;

const char sbp_msg_type_char[SBP_MSG_TYPE_LEN] = {
    'C',    // SBP_MSG_COMMAND
    'R',    // SBP_MSG_RESPONSE
    'P',    // SBP_MSG_PERIODIC
};

/**
 * @brief List of all the different protocol command/response types.
 */
typedef enum sbp_cmd_type_e {
    SBP_CMD_HANDSHAKE,
    SBP_CMD_RADIOGROUP,
    SBP_CMD_SWVERSION,
    SBP_CMD_HWVERSION,
    SBP_CMD_START,
    SBP_CMD_STOP,
    SBP_CMD_TYPE_LEN,
} sbp_cmd_type_t;

const char* const sbp_cmd_type_str[SBP_CMD_TYPE_LEN] = {
    "HS",       // SBP_CMD_HANDSHAKE
    "RG",       // SBP_CMD_RADIOGROUP
    "SWVER",    // SBP_CMD_SWVERSION
    "HWVER",    // SBP_CMD_HWVERSION
    "START",    // SBP_CMD_START
    "STOP",     // SBP_CMD_STOP
};

/**
 * @brief Structure of function pointers to use as callbacks for each command.
 */
typedef int (*sbp_cmd_callback_t)(sbp_state_t *protocol_state);
typedef struct sbp_cmd_callback_s {
    sbp_cmd_callback_t handshake = NULL;
    sbp_cmd_callback_t radiogroup = NULL;
    sbp_cmd_callback_t swversion = NULL;
    sbp_cmd_callback_t hwversion = NULL;
    sbp_cmd_callback_t start = NULL;
    sbp_cmd_callback_t stop = NULL;
} sbp_cmd_callbacks_t;

/**
 * @brief Structure to hold a protocol command message.
 * 
 * The line pointer is a null terminated string.
 * The id and value start values are the index inside the line string
 * containing this data.
 */
typedef struct sbp_cmd_s {
    sbp_cmd_type_t type;
    const char* line;
    size_t line_len;
    size_t id_start;
    size_t id_len;
    size_t value_start;
    size_t value_len;
} sbp_cmd_t;

/**
 * @brief Flags to hold which sensors are enabled in the protocol.
 */
typedef struct sbp_sensors_s {
    bool accelerometer : 1;
    bool magnetometer : 1;
    bool buttons : 1;
    bool button_logo : 1;
    bool button_pins : 1;
    bool temperature : 1;
    bool light_level : 1;
    bool sound_level : 1;
} sbp_sensors_t;

/**
 * @brief Structure to hold the state of the protocol.
 */
typedef struct sbp_state_s {
    uint8_t radio_group = 0;
    bool send_periodic = true;
    sbp_sensors_t sensors = { };
} sbp_state_t;

/**
 * @brief Structure to hold all the sensor data available in the protocol
 */
typedef struct sbp_sensor_data_s {
    int accelerometer_x = 0;
    int accelerometer_y = 0;
    int accelerometer_z = 0;
    int magnetometer_x = 0;
    int magnetometer_y = 0;
    int magnetometer_z = 0;
    bool button_a = 0;
    bool button_b = 0;
    bool button_logo = 0;
    bool button_p0 = 0;
    bool button_p1 = 0;
    bool button_p2 = 0;
    int temperature = 0;
    int light_level = 0;
    int sound_level = 0;
} sbp_sensor_data_t;


/**
 * @brief Initialises the protocol data structures.
 *
 * @param cmd_callbacks Function pointers for each command.
 * @param protocol_state The protocol state structure to initialize.
 */
void sbp_init(sbp_cmd_callbacks_t *cmd_callbacks, sbp_state_t *protocol_state);

/**
 * @brief Converts sensor data to a protocol serial string representation.
 *
 * This function converts the enabled sensor into a serial string
 * representation ready to be sent.
 *
 * @param enabled_data The configuration of the enabled/disabled sensor data.
 * @param data The actual sensor data.
 * @param str_buffer The buffer to store the serial string representation.
 * @param str_buffer_len The length of the buffer.
 * @return The number of characters written to the buffer, excluding the
 *         null terminator, or a negative number if an error occurred.
 */
int sbp_sensorDataPeriodicStr(const sbp_sensors_t enabled_data,
                              const sbp_sensor_data_t *data,
                              char *str_buffer,
                              int str_buffer_len);

/**
 * @brief Processes a handshake message and prepare the response to send back.
 *
 * @param msg Pointer to the message to process.
 * @param msg_len Length of the message to process.
 * @param str_buffer Buffer to store the response.
 * @param str_buffer_len Length of the buffer to store the response.
 * @return int The number of characters written to the buffer, excluding the
 *        null terminator, or a negative number if an error occurred.
 */
int sbp_processHandshake(const ManagedString& msg, char *str_buffer, const size_t str_buffer_len);

/**
 * @brief Processes a command message, identifies it, and prepares the
 * response to send back.
 *
 * @param msg Pointer to the message to process.
 * @param msg_len Length of the message to process.
 * @param str_buffer Buffer to store the response.
 * @param str_buffer_len Length of the buffer to store the response.
 * @return int The number of characters written to the buffer, excluding the
 *        null terminator, or a negative number if an error occurred.
 */
int sbp_processCommand(const ManagedString& msg, sbp_state_t *protocol_state, char *str_buffer, const size_t str_buffer_len);
