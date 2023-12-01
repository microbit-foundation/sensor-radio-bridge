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
            str_buffer + serial_data_length, 2, SBP_MSG_SEPARATOR
        );
    } else {
        serial_data_length += snprintf(
            str_buffer + (str_buffer_len - 2), 2, SBP_MSG_SEPARATOR
        );
        return SBP_ERROR_LEN;
    }

    return serial_data_length;
}


int sbp_processHandshake(
    const char *msg, const int msg_len, char *str_buffer, const int str_buffer_len
) {
    // The handshake is always 15 characters long "C[12345678]HK[]"
    if (msg_len != 15) {
        return SBP_ERROR_LEN;
    }

    // Check the message type is correct
    if (*msg++ != 'C') {
        return SBP_ERROR_MSG_TYPE;
    }
    if (*msg++ != '[') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Check the ID is valid
    const char *cmd_id_start = msg;
    static const int cmd_id_len = 8;
    for (int i = 0; i < cmd_id_len; i++, msg++) {
        if (!(*msg >= '0' && *msg <= '9') &&
                !(*msg >= 'A' && *msg <= 'F') &&
                    !(*msg >= 'a' && *msg <= 'f')) {
            return SBP_ERROR_PROTOCOL_FORMAT;
        }
    }

    if (*msg++ != ']') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Check the command is a handshake, it never has any data inside
    if (strncmp(msg, "HS[]", 4) != 0) {
        return SBP_ERROR_CMD_TYPE;
    }

    // Create the response with the command ID and the protocol version
    char response[]= "R[XXXXXXXX]HS[" SBP_PROTOCOL_VERSION "]" SBP_MSG_SEPARATOR;
    if (str_buffer_len < (int)sizeof(response)) {
        return SBP_ERROR_LEN;
    }
    strncpy(str_buffer, response, str_buffer_len);
    for (int i = 0; i < cmd_id_len; i++) {
        str_buffer[2 + i] = *(cmd_id_start + i);
    }

    // Don't include the null terminator in the length
    return sizeof(response) - 1;
}

// TODO: To a simple pre-define start message to be able to send something quickly
// int sbp_processCommand(
int sbp_processStart(
    const char *msg, const int msg_len, char *str_buffer, const int str_buffer_len
) {
    // The handshake is always 15 characters long "C[12345678]HK[]"
    if (msg_len != 24) {
        return SBP_ERROR_LEN;
    }

    // Check the message type is correct
    if (*msg++ != 'C') {
        return SBP_ERROR_MSG_TYPE;
    }
    if (*msg++ != '[') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Check the ID is valid
    const char *cmd_id_start = msg;
    static const int cmd_id_len = 8;
    for (int i = 0; i < cmd_id_len; i++, msg++) {
        if (!(*msg >= '0' && *msg <= '9') &&
                !(*msg >= 'A' && *msg <= 'F') &&
                    !(*msg >= 'a' && *msg <= 'f')) {
            return SBP_ERROR_PROTOCOL_FORMAT;
        }
    }

    if (*msg++ != ']') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Check the command is a handshake, it never has any data inside
    if (strncmp(msg, "START[A,B,BL]", 13) != 0) {
        return SBP_ERROR_CMD_TYPE;
    }

    // Create the response with the command ID and the protocol version
    char response[]= "R[XXXXXXXX]START[]" SBP_MSG_SEPARATOR;
    if (str_buffer_len < (int)sizeof(response)) {
        return SBP_ERROR_LEN;
    }
    strncpy(str_buffer, response, str_buffer_len);
    for (int i = 0; i < cmd_id_len; i++) {
        str_buffer[2 + i] = *(cmd_id_start + i);
    }

    // Don't include the null terminator in the length
    return sizeof(response) - 1;
}