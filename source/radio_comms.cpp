#include "main.h"
#include "radio_comms.h"
#include "mb_images.h"

#if CONFIG_ENABLED(RADIO_REMOTE)
/**
 * @brief Types for callbacks to execute when receiving cmds from the bridge.
 */
typedef void (*radio_cmd_func_t)(radio_cmd_t *value);

/**
 * @brief Stores the remote micro:bit callback functions for the received
 * radio commands.
 */
static radio_cmd_func_t radiotx_cmd_functions[RADIO_CMD_TYPE_LEN] = { };
#endif

#if CONFIG_ENABLED(RADIO_BRIDGE)
/**
 * @brief Stores the callback for the received radio packets.
 */
static radio_data_callback_t radiobridge_data_callback = NULL;

/**
 * @brief Stores the list of micro:bit IDs that have been seen recently,
 * the time they were last seen, and the index of the active micro:bit.
 */
static const size_t MB_IDS_LEN = 32;
static const size_t MB_IDS_ID = 0;
static const size_t MB_IDS_TIME = 1;
static uint32_t mb_ids[MB_IDS_LEN][2] = { };
static size_t active_mb_id_i = MB_IDS_LEN;
#define GET_ACTIVE_MB_ID()      mb_ids[active_mb_id_i][MB_IDS_ID]
#endif

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

#if CONFIG_ENABLED(RADIO_REMOTE)
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
        .padding = 0,
        .id = id,
        .mb_id = microbit_serial_number(),
        .sensor_data = {
            .accelerometer_x = uBit.accelerometer.getX(),
            .accelerometer_y = uBit.accelerometer.getY(),
            .accelerometer_z = uBit.accelerometer.getZ(),
            .button_a = (uint8_t)uBit.buttonA.isPressed(),
            .button_b = (uint8_t)uBit.buttonB.isPressed(),
            .button_logo = (uint8_t)uBit.logo.isPressed(),
            .padding = 0,
        },
    };

    uint8_t radio_data[sizeof(data)];
    memcpy(radio_data, &data, sizeof(data));

    uBit.radio.datagram.send(radio_data, sizeof(data));
}
#endif


// ----------------------------------------------------------------------------
// BRIDGE FUNCTIONS -----------------------------------------------------------
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
        .padding = 0,
        .id = id,
        .mb_id = mb_id,
        .cmd_data = { },
    };
    uBit.serial.printf("Sending command %d to %x\n", cmd, mb_id);

    uint8_t radio_data[sizeof(radio_cmd)];
    memcpy(radio_data, &radio_cmd, sizeof(radio_cmd));

    uBit.radio.datagram.send(radio_data, sizeof(radio_data));
}

void radiobridge_updateRemoteMbIds(uint32_t mb_id) {
    static const uint32_t TIME_TO_FORGET_MS = 2000;

    uint32_t now = uBit.systemTime();

    // If we don't have an active micro:bit, add to array and set as active
    if (active_mb_id_i == MB_IDS_LEN) {
        mb_ids[0][MB_IDS_ID] = mb_id;
        mb_ids[0][MB_IDS_TIME] = now;
        active_mb_id_i = 0;
        return;
    }

    bool found_mb_id = false;
    size_t oldest_inactive_mb_id_time = 0xFFFFFFFF;
    size_t oldest_inactive_mb_id_index = 0;
    for (size_t i = 0; i < MB_IDS_LEN; i++) {
        if (mb_ids[i][MB_IDS_ID] == mb_id) {
            mb_ids[i][MB_IDS_TIME] = now;
            found_mb_id = true;
        } else {
            if (mb_ids[i][MB_IDS_TIME] < (now - TIME_TO_FORGET_MS) && mb_ids[i][MB_IDS_ID] != GET_ACTIVE_MB_ID()) {
                // It's been too long since this inactive micro:bit was heard, forget it
                mb_ids[i][MB_IDS_ID] = 0;
                mb_ids[i][MB_IDS_TIME] = 0;
            }
            if (mb_ids[i][MB_IDS_TIME] < oldest_inactive_mb_id_time && mb_ids[i][MB_IDS_ID] != GET_ACTIVE_MB_ID()) {
                oldest_inactive_mb_id_index = i;
                oldest_inactive_mb_id_time = mb_ids[i][1];
            }
        }
    }
    if (!found_mb_id) {
        mb_ids[oldest_inactive_mb_id_index][MB_IDS_ID] = mb_id;
        mb_ids[oldest_inactive_mb_id_index][MB_IDS_TIME] = now;
    }
}

