#include <stdint.h>
#include <stdio.h>
#include "MicroBitFlash.h"
#include "MicroBit.h"
#include "serial_bridge_protocol.h"
#include "radio_comms.h"
#include "mb_images.h"
#include "main.h"

MicroBit uBit;

// Configure how many milliseconds to leave as a buffer for accurate periodic messages
static const int PERIODIC_BUFFER_MS = 9;

// Last 1 KB of flash where we can store the radio frequency and/or remote micro:bit ID
const uint32_t REMOTE_MB_ID_ADDR = 0x0007FC00;

// The sensor data instance to hold the latest sensor values
static sbp_sensor_data_t sensor_data = { };

// Function declarations
int setRadioFrequency(sbp_state_s *protocol_state);
uint32_t getRemoteMbId();

#if CONFIG_ENABLED(RADIO_BRIDGE)
/**
 * @return The micro:bit ID of the currently active remote micro:bit.
 */
static inline uint32_t getActiveRemoteMbId() {
#if CONFIG_ENABLED(DEV_MODE)
    return radiobridge_getActiveRemoteMbId();
#else
    return getRemoteMbId();
#endif
}

/**
 * @brief Callback for received radio packets.
 *
 * @param radio_sensor_data The data received via radio.
 */
void radioDataCallback(radio_packet_t *radio_packet) {
    if (radio_packet->packet_type != RADIO_PKT_SENSOR_DATA) return;
    if (radio_packet->mb_id == getActiveRemoteMbId()) {
        radio_sensor_data_t *radio_sensor_data = &radio_packet->sensor_data;
        sensor_data.accelerometer_x = radio_sensor_data->accelerometer_x;
        sensor_data.accelerometer_y = radio_sensor_data->accelerometer_y;
        sensor_data.accelerometer_z = radio_sensor_data->accelerometer_z;
        sensor_data.button_a = radio_sensor_data->button_a;
        sensor_data.button_b = radio_sensor_data->button_b;
        sensor_data.button_logo = radio_sensor_data->button_logo;
        sensor_data.fresh_data = true;
    }
#if CONFIG_ENABLED(DEV_MODE)
    radiobridge_updateRemoteMbIds(radio_packet->mb_id);
#endif
}
#endif

/**
 * @brief Stores the remote micro:bit ID into flash (NVM),
 * for permanence after reset or power off.
 *
 * @param protocol_state The protocol state with the updated remote micro:bit ID value.
 *
 * @return SBP_SUCCESS if the remote micro:bit was stored successfully, an error value otherwise.
 */
int storeRemoteMbId(sbp_state_s *protocol_state) {
    uint32_t *stored_remote_mb_id = (uint32_t *)REMOTE_MB_ID_ADDR;
    if (*stored_remote_mb_id == 0xFFFFFFFF) {
        uint32_t remote_mb_id = protocol_state->remote_id;
        MicroBitFlash flash;
        int success = flash.flash_write((void *)REMOTE_MB_ID_ADDR, (void *)&remote_mb_id, 4, NULL);
        if (success != MICROBIT_OK) return SBP_ERROR_INTERNAL;
        if (*stored_remote_mb_id != remote_mb_id) return SBP_ERROR_INTERNAL;
    } else if ((uint32_t)protocol_state->remote_id != *stored_remote_mb_id) {
        // We received a different ID than what we have stored, so reject it
        protocol_state->remote_id = *stored_remote_mb_id;
        return SBP_ERROR_CMD_REPEATED;
    }
    return SBP_SUCCESS;
}

/**
 * @brief Saves the remote micro:bit ID into Non Volatile Memory, and
 * configures the radio frequency based on this value.
 *
 * @param protocol_state The protocol state with the updated remote ID value.
 *
 * @return SBP_SUCCESS if the remote micro:bit ID was set successfully, an error value otherwise.
 */
int setRemoteMbId(sbp_state_s *protocol_state) {
    int result = storeRemoteMbId(protocol_state);
    if (result < SBP_SUCCESS) return result;

    protocol_state->radio_frequency = radio_getFrequencyFromId(protocol_state->remote_id);
    return setRadioFrequency(protocol_state);
}

