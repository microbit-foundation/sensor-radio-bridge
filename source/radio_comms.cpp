#include "main.h"
#include "radio_comms.h"

static void send_radio_data() {
    const uint8_t payload_size = 15;
    uint8_t radio_data[payload_size];

    int accelerometer_x = uBit.accelerometer.getX();
    int accelerometer_y = uBit.accelerometer.getY();
    int accelerometer_z = uBit.accelerometer.getZ();
    memcpy(&radio_data[0], &accelerometer_x, 4);
    memcpy(&radio_data[4], &accelerometer_y, 4);
    memcpy(&radio_data[8], &accelerometer_z, 4);
    radio_data[12] = uBit.buttonA.isPressed();
    radio_data[13] = uBit.buttonB.isPressed();
    radio_data[14] = uBit.logo.isPressed();

    uBit.radio.datagram.send(radio_data, payload_size);
}


void radio_send_main_loop() {
    uBit.radio.enable();
    uBit.radio.setGroup(RADIO_GROUP_DEFAULT);

    while (true) {
        send_radio_data();
        uBit.sleep(40);
    }
}
