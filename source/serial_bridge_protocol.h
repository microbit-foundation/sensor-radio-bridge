#pragma once

/** Error codes */
#define SBP_SUCCESS                 (0)
#define SBP_ERROR                   (-1)
#define SBP_ERROR_LEN               (-2)
#define SBP_ERROR_ENCODING          (-3)
#define SBP_ERROR_PROTOCOL_FORMAT   (-4)
#define SBP_ERROR_MSG_TYPE          (-5)
#define SBP_ERROR_CMD_TYPE          (-5)

#define SBP_MSG_SEPARATOR           ('\n')

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
    int accelerometer_x;
    int accelerometer_y;
    int accelerometer_z;
    int magnetometer_x;
    int magnetometer_y;
    int magnetometer_z;
    bool button_a;
    bool button_b;
    bool button_logo;
    bool button_p0;
    bool button_p1;
    bool button_p2;
    int temperature;
    int light_level;
    int sound_level;
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


int sbp_processHandshake(const char *msg, int msg_len, char *str_buffer, const int str_buffer_len);
