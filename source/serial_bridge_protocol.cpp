#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "serial_bridge_protocol.h"

#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define CLAMP(x, min, max)      MIN(MAX(x, min), max)

static size_t CMD_MAX_LEN = 0;
static sbp_cmd_callbacks_t cmd_cbk = { };

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------

static_assert(sizeof(unsigned long) == sizeof(uint32_t),
              "uintFromCommandValue() depends on unsigned long being 32 bits");

static uint32_t uintFromCommandValue(const char *value_str, const size_t value_str_len, uint32_t *value) {
    // Convert into a null terminated string
    char value_str_terminated[value_str_len + 1];
    for (size_t i = 0; i < value_str_len; i++) {
        value_str_terminated[i] = value_str[i];
    }
    value_str_terminated[value_str_len] = '\0';

    // Convert value_str into an integer
    char *endptr;
    unsigned long result = strtoul(value_str_terminated, &endptr, 10);
    if (endptr != &value_str_terminated[value_str_len]) {
        return SBP_ERROR_CMD_VALUE;
    }
    if (*value == 0 && !(value_str_len == 1 && value_str[0] == '0')) {
        return SBP_ERROR_CMD_VALUE;
    }
    *value = (uint32_t)result;
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
    const size_t id_len_max = 8;
    size_t id_len = 0;
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

    // The characters until ']' (max 8 chars) should be the command ID with a valid hex number
    id_start = msg - msg_start;
    for (id_len = 0; id_len < id_len_max; id_len++, msg++) {
        if (*msg == ']') {
            break;
        }
        if (!(*msg >= '0' && *msg <= '9') &&
                !(*msg >= 'A' && *msg <= 'F') &&
                    !(*msg >= 'a' && *msg <= 'f')) {
            return SBP_ERROR_PROTOCOL_FORMAT;
        }
    }
    if (id_len == 0 || *msg++ != ']') {
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

static int sbp_generateErrorResponseStr(const sbp_cmd_t *cmd, const uint8_t error_code, char *str_buffer, const size_t str_buffer_len) {
    // 22 characters from: `R[12345678]ERROR[255]` + null terminator
    if (str_buffer_len < (22 + SBP_MSG_SEPARATOR_LEN)) {
        return SBP_ERROR_LEN;
    }

    int cx = snprintf(
        str_buffer,
        str_buffer_len,
        "R[%.*s]ERROR[%d]" SBP_MSG_SEPARATOR,
        cmd->id_len, cmd->id,
        error_code
    );
    if (cx < 1) return SBP_ERROR_ENCODING;

    return cx;
}

/**
 * @brief Process a command to generate the appropriate response message.
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
            // TODO: Return an error if the value is not empty
            return sbp_generateResponseStr(received_cmd, SBP_PROTOCOL_VERSION, 1, str_buffer, str_buffer_len);
        }
        case SBP_CMD_RADIOFREQ: {
            // This command has two modes:
            // 1. An empty value - it returns the current frequency
            // 2. A value - it sets the frequency and returns the final frequency configured
            //    If the frequency was already saved to flash it cannot be changed, so this value might be different
            if (received_cmd->value_len != 0) {
                uint32_t radio_frequency;
                int result = uintFromCommandValue(received_cmd->value, received_cmd->value_len, &radio_frequency);
                if (result != SBP_SUCCESS || radio_frequency > SBP_CMD_RADIO_FREQ_MAX) {
                    return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INVALID_VALUE, str_buffer, str_buffer_len);
                }
                protocol_state->radio_frequency = (uint8_t)radio_frequency;

                if (cmd_cbk.radioFrequency && cmd_cbk.radioFrequency(protocol_state) != SBP_SUCCESS) {
                    return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INVALID_VALUE, str_buffer, str_buffer_len);
                }
            }

            // Convert protocol_state->radio_frequency into a string
            char response_radio_frequency[4] = { 0 };
            size_t frequency_str_len = snprintf(response_radio_frequency, 4, "%d", protocol_state->radio_frequency);
            if (frequency_str_len < 1) return SBP_ERROR_ENCODING;

            return sbp_generateResponseStr(
                    received_cmd, response_radio_frequency, frequency_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_REMOTEID: {
            // Empty value indicates a read command only
            if (received_cmd->value_len != 0) {
                uint32_t remote_microbit_id;
                int result = uintFromCommandValue(received_cmd->value, received_cmd->value_len, &remote_microbit_id);
                if (result != SBP_SUCCESS) {
                    return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INVALID_VALUE, str_buffer, str_buffer_len);
                }
                protocol_state->remote_id = remote_microbit_id;

                if (cmd_cbk.remoteMbId) {
                    int result = cmd_cbk.remoteMbId(protocol_state);
                    if (result < SBP_SUCCESS) {
                        uint8_t error_code;
                        switch (result) {
                            case SBP_ERROR_CMD_REPEATED: error_code = SBP_ERROR_CODE_VALUE_ALREADY_SET; break;
                            case SBP_ERROR_INTERNAL:     error_code = SBP_ERROR_CODE_INTERNAL_ERROR; break;
                            default:                     error_code = SBP_ERROR_CODE_INVALID_VALUE; break;
                        }
                        return sbp_generateErrorResponseStr(received_cmd, error_code, str_buffer, str_buffer_len);
                    }
                }
            }

            // Convert protocol_state->remote_id into a string, max value 2*32
            char response_mb_id[12] = { 0 };
            size_t mb_id_str_len = snprintf(response_mb_id, 12, "%u", (unsigned int)protocol_state->remote_id);
            if (mb_id_str_len < 1) return SBP_ERROR_ENCODING;

            return sbp_generateResponseStr(
                    received_cmd, response_mb_id, mb_id_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_ID: {
            // This is a read-only command and only accepts empty values
            if (received_cmd->value_len != 0) {
                return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INVALID_VALUE, str_buffer, str_buffer_len);
            }

            // Convert protocol_state->id into a string, range -2,147,483,648 to 2,147,483,647
            char response_mb_id[12] = { 0 };
            size_t mb_id_str_len = snprintf(response_mb_id, 12, "%u", (unsigned int)protocol_state->id);
            if (mb_id_str_len < 1) return SBP_ERROR_ENCODING;

            return sbp_generateResponseStr(
                    received_cmd, response_mb_id, mb_id_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_PERIOD: {
            // TODO: Make this also a "get" command when value is empty?
            uint32_t period_ms;
            int result = uintFromCommandValue(received_cmd->value, received_cmd->value_len, &period_ms);
            if (result != SBP_SUCCESS || period_ms < SBP_CMD_PERIOD_MIN || period_ms > SBP_CMD_PERIOD_MAX) {
                return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INVALID_VALUE, str_buffer, str_buffer_len);
            }
            protocol_state->period_ms = (uint16_t)period_ms;

            // Convert protocol_state->period_ms (uint16_t) into a string
            char response_period[6] = { 0 };
            size_t period_str_len = snprintf(response_period, 6, "%u", (unsigned int)protocol_state->period_ms);
            if (period_str_len < 1) return SBP_ERROR_ENCODING;

            return sbp_generateResponseStr(
                    received_cmd, response_period, period_str_len, str_buffer, str_buffer_len);
        }
        case SBP_CMD_SWVERSION: {
            // TODO: Return an error if the value is not empty
            // TODO: Need to sort out versions that are not single digit, e.g. 1.2.3
            return sbp_generateResponseStr(received_cmd, protocol_state->sw_version, 5, str_buffer, str_buffer_len);
        }
        case SBP_CMD_HWVERSION: {
            // TODO: Return an error if the value is not empty
            // Convert protocol_state->hw_version (uint8) to a string
            char response_hw_version[4] = { 0 };
            size_t hw_version_str_len = snprintf(response_hw_version, 6, "%d", protocol_state->hw_version);
            if (hw_version_str_len < 1) return SBP_ERROR_ENCODING;
            return sbp_generateResponseStr(received_cmd, response_hw_version, 1, str_buffer, str_buffer_len);
        }
        case SBP_CMD_START: {
            // The value format for the start command is a single letter for each sensor type
            // e.g. "AMBL" for accelerometer, magnetometer, buttons and light level
            sbp_sensors_t sensors;
            sensors.raw = 0;
            for (size_t i = 0; i < received_cmd->value_len; i++) {
                char value_char = *(received_cmd->value + i);
                bool valid_value = false;
                for (size_t j = 0; j < SBP_SENSOR_TYPE_LEN; j++) {
                    if (value_char == sbp_sensor_type[j]) {
                        sensors.raw |= (uint8_t)(1 << j);
                        valid_value = true;
                        break;
                    }
                }
                if (!valid_value) {
                    return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INVALID_VALUE, str_buffer, str_buffer_len);
                }
            }
            // Save the state in case we need to restore it due to an error on the callback
            bool original_send_periodic = protocol_state->send_periodic;
            bool original_periodic_compact = protocol_state->periodic_compact;
            sbp_sensors_t original_sensors = protocol_state->sensors;
            protocol_state->send_periodic = true;
            protocol_state->periodic_compact = false;
            protocol_state->sensors = sensors;

            if (cmd_cbk.start && cmd_cbk.start(protocol_state) != SBP_SUCCESS) {
                protocol_state->send_periodic = original_send_periodic;
                protocol_state->periodic_compact = original_periodic_compact;
                protocol_state->sensors = original_sensors;
                return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INTERNAL_ERROR, str_buffer, str_buffer_len);
            }

            return sbp_generateResponseStr(received_cmd, NULL, 0, str_buffer, str_buffer_len);
        }
        case SBP_CMD_ZSTART: {
            // Save the state in case we need to restore it due to an error on the callback
            bool original_send_periodic = protocol_state->send_periodic;
            bool original_periodic_compact = protocol_state->periodic_compact;
            sbp_sensors_t original_sensors = protocol_state->sensors;

            protocol_state->send_periodic = true;
            protocol_state->periodic_compact = true;
            protocol_state->sensors.raw = 0;
            // TODO: Currently hardcoding this command to only send accelerometer and buttons
            //       as that's all that is implemented right now
            protocol_state->sensors.accelerometer = true;
            protocol_state->sensors.buttons = true;

            if (cmd_cbk.zstart && cmd_cbk.zstart(protocol_state) != SBP_SUCCESS) {
                protocol_state->send_periodic = original_send_periodic;
                protocol_state->periodic_compact = original_periodic_compact;
                protocol_state->sensors = original_sensors;
                return sbp_generateErrorResponseStr(received_cmd, SBP_ERROR_CODE_INTERNAL_ERROR, str_buffer, str_buffer_len);
            }

            return sbp_generateResponseStr(received_cmd, NULL, 0, str_buffer, str_buffer_len);
        }
        case SBP_CMD_STOP: {
            // TODO: Return an error if the value is not empty
            protocol_state->send_periodic = false;
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
int sbp_init(sbp_cmd_callbacks_t *cmd_callbacks, sbp_state_t *protocol_state) {
    // Find the longest command type string, used for parsing commands
    for (int i = 0; i < SBP_CMD_TYPE_LEN; i++) {
        CMD_MAX_LEN = MAX(CMD_MAX_LEN, strlen(sbp_cmd_type_str[i]));
    }

    // Configure the callbacks
    cmd_cbk = *cmd_callbacks;

    // Some of the data should be already configured and within range
    if (protocol_state->hw_version == 0 ||
        protocol_state->sw_version == NULL ||
        protocol_state->radio_frequency > SBP_CMD_RADIO_FREQ_MAX ||
        protocol_state->period_ms > SBP_CMD_PERIOD_MAX) {
        return SBP_ERROR;
    }

    return SBP_SUCCESS;
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

int sbp_compactSensorDataPeriodicStr(
    const sbp_sensors_t enabled_data, const sbp_sensor_data_t *data,
    char *str_buffer, const int str_buffer_len
) {
    int serial_data_length = 0;
    // The message ID is only 1 byte long
    static uint8_t packet_id = 0;

    int cx = snprintf(
        str_buffer + serial_data_length,
        str_buffer_len - serial_data_length,
        "P%02X", packet_id++
    );
    if (cx > 0) {
        serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
    } else {
        return SBP_ERROR_ENCODING;
    }

    if (enabled_data.accelerometer) {
        // Accelerometer max is +/- 2 g, so we can use 12 bits (3 Hex digits) per axis
        uint16_t x = CLAMP(data->accelerometer_x, -2048, 2047) + 2048;
        uint16_t y = CLAMP(data->accelerometer_y, -2048, 2047) + 2048;
        uint16_t z = CLAMP(data->accelerometer_z, -2048, 2047) + 2048;

        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "%03X%03X%03X", x, y, z
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }
    if (enabled_data.buttons) {
        // Buttons are 1 bit each, with button A as the LSB
        uint8_t buttons = (data->button_a & 0x01) | ((data->button_b & 0x01) << 1);

        int cx = snprintf(
            str_buffer + serial_data_length,
            str_buffer_len - serial_data_length,
            "%1X", buttons
        );
        if (cx > 0) {
            serial_data_length += MIN(cx, str_buffer_len - serial_data_length - 1);
        } else {
            return SBP_ERROR_ENCODING;
        }
    }

    // TODO: Only accelerometer and buttons implemented, other sensors are not
    //       implemented yet
    if (enabled_data.magnetometer || enabled_data.button_logo || enabled_data.button_pins ||
        enabled_data.temperature || enabled_data.light_level || enabled_data.sound_level) {
        return SBP_ERROR_NOT_IMPLEMENTED;
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
