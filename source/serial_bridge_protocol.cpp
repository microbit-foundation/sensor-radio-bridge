#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "serial_bridge_protocol.h"

#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

static size_t CMD_MAX_LEN = 0;
static sbp_cmd_callbacks_t cmd_cbk = { };

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------

static int intFromCommandValue(const char *value_str, const size_t value_str_len, int *value) {
    // Convert into a null terminated string
    char value_str_terminated[value_str_len + 1];
    for (size_t i = 0; i < value_str_len; i++) {
        value_str_terminated[i] = value_str[i];
    }
    value_str_terminated[value_str_len] = '\0';

    // Convert value_str into an integer
    *value = atoi(value_str_terminated);
    if (*value == 0 && !(value_str_len == 1 && value_str[0] == '0')) {
        return SBP_ERROR_CMD_VALUE;
    }
    return SBP_SUCCESS;
}

// ----------------------------------------------------------------------------
// PRIVATE FUNCTIONS ----------------------------------------------------------
// ----------------------------------------------------------------------------

/**
 * @brief Parse a protocol message as a command.
 *
 * @param msg String of the protocol message to parse.
 * @param msg_len The length of the protocol message string
 *                (not including null terminator).
 * @param cmd Pointer to the command structure to populate.
 * @return Success or error code.
 */
static int sbp_parseCommand(const char *msg, const size_t msg_len, sbp_cmd_t *cmd) {
    const char *msg_start = msg;
    sbp_cmd_type_t cmd_type = SBP_CMD_TYPE_LEN;
    size_t id_start = 0;
    size_t id_len = 8;
    size_t value_start = 0;
    size_t value_len = 0;

    // The first character should be the message type, in this case the
    // command type, followed by a '['
    if (*msg++ != sbp_msg_type_char[SBP_MSG_COMMAND]) {
        return SBP_ERROR_MSG_TYPE;
    }
    if (*msg++ != '[') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // The next 8 characters should be the command ID with a valid hex number
    // and end with a ']'
    id_start = msg - msg_start;
    for (size_t i = 0; i < id_len; i++, msg++) {
        if (!(*msg >= '0' && *msg <= '9') &&
                !(*msg >= 'A' && *msg <= 'F') &&
                    !(*msg >= 'a' && *msg <= 'f')) {
            return SBP_ERROR_PROTOCOL_FORMAT;
        }
    }
    if (*msg++ != ']') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // The following characters should be the command type, followed by a '['
    char cmd_type_str[CMD_MAX_LEN + 1] = { 0 };
    for (size_t i = 0; i < CMD_MAX_LEN; i++, msg++) {
        if (*msg == '[') {
            break;
        }
        cmd_type_str[i] = *msg;
    }
    // We might have reached the max command string length without
    // finding the '[' yet, but in that case it should be the next character
    if (*msg != '[' && *(++msg) != '[') {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }
    msg++;

    // Find the command type in the list of valid commands
    for (size_t i = 0; i < SBP_CMD_TYPE_LEN; i++) {
        if (strncmp(cmd_type_str, sbp_cmd_type_str[i], CMD_MAX_LEN) == 0) {
            cmd_type = (sbp_cmd_type_t)i;
            break;
        }
    }
    if (cmd_type == SBP_CMD_TYPE_LEN) {
        return SBP_ERROR_CMD_TYPE;
    }

    // Find the value length, which is the number of characters between the '[' and ']'
    value_start = msg - msg_start;
    for (value_len = 0; value_len < (msg_len - value_start); value_len++, msg++) {
        if (*msg == ']') {
            break;
        }
    }
    if (*msg != ']' || msg_len != (msg - msg_start + 1)) {
        return SBP_ERROR_PROTOCOL_FORMAT;
    }

    // Only populate cmd structure if the command is valid, which it is at this point
    cmd->type = cmd_type;
    cmd->line = msg_start;
    cmd->line_len = msg_len;
    cmd->id_start = id_start;
    cmd->id_len = id_len;
    cmd->value_start = value_start;
    cmd->value_len = value_len;

    return SBP_SUCCESS;
}

