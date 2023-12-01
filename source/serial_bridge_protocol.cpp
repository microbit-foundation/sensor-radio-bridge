#include <stdio.h>
#include <string.h>
#include "serial_bridge_protocol.h"

#define MIN(a, b)       (((a) < (b)) ? (a) : (b))


int sbp_sensorDataPeriodicStr(
    const sbp_sensors_t enabled_data, const sbp_sensor_data_t *data,
    char *str_buffer, const int str_buffer_len
) {
    int serial_data_length = 0;
    static u_int32_t packet_id = 0;

    int cx = snprintf(
        str_buffer + serial_data_length,
        str_buffer_len - serial_data_length,
        "P[%lX]",
        packet_id++
    );
    if (cx > 0) {
        serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
    } else {
        return SBP_ERROR_ENCODING;
    }

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
    if ((str_buffer_len - serial_data_length) >= 2) {
        serial_data_length += snprintf(
            str_buffer + serial_data_length, 2, "\n"
        );
    } else {
        str_buffer[str_buffer_len - 2] = '\n';
        str_buffer[str_buffer_len - 1] = '\0';
        return SBP_ERROR_LEN;
    }

    return serial_data_length;
}


int sbp_processHandshake(
    const char *msg, int msg_len, char *str_buffer, const int str_buffer_len
) {
    // The handshake is always 15 characters long "C[12345678]HK[]"
    if (msg_len != 15) {
        return SBP_ERROR_LEN;
    }

    char *msg_char = (char *)msg;

    // Check the message type is correct
    if (*msg_char++ != 'C') {
        return SBP_ERROR_MSG_TYPE;
    }
    if (*msg_char++ != '[') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Check the ID is valid
    char *id_start = msg_char;
    static const int id_len = 8;
    for (int i = 0; i < id_len; i++, msg_char++) {
        if (!(*msg_char >= '0' && *msg_char <= '9') &&
                !(*msg_char >= 'A' && *msg_char <= 'F') &&
                    !(*msg_char >= 'a' && *msg_char <= 'f')) {
            return SBP_ERROR_PROTOCOL_FORMAT;
        }
    }

    if (*msg_char++ != ']') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Check the command is a handshake
    if (strncmp(msg_char, "HS[]", 4) != 0) {
        return SBP_ERROR_CMD_TYPE;
    }

    // Create the response
    char response[]= "R[XXXXXXXX]HS[]\n";
    if (str_buffer_len < (int)sizeof(response)) {
        return SBP_ERROR_LEN;
    }
    strncpy(str_buffer, response, sizeof(response));
    for (int i = 0; i < id_len; i++) {
        str_buffer[2 + i] = *(id_start + i);
    }

    // Don't include the null terminator in the length
    return sizeof(response) - 1;
}