/**
 * @brief Get the Radio Frequency from Non Volatile Memory, or the default
 * value if not set in NVM.
 *
 * The default value, if noting stored in NVM, will be the unique ID from
 * this micro:bit. This is statistically less likely to match the remote ID
 * than picking a number like 0.
 *
 * @return The radio frequency.
 */
uint32_t getRemoteMbId() {
    uint32_t *stored_remote_mb_id = (uint32_t *)REMOTE_MB_ID_ADDR;
    if (*stored_remote_mb_id == 0xFFFFFFFF) {
        return microbit_serial_number();
    }
    return *stored_remote_mb_id;
}

/**
 * @brief Get the radio frequency from the configured ID of the remote
 * micro:bit.
 *
 * @return The radio frequency.
 */
uint8_t getRadioFrequency() {
    return radio_getFrequencyFromId(getRemoteMbId());
}

/**
 * @brief Sets the radio frequency configured in the protocol state.
 *
 * @param protocol_state The protocol state with the updated radio frequency.
 *
 * @return SBP_SUCCESS if the radio frequency was set successfully, an error
 *         value otherwise.
 */
int setRadioFrequency(sbp_state_s *protocol_state) {
    int success = uBit.radio.setFrequencyBand(protocol_state->radio_frequency);
    if (success != MICROBIT_OK) return SBP_ERROR_INTERNAL;
    return SBP_SUCCESS;
}

/**
 * @brief Sets any actions required when the start/zstart command is received.
 *
 * @param protocol_state The protocol state to set the start command for.
 *
 * @return SBP_SUCCESS
 */
int setStartCommand(sbp_state_s *protocol_state) {
    // Discard any data received before this point as stale data
    sensor_data.fresh_data = false;
    return SBP_SUCCESS;
}

/**
 * @brief Updates the sensor data structure with the current values as enabled
 * in sensor_config.
 *
 * @param sensor_config The sensor configuration to use.
 * @param sensor_data The sensor data structure to update.
 */
void updateSensorData(const sbp_sensors_t sensor_config, sbp_sensor_data_t *sensor_data) {
#if CONFIG_DISABLED(RADIO_BRIDGE)
    if (sensor_config.accelerometer) {
        sensor_data->accelerometer_x = uBit.accelerometer.getX();
        sensor_data->accelerometer_y = uBit.accelerometer.getY();
        sensor_data->accelerometer_z = uBit.accelerometer.getZ();
    }
    if (sensor_config.magnetometer) {
        sensor_data->magnetometer_x = uBit.compass.getX();
        sensor_data->magnetometer_y = uBit.compass.getY();
        sensor_data->magnetometer_z = uBit.compass.getZ();
    }
    if (sensor_config.buttons) {
        sensor_data->button_a = (bool)uBit.buttonA.isPressed();
        sensor_data->button_b = (bool)uBit.buttonB.isPressed();
    }
    if (sensor_config.button_logo) {
        sensor_data->button_logo = (bool)uBit.logo.isPressed();
    }
    if (sensor_config.button_pins) {
        sensor_data->button_p0 = (bool)uBit.io.P0.isTouched();
        sensor_data->button_p1 = (bool)uBit.io.P1.isTouched();
        sensor_data->button_p2 = (bool)uBit.io.P2.isTouched();
    }
    if (sensor_config.temperature) {
        sensor_data->temperature = uBit.thermometer.getTemperature();
    }
    if (sensor_config.light_level) {
        sensor_data->light_level = uBit.display.readLightLevel();
    }
    if (sensor_config.sound_level) {
        sensor_data->sound_level = (int)uBit.audio.levelSPL->getValue();
    };
    sensor_data->fresh_data = true;
#endif
}

