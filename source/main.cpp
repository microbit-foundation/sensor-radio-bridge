#include <stdint.h>
#include <stdio.h>
#include "MicroBit.h"
#include "serial_bridge_protocol.h"

MicroBit uBit;


// Configure how many milliseconds between serial messages
static const int PACKET_INTERVAL_MS = 20;

static const int SERIAL_BUFFER_LEN = 128;


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

    const int serial_data_len = SERIAL_BUFFER_LEN + 1;
    char serial_data[serial_data_len];

    // Default data to send
    sbp_sensors_t sensor_config = {
        .accelerometer = true,
        .magnetometer = true,
        .buttons = true,
        .button_logo = true,
        .button_pins = true,
        .temperature = true,
        .light_level = true,
        .sound_level = true,
    };

    ManagedString handshake_cmd = uBit.serial.readUntil(SBP_MSG_SEPARATOR, SYNC_SLEEP);
    int response_len = sbp_processHandshake(
            handshake_cmd.toCharArray(), handshake_cmd.length(),
            serial_data, serial_data_len);
    if (response_len < SBP_SUCCESS) {
        uBit.panic(200);
    }
    uBit.serial.send((uint8_t *)serial_data, response_len, SYNC_SLEEP);


    sbp_sensor_data_t sensor_data = {};
    uint32_t next_packet = uBit.systemTime() + PACKET_INTERVAL_MS;
    while (true) {
        updateSensorData(sensor_config, &sensor_data);
        int serial_str_length = sbp_sensorDataPeriodicStr(
            sensor_config, &sensor_data, serial_data, serial_data_len);
        if (serial_str_length < SBP_SUCCESS) {
            uBit.panic(220 + (serial_str_length * -1));
        }

        // For development/debugging purposes, uncomment to print how much
        // free time there is available for other operations
        // uBit.serial.printf("t[%d]", next_packet - uBit.systemTime());

        // Now wait until ready to send the next serial message
        while (uBit.systemTime() < next_packet);
        next_packet = uBit.systemTime() + PACKET_INTERVAL_MS;
        uBit.serial.send((uint8_t *)serial_data, serial_str_length, SYNC_SLEEP);
    }
}