/**
 * @brief Generate a protocol response message for a command message.
 * 
 * @param cmd The command message to respond to.
 * @param value The string with the value to include in the response.
 * @param value_len The length of the value string
 *                  (not including null terminator).
 * @param str_buffer The buffer to write the response string into.
 * @param str_buffer_len The length of the response string buffer.
 * @return Length of the response string, or error code.
 */
static int sbp_generateResponseStr(
    const sbp_cmd_t *cmd, const char *value, const size_t value_len,
    char *str_buffer, const size_t str_buffer_len
) {
    size_t buffer_i = 0;
    size_t cmd_type_length = strlen(sbp_cmd_type_str[cmd->type]);

    // 14 characters from: `R[12345678][]\n`
    if (str_buffer_len < (15 + cmd_type_length + value_len + strlen(SBP_MSG_SEPARATOR))) {
        return SBP_ERROR_LEN;
    }

    // Start with the message type
    str_buffer[buffer_i++] = sbp_msg_type_char[SBP_MSG_RESPONSE];

    // Copy the command ID
    str_buffer[buffer_i++] = '[';
    char *cmd_id_start = (char *)(cmd->line + cmd->id_start);
    for (size_t i = 0; i < cmd->id_len; i++) {
        str_buffer[buffer_i++] = *(cmd_id_start + i);
    }
    str_buffer[buffer_i++] = ']';

    // Copy the command type
    for (size_t i = 0; i < cmd_type_length; i++) {
        str_buffer[buffer_i++] = sbp_cmd_type_str[cmd->type][i];
    }

    // Command value
    str_buffer[buffer_i++] = '[';
    for (size_t i = 0; i < value_len; i++) {
        str_buffer[buffer_i++] = value[i];
    }
    str_buffer[buffer_i++] = ']';

    // Ensure the string ends with the end of message separator
    for (size_t i = 0; i < strlen(SBP_MSG_SEPARATOR); i++) {
        str_buffer[buffer_i++] = SBP_MSG_SEPARATOR[i];
    }

    // Add the null terminator, but don't include it in the returned length
    str_buffer[buffer_i] = '\0';
    return buffer_i;
}

static int sbp_generateErrorResponseStr(const sbp_cmd_t *cmd, char *str_buffer, const size_t str_buffer_len) {
    // TODO: Implement this in a way that is not hard coded
    char response[]= "R[12345678]ERROR[1234]" SBP_MSG_SEPARATOR;
    strncpy(str_buffer, response, str_buffer_len);

    return SBP_SUCCESS;
}

/**
 * @brief Process a command message to generate the appropriate response message.
 * 
 * @param received_cmd The command message to respond to.
 * @param protocol_state The current state of the protocol.
 *                       This will be updated by the command.
 * @param str_buffer The buffer to write the response string into.
 * @param str_buffer_len The length of the response string buffer.
 * @return Length of the response string, or error code.
 */