int main() {
    uBit.init();

    uBit.display.print(IMG_WAITING);

    const int SERIAL_BUFFER_LEN = 128;
    uBit.serial.setTxBufferSize(SERIAL_BUFFER_LEN);
    uBit.serial.setRxBufferSize(SERIAL_BUFFER_LEN);
    uBit.serial.setBaudrate(115200);

    const size_t serial_data_len = SERIAL_BUFFER_LEN + 1;
    char serial_data[serial_data_len];

    sbp_state_t protocol_state = {
        .send_periodic = SBP_DEFAULT_SEND_PERIODIC,
        .periodic_compact = SBP_DEFAULT_PERIODIC_Z,
        .radio_frequency = getRadioFrequency(),
        .remote_id = getRemoteMbId(),
        .id = microbit_serial_number(),
        .period_ms = SBP_DEFAULT_PERIOD_MS,
        // TODO: Get the hardware version from the micro:bit DAL/CODAL
        .hw_version = 2,
        .sw_version = PROJECT_VERSION,
        .sensors = { },
    };
    sbp_cmd_callbacks_t protocol_callbacks = {
        .radioFrequency = setRadioFrequency,
        .remoteMbId = setRemoteMbId,
        .start = setStartCommand,
        .zstart = setStartCommand,
    };

    int init_success = sbp_init(&protocol_callbacks, &protocol_state);
    if (init_success < SBP_SUCCESS) uBit.panic(200);

#if CONFIG_ENABLED(RADIO_REMOTE)
    radiotx_mainLoop();
#elif CONFIG_ENABLED(RADIO_BRIDGE)
    radiobridge_init(radioDataCallback, protocol_state.radio_frequency);
#endif

    uint32_t next_periodic_msg = uBit.systemTime() + protocol_state.period_ms;
    while (true) {
        // Read any incoming message & process it until we reached the time reserved for periodic messages
        while ((uBit.systemTime() + PERIODIC_BUFFER_MS) < next_periodic_msg) {
            ManagedString cmd = uBit.serial.readUntil(SBP_MSG_SEPARATOR, ASYNC);
            if (cmd.length() > 0) {
                // Read any incoming message & process it
                int response_len = sbp_processCommand(cmd, &protocol_state, serial_data, serial_data_len);
                if (response_len < SBP_SUCCESS) uBit.panic(210);
                uBit.serial.send((uint8_t *)serial_data, response_len, SYNC_SLEEP);
            }
            // Sleep if there is no buffered message, and enough time before the periodic message
            if (!uBit.serial.isReadable() && ((uBit.systemTime() + PERIODIC_BUFFER_MS) < next_periodic_msg)) {
                uBit.sleep(1);  // This might take up to 4ms, as that's the CODAL ticker resolution
            }
        }

#if CONFIG_ENABLED(DEV_MODE)
        if (uBit.logo.isPressed()) {
            // Useful to test ML Tool crash recovery
            uBit.panic(0);
        }
    #if CONFIG_ENABLED(RADIO_BRIDGE)
        if (uBit.buttonA.isPressed()) {
            radiobridge_switchNextRemoteMicrobit();
        }
    #endif
#endif

        // If periodic messages are enabled and new data has been received, send it
        if (protocol_state.send_periodic) {
            updateSensorData(protocol_state.sensors, &sensor_data);
            bool fresh_data = sensor_data.fresh_data;
            sensor_data.fresh_data = false;

            int serial_str_length;
            if (protocol_state.periodic_compact) {
                serial_str_length = sbp_compactSensorDataPeriodicStr(
                        protocol_state.sensors, &sensor_data, serial_data, serial_data_len);
            } else {
                serial_str_length = sbp_sensorDataPeriodicStr(
                        protocol_state.sensors, &sensor_data, serial_data, serial_data_len);
            }
            if (serial_str_length < SBP_SUCCESS) uBit.panic(220);

            // For development, uncomment to check available free time
            // uBit.serial.printf("t[%d]", next_periodic_msg - uBit.systemTime());

            // Now wait without sleeping until ready to send the serial message
            while (uBit.systemTime() < next_periodic_msg);
            next_periodic_msg = uBit.systemTime() + protocol_state.period_ms;

            if (fresh_data) {
                uBit.serial.send((uint8_t *)serial_data, serial_str_length, SYNC_SLEEP);
                uBit.display.print(IMG_RUNNING);
            } else {
                // Stale data, blink the waiting image until new data is received
                static bool blink = true;
                static uint32_t blink_count = 0;
                if (blink_count++ % 30 == 0) {
                    uBit.display.print(blink ? IMG_WAITING : IMG_EMPTY);
                    blink = !blink;
                }
            }
        } else {
            // In this case we don't need to keep a constant periodic interval, just continue
            next_periodic_msg = uBit.systemTime() + protocol_state.period_ms;
        }
    }
}
