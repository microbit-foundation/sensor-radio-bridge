#pragma once


#define SBP_SUCCESS                 (0)
#define SBP_ERROR                   (-1)
#define SBP_ERROR_LEN               (-2)
#define SBP_ERROR_ENCODING          (-3)

/**
 * @brief Flags to hold which sensors are enabled in the protocol.
 */
typedef struct sbp_sensors_s {
    bool accelerometer : 1;
    bool compass : 1;
    bool buttons : 1;
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
    int compass_x;
    int compass_y;
    int compass_z;
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
 * @brief Generate a string of sensor data to send via serial
 * 
 * @param enabled_data 
 * @param data 
 * @param str_buffer Pointer to the char buffer to fill.
 * @param str_buffer_len Total buffer length.
 * @return int Number of characters written to the buffer, excluding the
 *             null terminator. A negative number indicates an error.
 */
int sbp_sensorDataSerialStr(const sbp_sensors_t *enabled_data,
                            const sbp_sensor_data_t *data,
                            char *str_buffer,
                            int str_buffer_len);
