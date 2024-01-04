#include "main.h"
#include "radio_comms.h"

/**
 * @brief Stores the callback for the received radio packets.
 */
static radio_data_callback_t data_callback = NULL;

// ----------------------------------------------------------------------------
// PRIVATE FUNCTIONS ----------------------------------------------------------
// ----------------------------------------------------------------------------

#if CONFIG_ENABLED(RADIO_RECEIVER)
/**
 * @brief Event handler for received radio packets.
 *
 * @param e Message bus event information, not used.
 */
static void onRadioData(MicroBitEvent e) {
    if (data_callback == NULL) {
        return;
    }

    radio_sensor_data_t data;
    PacketBuffer radio_packet = uBit.radio.datagram.recv();
    if (radio_packet.length() != sizeof(data)) {
        // TODO: We should probably ignore the packet instead of panicking
        //       or issue an error to the callback
        uBit.panic(240);
    }
    memcpy(&data, radio_packet.getBytes(), sizeof(data));

    data_callback(&data);
}
#endif

#if CONFIG_ENABLED(RADIO_SENDER)
/**
 * @brief Sends the radio data.
 */
static void send_radio_data() {
    radio_sensor_data_t data = {
        .mb_id = microbit_serial_number(),
        .accelerometer_x = uBit.accelerometer.getX(),
        .accelerometer_y = uBit.accelerometer.getY(),
        .accelerometer_z = uBit.accelerometer.getZ(),
        .button_a = (uint8_t)uBit.buttonA.isPressed(),
        .button_b = (uint8_t)uBit.buttonB.isPressed(),
        .button_logo = (uint8_t)uBit.logo.isPressed(),
    };
    uint8_t radio_data[sizeof(data)];

    memcpy(radio_data, &data, sizeof(data));

    uBit.radio.datagram.send(radio_data, sizeof(data));
}
#endif

// ----------------------------------------------------------------------------
// PUBLIC FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------

#if CONFIG_ENABLED(RADIO_RECEIVER)
void radio_receive_init(radio_data_callback_t callback) {
    data_callback = callback;
    uBit.radio.enable();
    uBit.radio.setGroup(RADIO_GROUP_DEFAULT);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onRadioData);
}
#endif

#if CONFIG_ENABLED(RADIO_SENDER)
void radio_send_main_loop() {
    uBit.radio.enable();
    uBit.radio.setGroup(RADIO_GROUP_DEFAULT);

    while (true) {
        send_radio_data();
        uBit.sleep(40);
    }
}
#endif
