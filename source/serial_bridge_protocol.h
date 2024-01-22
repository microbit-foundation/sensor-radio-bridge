#pragma once

// TODO: Should only include ManagedString.h, but that doesn't compile
#include "MicroBit.h"

#define SBP_PROTOCOL_VERSION        "1"

/** Default protocol state values */
#define SBP_DEFAULT_RADIO_FREQ      42
#define SBP_DEFAULT_SEND_PERIODIC   false
#define SBP_DEFAULT_PERIODIC_Z      false
#define SBP_DEFAULT_PERIOD_MS       20
#define SBP_DEFAULT_SENSORS         0

/** Error codes */
#define SBP_SUCCESS                 (0)
#define SBP_ERROR                   (-1)
#define SBP_ERROR_LEN               (-2)
#define SBP_ERROR_ENCODING          (-3)
#define SBP_ERROR_PROTOCOL_FORMAT   (-4)
#define SBP_ERROR_MSG_TYPE          (-5)
#define SBP_ERROR_CMD_TYPE          (-6)
#define SBP_ERROR_CMD_VALUE         (-7)
#define SBP_ERROR_NOT_IMPLEMENTED   (-8)

#define SBP_MSG_SEPARATOR           "\n"
#define SBP_MSG_SEPARATOR_LEN       (sizeof(SBP_MSG_SEPARATOR) - 1)

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
    SBP_CMD_RADIOFREQ,
    SBP_CMD_PERIOD,
    SBP_CMD_SWVERSION,
    SBP_CMD_HWVERSION,
    SBP_CMD_START,
    SBP_CMD_ZSTART,
    SBP_CMD_STOP,
    SBP_CMD_TYPE_LEN,
} sbp_cmd_type_t;

const char* const sbp_cmd_type_str[SBP_CMD_TYPE_LEN] = {
    "HS",       // SBP_CMD_HANDSHAKE
    "RF",       // SBP_CMD_RADIOFREQ
    "PER",      // SBP_CMD_PERIOD
    "SWVER",    // SBP_CMD_SWVERSION
    "HWVER",    // SBP_CMD_HWVERSION
    "START",    // SBP_CMD_START
    "ZSTART",   // SBP_CMD_ZSTART
    "STOP",     // SBP_CMD_STOP
};

/** Command value limits */
#define SBP_CMD_RADIO_FREQ_MIN      (0)
#define SBP_CMD_RADIO_FREQ_MAX      (83)
#define SBP_CMD_PERIOD_MIN          (0)
#define SBP_CMD_PERIOD_MAX          (UINT16_MAX)

/**
 * @brief Structure of function pointers to use as callbacks for each command.
 */
typedef int (*sbp_cmd_callback_t)(sbp_state_t *protocol_state);
typedef struct sbp_cmd_callback_s {
    sbp_cmd_callback_t handshake = NULL;
    sbp_cmd_callback_t radiofrequency = NULL;
    sbp_cmd_callback_t period = NULL;
    sbp_cmd_callback_t swversion = NULL;
    sbp_cmd_callback_t hwversion = NULL;
    sbp_cmd_callback_t start = NULL;
    sbp_cmd_callback_t zstart = NULL;
    sbp_cmd_callback_t stop = NULL;
} sbp_cmd_callbacks_t;

/**
 * @brief All the string literals for the different sensor types and subtypes.
 */
#define SBP_SENSOR_STR_ACC          "A"
#define SBP_SENSOR_STR_ACC_X        "AX"
#define SBP_SENSOR_STR_ACC_Y        "AY"
#define SBP_SENSOR_STR_ACC_Z        "AZ"
#define SBP_SENSOR_STR_MAG          "M"
#define SBP_SENSOR_STR_MAG_X        "MX"
#define SBP_SENSOR_STR_MAG_Y        "MY"
#define SBP_SENSOR_STR_MAG_Z        "MZ"
#define SBP_SENSOR_STR_BTN          "B"
#define SBP_SENSOR_STR_BTN_A        "BA"
#define SBP_SENSOR_STR_BTN_B        "BB"
#define SBP_SENSOR_STR_BTN_LOGO     "F"
#define SBP_SENSOR_STR_BTN_PINS     "P"
#define SBP_SENSOR_STR_BTN_P0       "P0"
#define SBP_SENSOR_STR_BTN_P1       "P1"
#define SBP_SENSOR_STR_BTN_P2       "P2"
#define SBP_SENSOR_STR_TEMP         "T"
#define SBP_SENSOR_STR_LIGHT        "L"
#define SBP_SENSOR_STR_SOUND        "S"

