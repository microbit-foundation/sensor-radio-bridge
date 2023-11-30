#include <stdio.h>
#include "serial_bridge_protocol.h"

#define MIN(a, b)       (((a) < (b)) ? (a) : (b))


int sbp_sensorDataSerialStr(
    const sbp_sensors_t enabled_data, const sbp_sensor_data_t *data,
    char *str_buffer, const int str_buffer_len
) {
    int serial_data_length = 0;

    if (enabled_data.accelerometer) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "AX[%d]AY[%d]AZ[%d]",
            data->accelerometer_x,
            data->accelerometer_y,
            data->accelerometer_z
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.magnetometer) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "CX[%d]CY[%d]CZ[%d]",
            data->magnetometer_x,
            data->magnetometer_y,
            data->magnetometer_z
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.buttons) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "BA[%d]BB[%d]",
            data->button_a,
            data->button_b
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.button_logo) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "BL[%d]",
            data->button_logo
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.button_pins) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "B0[%d]B1[%d]B2[%d]",
            data->button_p0,
            data->button_p1,
            data->button_p2
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.temperature) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "T[%d]",
            data->temperature
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.light_level) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "L[%d]",
            data->light_level
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.sound_level) {
        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "S[%d]",
            data->sound_level
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }

    // Ensure the string ends with a carriage return and new line
    if ((str_buffer_len - serial_data_length) >= 3) {
        serial_data_length += snprintf(
            str_buffer + serial_data_length, 3, "\r\n"
        );
    } else {
        str_buffer[str_buffer_len - 3] = '\r';
        str_buffer[str_buffer_len - 2] = '\n';
        str_buffer[str_buffer_len - 1] = '\0';
        return SBP_ERROR_LEN;
    }

    return serial_data_length;
}
