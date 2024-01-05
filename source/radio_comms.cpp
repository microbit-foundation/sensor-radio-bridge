#include "main.h"
#include "radio_comms.h"

/**
 * @brief Stores the callback for the received radio packets.
 */
static radio_data_callback_t radiobridge_data_callback = NULL;


// ----------------------------------------------------------------------------
// SENSOR DATA TX & RX FUNCTIONS ----------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_RECEIVER)
/**
 * @brief Event handler for received radio packets.
 *
 * @param e Message bus event information, not used.
 */
static void radiobridge_onRadioData(MicroBitEvent e) {
    if (radiobridge_data_callback == NULL) return;

    radio_sensor_data_t data;
    PacketBuffer radio_packet = uBit.radio.datagram.recv();
    if (radio_packet.length() != sizeof(data)) {
        // TODO: Maybe ignore the packet instead? or issue error to callback?
        uBit.panic(240);
    }
    memcpy(&data, radio_packet.getBytes(), sizeof(data));

    radiobridge_data_callback(&data);
}
#endif

#if CONFIG_ENABLED(RADIO_SENDER)
/**
 * @brief Sends the periodic radio data.
 */
static void radiotx_sendPeriodicData() {
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
// BRIDGE RECEIVER FUNCTIONS --------------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_RECEIVER)
void radiobridge_init(radio_data_callback_t callback, uint8_t radio_frequency) {
    radiobridge_data_callback = callback;
    uBit.radio.enable();
    uBit.radio.setTransmitPower(MICROBIT_RADIO_POWER_LEVELS - 1);
    uBit.radio.setFrequencyBand(radio_frequency);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, radiobridge_onRadioData);
}
#endif


// ----------------------------------------------------------------------------
// SENDER FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_SENDER)
void radiotx_mainLoop(uint8_t radio_frequency) {
    uBit.radio.enable();
    uBit.radio.setTransmitPower(MICROBIT_RADIO_POWER_LEVELS - 1);
    uBit.radio.setFrequencyBand(radio_frequency);

    while (true) {
        radiotx_sendPeriodicData();
        uBit.sleep(40);
    }
}
#endif
