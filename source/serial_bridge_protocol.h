#pragma once

/** Error codes */
#define SBP_SUCCESS                 (0)
#define SBP_ERROR                   (-1)
#define SBP_ERROR_LEN               (-2)
#define SBP_ERROR_ENCODING          (-3)
#define SBP_ERROR_PROTOCOL_FORMAT   (-4)
#define SBP_ERROR_MSG_TYPE          (-5)
#define SBP_ERROR_CMD_TYPE          (-6)

#define SBP_MSG_SEPARATOR           "\n"

#define SBP_PROTOCOL_VERSION        "1"

/**
 * @brief Enum to list all the different protocol message types.
 */
typedef enum sbp_cmd_type_e {
    SBP_CMD_HANDSHAKE,
    SBP_CMD_START,
    SBP_CMD_STOP,
} sbp_cmd_type_t;

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
int sbp_processHandshake(const char *msg, int msg_len, char *str_buffer, const int str_buffer_len);

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
//int sbp_processCommand(const char *msg, const int msg_len, char *str_buffer, const int str_buffer_len);
// TODO: To a simple pre-define start message to be able to send something quickly
int sbp_processStart(const char *msg, const int msg_len, char *str_buffer, const int str_buffer_len);
