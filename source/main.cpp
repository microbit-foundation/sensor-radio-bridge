#include <stdint.h>
#include <stdio.h>
#include "MicroBit.h"
#include "serial_bridge_protocol.h"

MicroBit uBit;

// Configure how many milliseconds between serial messages
static const int PERIODIC_INTERVAL_MS = 50;
static const int CMD_RESPONSE_TIME = 10;

static const int SERIAL_BUFFER_LEN = 128;

/**
 * @brief Updates the sensor data structure with the current values as enabled
 * in sensor_config.
 *
 * @param sensor_config The sensor configuration to use.
 * @param sensor_data The sensor data structure to update.
 */
void updateSensorData(const sbp_sensors_t sensor_config, sbp_sensor_data_t *sensor_data) {
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
}

int main() {
    uBit.init();

    uBit.serial.setTxBufferSize(SERIAL_BUFFER_LEN);
    uBit.serial.setRxBufferSize(SERIAL_BUFFER_LEN);
    uBit.serial.setBaudrate(115200);

    const size_t serial_data_len = SERIAL_BUFFER_LEN + 1;
    char serial_data[serial_data_len];

    sbp_state_t protocol_state = { };
    sbp_sensor_data_t sensor_data = { };
    sbp_cmd_callbacks_t protocol_callbacks = { };

    sbp_init(&protocol_callbacks, &protocol_state);

    // The first message should always be the handshake
    ManagedString handshake_cmd = uBit.serial.readUntil(SBP_MSG_SEPARATOR, SYNC_SLEEP);
    int hs_response_len = sbp_processHandshake(handshake_cmd, serial_data, serial_data_len);
    if (hs_response_len < SBP_SUCCESS) uBit.panic(200);
    uBit.serial.send((uint8_t *)serial_data, hs_response_len, SYNC_SLEEP);

    // Now process any other message while sending periodic messages (if enabled)
    uint32_t next_periodic_msg = uBit.systemTime() + PERIODIC_INTERVAL_MS;
    while (true) {
        while ((uBit.systemTime() + CMD_RESPONSE_TIME) < next_periodic_msg) {
            ManagedString cmd = uBit.serial.readUntil(SBP_MSG_SEPARATOR, ASYNC);
            if (cmd.length() > 0) {
                // Read any incoming message & process it
                int response_len = sbp_processCommand(cmd, &protocol_state, serial_data, serial_data_len);
                if (response_len < SBP_SUCCESS) uBit.panic(210);

                // For development, uncomment to check available free time
                // uBit.serial.printf("t[%d]", next_periodic_msg - uBit.systemTime());

                uBit.serial.send((uint8_t *)serial_data, response_len, SYNC_SLEEP);
            }
            // Sleep if there is still plenty of time before the next periodic message
            if (!uBit.serial.isReadable() &&
                    ((uBit.systemTime() + (CMD_RESPONSE_TIME * 2)) < next_periodic_msg)) {
                uBit.sleep(1);
            }
        }

        // If periodic messages are enabled, send them
        if (protocol_state.send_periodic) {
            updateSensorData(protocol_state.sensors, &sensor_data);
            int serial_str_length = sbp_sensorDataPeriodicStr(
                protocol_state.sensors, &sensor_data, serial_data, serial_data_len);
            if (serial_str_length < SBP_SUCCESS) {
                uBit.panic(220);
            }

            // For development, uncomment to check available free time
            // uBit.serial.printf("t[%d]", next_periodic_msg - uBit.systemTime());

            // Now wait until ready to send the next serial message
            while (uBit.systemTime() < next_periodic_msg);
            next_periodic_msg = uBit.systemTime() + PERIODIC_INTERVAL_MS;
            uBit.serial.send((uint8_t *)serial_data, serial_str_length, SYNC_SLEEP);
            // uBit.serial.printf("t[%d]\n", next_periodic_msg - uBit.systemTime());
        } else {
            while (uBit.systemTime() < next_periodic_msg);
            next_periodic_msg = uBit.systemTime() + PERIODIC_INTERVAL_MS;
        }
    }
}