static int sbp_processCommandResponse(
    const sbp_cmd_t *received_cmd,
    sbp_state_t *protocol_state,
    char *str_buffer, const size_t str_buffer_len
) {
    int callback_result = SBP_SUCCESS;
    switch (received_cmd->type) {
        case SBP_CMD_HANDSHAKE: {
            if (cmd_cbk.handshake) {
                callback_result = cmd_cbk.handshake(protocol_state);
            }
            if (callback_result < 0) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            return sbp_generateResponseStr(received_cmd, SBP_PROTOCOL_VERSION, 1, str_buffer, str_buffer_len);
        }
        case SBP_CMD_RADIOGROUP: {
            int radio_group;
            int conversion_state = intFromCommandValue(
                received_cmd->line + received_cmd->value_start, received_cmd->value_len, &radio_group);
            if (conversion_state != SBP_SUCCESS || radio_group < 0 || radio_group > 255) {
                return SBP_ERROR_CMD_VALUE;
            }
            protocol_state->radio_group = radio_group;

            if (cmd_cbk.radiogroup) {
                callback_result = cmd_cbk.radiogroup(protocol_state);
            }

            if (callback_result < 0) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            // Convert protocol_state->radio_group into a string
            char response_radio_group[4] = { 0 };
            size_t group_str_len = snprintf(response_radio_group, 4, "%d", protocol_state->radio_group);
            if (group_str_len < 1) {
                return SBP_ERROR_ENCODING;
            }
            return sbp_generateResponseStr(
                    received_cmd, response_radio_group, group_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_SWVERSION: {
            if (cmd_cbk.swversion) {
                callback_result = cmd_cbk.swversion(protocol_state);
            }
            if (callback_result < 0) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            // TODO: Implement this in a way that is not hard coded
            return sbp_generateResponseStr(received_cmd, "0.1.0", 5, str_buffer, str_buffer_len);
        }
        case SBP_CMD_HWVERSION: {
            if (cmd_cbk.hwversion) {
                callback_result = cmd_cbk.hwversion(protocol_state);
            }
            if (callback_result < 0) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            // TODO: Implement this in a way that is not hard coded
            return sbp_generateResponseStr(received_cmd, "2", 1, str_buffer, str_buffer_len);
        }
        case SBP_CMD_START: {
            protocol_state->send_periodic = true;
            // TODO: process the content inside the value to determine which sensors to enable
            protocol_state->sensors = (sbp_sensors_t){ };
            protocol_state->sensors.accelerometer = true;
            protocol_state->sensors.buttons = true;
            protocol_state->sensors.button_logo = true;
            if (cmd_cbk.start) {
                callback_result = cmd_cbk.start(protocol_state);
            }
            if (callback_result < 0) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            return sbp_generateResponseStr(received_cmd, NULL, 0, str_buffer, str_buffer_len);
        }
        case SBP_CMD_STOP: {
            protocol_state->send_periodic = false;
            if (cmd_cbk.stop) {
                callback_result = cmd_cbk.stop(protocol_state);
            }
            if (callback_result < 0) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            return sbp_generateResponseStr(received_cmd, NULL, 0, str_buffer, str_buffer_len);
        }
        default:
            callback_result =  SBP_ERROR_CMD_TYPE;
            break;
    }
    return callback_result;
}

// ----------------------------------------------------------------------------
// PUBLIC FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------
void sbp_init(sbp_cmd_callbacks_t *cmd_callbacks, sbp_state_t *protocol_state) {
    // Find the longest command type string, used for parsing commands
    for (int i = 0; i < SBP_CMD_TYPE_LEN; i++) {
        CMD_MAX_LEN = MAX(CMD_MAX_LEN, strlen(sbp_cmd_type_str[i]));
    }

    // Configure the callbacks
    cmd_cbk = *cmd_callbacks;

    // Set protocol state defaults
    protocol_state->radio_group = 42;
    protocol_state->send_periodic = false;
    protocol_state->sensors = (sbp_sensors_t){
        .accelerometer = true,
        .magnetometer = true,
        .buttons = true,
        .button_logo = true,
        .button_pins = true,
        .temperature = true,
        .light_level = true,
        .sound_level = true,
    };
}

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

int sbp_processCommand(const ManagedString& msg, sbp_state_t *protocol_state, char *str_buffer, const size_t str_buffer_len) {
    sbp_cmd_t received_cmd = { };
    const char *msg_str = msg.toCharArray();
    const size_t msg_len = msg.length();

    int result = sbp_parseCommand(msg_str, msg_len, &received_cmd);
    if (result != SBP_SUCCESS) {
        return result;
    }
    return sbp_processCommandResponse(&received_cmd, protocol_state, str_buffer, str_buffer_len);
}

int sbp_processHandshake(const ManagedString& msg, char *str_buffer, const size_t str_buffer_len) {
    sbp_cmd_t received_cmd = { };
    const char *msg_str = msg.toCharArray();
    const size_t msg_len = msg.length();

    int result = sbp_parseCommand(msg_str, msg_len, &received_cmd);
    if (result != SBP_SUCCESS) {
        return result;
    }
    if (received_cmd.type != SBP_CMD_HANDSHAKE) {
        return SBP_ERROR_CMD_TYPE;
    }
    return sbp_generateResponseStr(&received_cmd, SBP_PROTOCOL_VERSION, 1, str_buffer, str_buffer_len);
}