/**
 * @brief Switches the active micro:bit to the next one active in the list.
 */
void radiobridge_switchNextRemoteMicrobit() {
    // Debounce to only allow switching once per second
    static uint32_t last_switch_time = 0;
    if (uBit.systemTime() < (last_switch_time + 1000)) return;
    last_switch_time = uBit.systemTime();

    // Nothing to do if there is no active micro:bit
    if (active_mb_id_i == MB_IDS_LEN) return;
    // Or if the active micro:bit somehow does not have an ID assigned
    // TODO: This is an error condition, we should do something to recover
    if (GET_ACTIVE_MB_ID() == 0) return;

    // Rotate the active micro:bit ID to the next one in the mb_ids array that has a value
    // If there isn't any other active micro:bits, then the current active will be picked again
    size_t next_active_mb_id_i = active_mb_id_i;
    do {
        next_active_mb_id_i = (next_active_mb_id_i + 1) % MB_IDS_LEN;
    } while (mb_ids[next_active_mb_id_i][MB_IDS_ID] == 0);
    active_mb_id_i = next_active_mb_id_i;
    radiobridge_sendCommand(GET_ACTIVE_MB_ID(), RADIO_CMD_BLINK);
}

uint32_t radiobridge_getActiveRemoteMbId() {
    return GET_ACTIVE_MB_ID();
}
#endif


// ----------------------------------------------------------------------------
// REMOTE FUNCTIONS -----------------------------------------------------------
// ----------------------------------------------------------------------------
#if CONFIG_ENABLED(RADIO_REMOTE)

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
        uBit.display.print(IMG_RUNNING);
    });
}

static void radiotx_onRadioData(MicroBitEvent e) {
    radio_packet_t received_cmd;
    PacketBuffer radio_packet = uBit.radio.datagram.recv();
    if (radio_packet.length() != sizeof(received_cmd)) {
        // TODO: Maybe ignore the packet instead? or issue error to callback?
        uBit.panic(241);
    }
    memcpy(&received_cmd, radio_packet.getBytes(), sizeof(received_cmd));

    // Ignore packets that are not commands
    if (received_cmd.packet_type != RADIO_PKT_CMD) return;

    // Ignore commands for other boards, mb_id == 0 means command for all boards
    if (received_cmd.mb_id != 0 && received_cmd.mb_id != microbit_serial_number()) return;

    // Execute the command
    radiotx_cmd_functions[received_cmd.cmd_type](&received_cmd.cmd_data);
}

void radiotx_mainLoop() {
    radiotx_cmd_functions[RADIO_CMD_BLINK] = radiotx_cmd_blink;

    // Configure the radio, and configure frequency based on this micro:bit's ID
    uBit.radio.enable();
    uBit.radio.setTransmitPower(MICROBIT_RADIO_POWER_LEVELS - 1);
    uint8_t radio_frequency = radio_getFrequencyFromId(microbit_serial_number());
    uBit.radio.setFrequencyBand(radio_frequency);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, radiotx_onRadioData);

    uBit.display.print(IMG_RUNNING);
    bool broadcast_sensors = true;

    while (true) {
        if (broadcast_sensors) {
            radiotx_sendPeriodicData();
        }

#if CONFIG_ENABLED(DEV_MODE)
        // For development and testing, start or stop broadcasting data
        if (uBit.buttonA.isPressed()) {
            broadcast_sensors = !broadcast_sensors;
            uBit.display.print(broadcast_sensors ? IMG_RUNNING : IMG_WAITING);
            uBit.sleep(300);
        }
#endif

        uBit.sleep(10);
    }
}
#endif
