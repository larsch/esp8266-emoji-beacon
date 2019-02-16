#include <Arduino.h>
#include <SoftwareSerial.h>
#include "user_interface.h"

#include "emojis.h"

struct beacon_fixed
{
    uint16_t frame_control;
    uint16_t duration;
    uint8_t destination[6];
    uint8_t source[6];
    uint8_t bssid[6];
    uint16_t seq_ctrl;
    uint64_t timestamp;
    uint16_t beacon_interval;
    uint16_t capabilities;
} __attribute__((packed));

static_assert(sizeof(beacon_fixed) == 36, "36");

#define PARM_SSID 0x00
#define PARM_SUPPORTED_RATES 0x01
#define PARM_CURRENT_CHANNEL 0x03

struct beacon_packet
{
    beacon_fixed fixed;
    uint8_t parameters[92];
} __attribute__((packed));

static_assert(sizeof(beacon_packet) == 128, "128");

/** Current beacon buffer */
static beacon_packet beacon = {
    .fixed = {
        .frame_control = htons(0x8000),
        .duration = htons(0x0000),
        .destination = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .source = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
        .bssid = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
        .seq_ctrl = htons(0xc06c),
        .timestamp = 0x00,
        .beacon_interval = htons(0xff00),
        .capabilities = htons(0x0104),
    },
    .parameters = {},
};

/** Current beacon packet size */
static size_t beacon_size;

/** supported rates, inserted after SSID */
static const uint8_t supported_rates[] = {
    PARM_SUPPORTED_RATES,
    0x08,
    0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c};

/** Generate a beacon packet with the specified SSID, BSSID and channel number */
static void generate_beacon(const uint8_t *ssid, size_t ssidlength, uint8_t bssid[6], uint8_t channel)
{
    memcpy(beacon.fixed.source, bssid, 6);
    memcpy(beacon.fixed.bssid, bssid, 6);
    uint8_t *param = &beacon.parameters[0];
    size_t paramlen = 0;
    /* SSID */
    param[paramlen++] = PARM_SSID;
    param[paramlen++] = ssidlength;
    memcpy(param + paramlen, ssid, ssidlength);
    paramlen += ssidlength;
    /* Supported rates */
    memcpy(param + paramlen, supported_rates, sizeof(supported_rates));
    paramlen += sizeof(supported_rates);
    /* Current channel */
    param[paramlen++] = PARM_CURRENT_CHANNEL;
    param[paramlen++] = 1; /* length */
    param[paramlen++] = channel;

    beacon_size = sizeof(beacon_fixed) + paramlen;
}

/** Current position in emoji list */
static size_t emoji_position = 0;

/** Start time of current iteration */
uint32_t start_time = millis();

/** Number of emojis sent this iteration */
uint32_t beacon_count = 0;

/** generate and send the next beacon */
static void send_next_beacon()
{
    int channel = 6;
    wifi_set_channel(channel);
    uint8_t bssid[] = {0x80, 0x29, 0xbe, 0xef, emoji_position >> 8, emoji_position};
    size_t ssidlen = emojis[emoji_position++];
    const uint8_t *ssid = &emojis[emoji_position];
    emoji_position += ssidlen;
    if (emoji_position == sizeof(emojis))
        emoji_position = 0;

    ++beacon_count;

    if (emoji_position == 0)
    {
        uint32_t now = millis();
        Serial.print(beacon_count);
        Serial.print(" beacons sent in ");
        Serial.print(now - start_time);
        Serial.println(" ms");
        start_time = now;
        beacon_count = 0;
    }

    generate_beacon(ssid, ssidlen, bssid, channel);
    wifi_send_pkt_freedom((uint8_t *)&beacon, beacon_size, 0);
}

/** Callback; tx complete */
static void send_complete(uint8_t status)
{
    send_next_beacon();
}

/** Callback; rx (unused) */
static void rx_cb(uint8_t *buf, uint16_t len)
{
}

void setup()
{
    Serial.begin(115200);

    wifi_set_opmode(STATION_MODE);
    wifi_set_channel(1);
    wifi_set_promiscuous_rx_cb(rx_cb);
    wifi_promiscuous_enable(1);

    wifi_register_send_pkt_freedom_cb(send_complete);

    send_next_beacon();
}

void loop()
{
}