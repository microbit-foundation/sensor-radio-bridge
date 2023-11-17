#include <stdint.h>
#include <stdio.h>
#include "MicroBit.h"
#include "serial_bridge_protocol.h"

MicroBit uBit;


// Configure how many milliseconds between serial messages
static const int PACKET_INTERVAL_MS = 20;

static const int SERIAL_BUFFER_LEN = 128;


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
        .compass = true,
        .buttons = true,
        .button_pins = true,
        .temperature = true,
        .light_level = true,
        .sound_level = true,
    };

    uint32_t next_packet = uBit.systemTime() + PACKET_INTERVAL_MS;
    while (true) {
        sbp_sensor_data_t sensor_data = {
            .accelerometer_x = uBit.accelerometer.getX(),
            .accelerometer_y = uBit.accelerometer.getY(),
            .accelerometer_z = uBit.accelerometer.getZ(),
            .compass_x = uBit.compass.getX(),
            .compass_y = uBit.compass.getY(),
            .compass_z = uBit.compass.getZ(),
            .button_a = (bool)uBit.buttonA.isPressed(),
            .button_b = (bool)uBit.buttonB.isPressed(),
            .button_logo = (bool)uBit.logo.isPressed(),
            .button_p0 = (bool)uBit.io.P0.isTouched(),
            .button_p1 = (bool)uBit.io.P1.isTouched(),
            .button_p2 = (bool)uBit.io.P2.isTouched(),
            .temperature = uBit.thermometer.getTemperature(),
            .light_level = uBit.display.readLightLevel(),
            .sound_level = (int)uBit.audio.levelSPL->getValue(),
        };
        int serial_str_length = sbp_sensorDataSerialStr(
            &sensor_config, &sensor_data, serial_data, serial_data_len);
        if (serial_str_length < SBP_SUCCESS) {
            uBit.panic(120 + (serial_str_length * -1));
        }

        // For development/debugging purposes, uncomment to print how much
        // free time there is available for other operations
        // uBit.serial.printf("t[%d],", next_packet - uBit.systemTime());

        // Now wait until ready to send the next serial message
        while (uBit.systemTime() < next_packet);
        next_packet = uBit.systemTime() + PACKET_INTERVAL_MS;
        uBit.serial.send((uint8_t *)serial_data, serial_str_length, SYNC_SLEEP);
    }
}
