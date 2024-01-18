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
    cmd->id = msg_start + id_start;
    cmd->id_len = id_len;
    cmd->value = msg_start + value_start;
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

    // 14 characters from: `R[12345678][]` + null terminator
    if (str_buffer_len < (14 + cmd_type_length + value_len + SBP_MSG_SEPARATOR_LEN)) {
        return SBP_ERROR_LEN;
    }

    // Start with the message type
    str_buffer[buffer_i++] = sbp_msg_type_char[SBP_MSG_RESPONSE];

    // Copy the command ID
    str_buffer[buffer_i++] = '[';
    for (size_t i = 0; i < cmd->id_len; i++) {
        str_buffer[buffer_i++] = *(cmd->id + i);
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
    for (size_t i = 0; i < SBP_MSG_SEPARATOR_LEN; i++) {
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
    switch (received_cmd->type) {
        case SBP_CMD_HANDSHAKE: {
            if (cmd_cbk.handshake && cmd_cbk.handshake(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            return sbp_generateResponseStr(received_cmd, SBP_PROTOCOL_VERSION, 1, str_buffer, str_buffer_len);
        }
        case SBP_CMD_RADIOFREQ: {
            int radio_frequency;
            int result = intFromCommandValue(received_cmd->value, received_cmd->value_len, &radio_frequency);
            if (result != SBP_SUCCESS || radio_frequency < SBP_CMD_RADIO_FREQ_MIN || radio_frequency > SBP_CMD_RADIO_FREQ_MAX) {
                return SBP_ERROR_CMD_VALUE;
            }
            protocol_state->radio_frequency = (uint8_t)radio_frequency;

            if (cmd_cbk.radiofrequency && cmd_cbk.radiofrequency(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }

            // Convert protocol_state->radio_frequency into a string
            char response_radio_frequency[4] = { 0 };
            size_t frequency_str_len = snprintf(response_radio_frequency, 4, "%d", protocol_state->radio_frequency);
            if (frequency_str_len < 1) return SBP_ERROR_ENCODING;

            return sbp_generateResponseStr(
                    received_cmd, response_radio_frequency, frequency_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_PERIOD: {
            int period_ms;
            int result = intFromCommandValue(received_cmd->value, received_cmd->value_len, &period_ms);
            if (result != SBP_SUCCESS || period_ms < SBP_CMD_PERIOD_MIN || period_ms > SBP_CMD_PERIOD_MAX) {
                return SBP_ERROR_CMD_VALUE;
            }
            protocol_state->period_ms = (uint16_t)period_ms;

            if (cmd_cbk.period && cmd_cbk.period(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }

            // Convert protocol_state->period_ms into a string
            char response_period[6] = { 0 };
            size_t period_str_len = snprintf(response_period, 6, "%d", protocol_state->period_ms);
            if (period_str_len < 1) return SBP_ERROR_ENCODING;

            return sbp_generateResponseStr(
                    received_cmd, response_period, period_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_SWVERSION: {
            if (cmd_cbk.swversion && cmd_cbk.swversion(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            // TODO: Implement this in a way that is not hard coded
            return sbp_generateResponseStr(received_cmd, "0.1.0", 5, str_buffer, str_buffer_len);
        }
        case SBP_CMD_HWVERSION: {
            if (cmd_cbk.hwversion && cmd_cbk.hwversion(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            // TODO: Implement this in a way that is not hard coded
            return sbp_generateResponseStr(received_cmd, "2", 1, str_buffer, str_buffer_len);
        }
        case SBP_CMD_START: {
            protocol_state->sensors.raw = 0;
            // The value format for the start command is a single letter for each sensor type
            // e.g. "AMBL" for accelerometer, magnetometer, buttons and light level
            for (size_t i = 0; i < received_cmd->value_len; i++) {
                char value_char = *(received_cmd->value + i);
                bool sensor_found = false;
                for (size_t j = 0; j < SBP_SENSOR_TYPE_LEN; j++) {
                    if (value_char == sbp_sensor_type[j]) {
                        protocol_state->sensors.raw |= (uint8_t)(1 << j);
                        sensor_found = true;
                        break;
                    }
                }
                if (!sensor_found) return SBP_ERROR_CMD_VALUE;
            }
            protocol_state->send_periodic = true;
            if (cmd_cbk.start && cmd_cbk.start(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            return sbp_generateResponseStr(received_cmd, NULL, 0, str_buffer, str_buffer_len);
        }
        case SBP_CMD_STOP: {
            protocol_state->send_periodic = false;
            if (cmd_cbk.stop && cmd_cbk.stop(protocol_state) != SBP_SUCCESS) {
                return sbp_generateErrorResponseStr(received_cmd, str_buffer, str_buffer_len);
            }
            return sbp_generateResponseStr(received_cmd, NULL, 0, str_buffer, str_buffer_len);
        }
        default:
            return SBP_ERROR_CMD_TYPE;
    }
    return SBP_ERROR;
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
    protocol_state->radio_frequency = SBP_DEFAULT_RADIO_FREQ;
    protocol_state->send_periodic = SBP_DEFAULT_SEND_PERIODIC;
    protocol_state->period_ms = SBP_DEFAULT_PERIOD_MS;
    protocol_state->sensors.raw = SBP_DEFAULT_SENSORS;
}

int sbp_sensorDataPeriodicStr(
    const sbp_sensors_t enabled_data, const sbp_sensor_data_t *data,
    char *str_buffer, const int str_buffer_len
) {
    int serial_data_length = 0;
    static uint32_t packet_id = 0;

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
            SBP_SENSOR_STR_ACC_X "[%d]" SBP_SENSOR_STR_ACC_Y "[%d]" SBP_SENSOR_STR_ACC_Z "[%d]",
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
            SBP_SENSOR_STR_MAG_X "[%d]" SBP_SENSOR_STR_MAG_Y "[%d]" SBP_SENSOR_STR_MAG_Z "[%d]",
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
            SBP_SENSOR_STR_BTN_A "[%d]" SBP_SENSOR_STR_BTN_B "[%d]",
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
            SBP_SENSOR_STR_BTN_LOGO "[%d]",
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
            SBP_SENSOR_STR_BTN_P0 "[%d]" SBP_SENSOR_STR_BTN_P1 "[%d]" SBP_SENSOR_STR_BTN_P2 "[%d]",
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
            SBP_SENSOR_STR_TEMP "[%d]",
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
            SBP_SENSOR_STR_LIGHT "[%d]",
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
            SBP_SENSOR_STR_SOUND "[%d]",
            data->sound_level
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }

    // Ensure the string ends with the message separator and a null terminator
    if ((str_buffer_len - serial_data_length) >= (SBP_MSG_SEPARATOR_LEN + 1)) {
        serial_data_length += snprintf(
            str_buffer + serial_data_length, SBP_MSG_SEPARATOR_LEN + 1, SBP_MSG_SEPARATOR
        );
    } else {
        const size_t first_char_index = str_buffer_len - (SBP_MSG_SEPARATOR_LEN + 1);
        for (size_t i = 0; i < SBP_MSG_SEPARATOR_LEN; i++) {
            str_buffer[first_char_index + i] = SBP_MSG_SEPARATOR[i];
        }
        str_buffer[str_buffer_len - 1] = '\0';
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
