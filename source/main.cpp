#include <stdint.h>
#include <stdio.h>
#include "MicroBit.h"

MicroBit uBit;

// Configure what data to send via serial
static const bool STREAM_ACC = true;
static const bool STREAM_COMP = false;
static const bool STREAM_BUTTONS = true;

// Configure how many milliseconds between serial messages
static const int PACKET_INTERVAL_MS = 20;

static const int SERIAL_BUFFER_LEN = 128;


int main() {
    uBit.init();

    uBit.serial.setTxBufferSize(SERIAL_BUFFER_LEN);
    uBit.serial.setRxBufferSize(SERIAL_BUFFER_LEN);
    uBit.serial.setBaudrate(115200);

    char serial_data[SERIAL_BUFFER_LEN + 1];
    int serial_data_length = 0;
    uint32_t next_packet = uBit.systemTime() + PACKET_INTERVAL_MS;

    while (true) {
        serial_data_length = 0;

        if (STREAM_ACC) {
            int cx = snprintf(
                serial_data + serial_data_length,
                SERIAL_BUFFER_LEN - serial_data_length,
                "AX[%d],AY[%d],AZ[%d],",
                uBit.accelerometer.getX(),
                uBit.accelerometer.getY(),
                uBit.accelerometer.getZ()
            );
            if (cx > 0 && cx < (SERIAL_BUFFER_LEN - serial_data_length)) {
                serial_data_length += cx;
            }
        }
        if (STREAM_COMP) {
            int cx = snprintf(
                serial_data + serial_data_length,
                SERIAL_BUFFER_LEN - serial_data_length,
                "CX[%d],CY[%d],CZ[%d],",
                uBit.compass.getX(),
                uBit.compass.getY(),
                uBit.compass.getZ()
            );
            if (cx > 0 && cx < (SERIAL_BUFFER_LEN - serial_data_length)) {
                serial_data_length += cx;
            }
        }
        if (STREAM_BUTTONS) {
            int cx = snprintf(
                serial_data + serial_data_length,
                SERIAL_BUFFER_LEN - serial_data_length,
                "BA[%d],BB[%d],BL[%d],",
                uBit.buttonA.isPressed(),
                uBit.buttonB.isPressed(),
                uBit.logo.isPressed()
            );
            if (cx > 0 && cx < (SERIAL_BUFFER_LEN - serial_data_length)) {
                serial_data_length += cx;
            }
        }

        // If the last character is a comma let it be overwritten
        if (serial_data[serial_data_length - 1] == ',') {
            serial_data_length--;
        }

        // Ensure the string ends with a carriage return and new line
        if (((SERIAL_BUFFER_LEN + 1) - serial_data_length) >= 3) {
            serial_data_length += snprintf(
                serial_data + serial_data_length, 3, "\r\n"
            );
        } else {
            serial_data[SERIAL_BUFFER_LEN - 2] = '\r';
            serial_data[SERIAL_BUFFER_LEN - 1] = '\n';
            serial_data[SERIAL_BUFFER_LEN] = '\0';
            serial_data_length = SERIAL_BUFFER_LEN;
        }

        // For development/debugging purposes, uncomment to print how much
        // free time there is available for other operations
        // uBit.serial.printf("T[%d],", next_packet - uBit.systemTime());

        // Now wait until ready to send the next serial message
        while (uBit.systemTime() < next_packet);
        uBit.serial.send((uint8_t *)serial_data, serial_data_length, SYNC_SLEEP);
        next_packet += PACKET_INTERVAL_MS;
    }
}