/**
 * @brief The sensor types do not include the subtypes
 * (e.g. includes "accelerometer" but not "accelerometer x").
 * This is used for the START command to indicate what data to stream.
 */
typedef enum sbp_sensor_type_e {
    SBP_SENSOR_TYPE_ACC,
    SBP_SENSOR_TYPE_MAG,
    SBP_SENSOR_TYPE_BTN,
    SBP_SENSOR_TYPE_BTN_LOGO,
    SBP_SENSOR_TYPE_BTN_PINS,
    SBP_SENSOR_TYPE_TEMP,
    SBP_SENSOR_TYPE_LIGHT,
    SBP_SENSOR_TYPE_SOUND,
    SBP_SENSOR_TYPE_LEN,
} sbp_sensor_type_t;

const char sbp_sensor_type[SBP_SENSOR_TYPE_LEN] = {
    ((char *)SBP_SENSOR_STR_ACC)[0],
    ((char *)SBP_SENSOR_STR_MAG)[0],
    ((char *)SBP_SENSOR_STR_BTN)[0],
    ((char *)SBP_SENSOR_STR_BTN_LOGO)[0],
    ((char *)SBP_SENSOR_STR_BTN_PINS)[0],
    ((char *)SBP_SENSOR_STR_TEMP)[0],
    ((char *)SBP_SENSOR_STR_LIGHT)[0],
    ((char *)SBP_SENSOR_STR_SOUND)[0],
};

/**
 * @brief Structure to hold a protocol command message.
 * 
 * Only the line pointer is a null terminated string.
 * The ID and Value pointers are not null terminated, as they are substrings
 * inside the line string, but their length is provided.
 */
typedef struct sbp_cmd_s {
    sbp_cmd_type_t type;
    const char* line;
    size_t line_len;
    const char*  id;
    size_t id_len;
    const char*  value;
    size_t value_len;
} sbp_cmd_t;

/**
 * @brief Flags to hold which sensors are enabled in the protocol.
 * This might not be portable to other compilers, as how bitfields are packed
 * is implementation defined, but it's stable for GCC.
 */
typedef union {
    uint8_t raw = 0;
    struct {
        // These need to be in the same order as sbp_sensor_type_e
        bool accelerometer : 1;     // SBP_SENSOR_TYPE_ACC
        bool magnetometer : 1;      // SBP_SENSOR_TYPE_MAG
        bool buttons : 1;           // SBP_SENSOR_TYPE_BTN
        bool button_logo : 1;       // SBP_SENSOR_TYPE_BTN_LOGO
        bool button_pins : 1;       // SBP_SENSOR_TYPE_BTN_PINS
        bool temperature : 1;       // SBP_SENSOR_TYPE_TEMP
        bool light_level : 1;       // SBP_SENSOR_TYPE_LIGHT
        bool sound_level : 1;       // SBP_SENSOR_TYPE_SOUND
    };
} sbp_sensors_t;

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
 * @brief Structure to hold the state of the protocol.
 */
typedef struct sbp_state_s {
    uint8_t radio_frequency = 0;
    bool send_periodic = false;
    bool periodic_compressed = false;
    uint16_t period_ms = 0;
    sbp_sensors_t sensors = { };
} sbp_state_t;


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
 * @brief Converts sensor data to a protocol serial string with the compressed
 * format.
 *
 * @param enabled_data The configuration of the enabled/disabled sensor data.
 * @param data The actual sensor data.
 * @param str_buffer The buffer to store the serial string representation.
 * @param str_buffer_len The length of the buffer.
 * @return The number of characters written to the buffer, excluding the
 *        null terminator, or a negative number if an error occurred.
 */
int sbp_compressedSensorDataPeriodicStr(const sbp_sensors_t enabled_data,
                                        const sbp_sensor_data_t *data,
                                        char *str_buffer,
                                        int str_buffer_len);

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
