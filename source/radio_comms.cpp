#include "main.h"
#include "radio_comms.h"

/**
 * @brief Stores the callback for the received radio packets.
 */
static radio_data_callback_t radiobridge_data_callback = NULL;

static radio_cmd_func_t radiotx_cmd_functions[RADIO_CMD_TYPE_LEN] = { };


// ----------------------------------------------------------------------------
// SENSOR DATA TX & RX FUNCTIONS ----------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_BRIDGE)
/**
 * @brief Event handler for received radio packets.
 *
 * @param e Message bus event information, not used.
 */
static void radiobridge_onRadioData(MicroBitEvent e) {
    if (radiobridge_data_callback == NULL) return;

    radio_packet_t data;
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
    // TODO: Use a randomised ID instead of a counter
    static uint32_t id = 0;
    id++;

    radio_packet_t data = {
        .packet_type = RADIO_PKT_SENSOR_DATA,
        .cmd_type = RADIO_CMD_INVALID,
        .id = id,
        .mb_id = microbit_serial_number(),
        .sensor_data = {
            .accelerometer_x = uBit.accelerometer.getX(),
            .accelerometer_y = uBit.accelerometer.getY(),
            .accelerometer_z = uBit.accelerometer.getZ(),
            .button_a = (uint8_t)uBit.buttonA.isPressed(),
            .button_b = (uint8_t)uBit.buttonB.isPressed(),
            .button_logo = (uint8_t)uBit.logo.isPressed(),
        },
    };

    uint8_t radio_data[sizeof(data)];
    memcpy(radio_data, &data, sizeof(data));

    uBit.radio.datagram.send(radio_data, sizeof(data));
}
#endif


// ----------------------------------------------------------------------------
// BRIDGE RECEIVER FUNCTIONS --------------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_BRIDGE)
void radiobridge_init(radio_data_callback_t callback, uint8_t radio_frequency) {
    radiobridge_data_callback = callback;
    uBit.radio.enable();
    uBit.radio.setTransmitPower(MICROBIT_RADIO_POWER_LEVELS - 1);
    uBit.radio.setFrequencyBand(radio_frequency);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, radiobridge_onRadioData);
}

void radiobridge_sendCommand(uint32_t mb_id, radio_cmd_type_t cmd, radio_cmd_t *value) {
    // TODO: Use a randomised ID instead of a counter
    static uint32_t id = 0;
    id++;

    radio_packet_t radio_cmd = {
        .packet_type = RADIO_PKT_CMD,
        .cmd_type = cmd,
        .id = id,
        .mb_id = mb_id,
        .cmd_data = { 0 },
    };
    uBit.serial.printf("Sending command %d to %x\n", cmd, mb_id);

    uint8_t radio_data[sizeof(radio_cmd)];
    memcpy(radio_data, &radio_cmd, sizeof(radio_cmd));

    uBit.radio.datagram.send(radio_data, sizeof(radio_data));
}
#endif


// ----------------------------------------------------------------------------
// SENDER FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_SENDER)

static void radiotx_cmd_blink(radio_cmd_t *value) {
    (void)value;

    // Flash the LEDs in a different fiber to avoid blocking the radio
    create_fiber([]() {
        MicroBitImage full("255,255,255,255,255\n255,255,255,255,255\n255,255,255,255,255\n255,255,255,255,255\n255,255,255,255,255\n");
        for (int i = 0; i < 3; i++) {
            uBit.display.print(full);
            uBit.sleep(200);
            uBit.display.clear();
            uBit.sleep(200);
        }
    });
}

static void radiotx_onRadioData(MicroBitEvent e) {
    radio_packet_t received_cmd;
    PacketBuffer radio_packet = uBit.radio.datagram.recv();
    if (radio_packet.length() != sizeof(received_cmd)) {
        // TODO: Maybe ignore the packet instead? or issue error to callback?
        uBit.panic(240);
    }
    memcpy(&received_cmd, radio_packet.getBytes(), sizeof(received_cmd));

    // Ignore packets that are not commands
    if (received_cmd.packet_type != RADIO_PKT_CMD) return;

    // Ignore commands for other boards, mb_id == 0 means command for all boards
    if (received_cmd.mb_id != 0 && received_cmd.mb_id != microbit_serial_number()) return;

    // Execute the command
    radiotx_cmd_functions[received_cmd.cmd_type](&received_cmd.cmd_data);
}

void radiotx_mainLoop(uint8_t radio_frequency) {
    radiotx_cmd_functions[RADIO_CMD_BLINK] = radiotx_cmd_blink;

    uBit.radio.enable();
    uBit.radio.setTransmitPower(MICROBIT_RADIO_POWER_LEVELS - 1);
    uBit.radio.setFrequencyBand(radio_frequency);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, radiotx_onRadioData);


    bool broadcast_sensors = true;

    while (true) {
        radiotx_sendPeriodicData();
        if (uBit.buttonA.isPressed()) {
            // FIXME: For development, start or stop broadcasting data
            broadcast_sensors = !broadcast_sensors;
        }
        uBit.sleep(40);
    }
}
#endif
