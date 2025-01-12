#include "monitoring.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "MONITORING";

static bool s_monitoring_active = false;
static int s_device_count = 0;
static int s_packet_count = 0;

// Device table
static device_info_t s_devices[MAX_DEVICES];
static bool s_device_inUse[MAX_DEVICES];

// Packet list
static packet_info_t s_packets[MAX_PACKETS];

// Helper to add or update device in s_devices
static void add_or_update_device(const uint8_t *mac, int8_t rssi)
{
    time_t now;
    time(&now);

    // Check existing
    for (int i=0; i<MAX_DEVICES; i++) {
        if (s_device_inUse[i] && memcmp(s_devices[i].mac, mac, 6) == 0) {
            s_devices[i].rssi = rssi;
            s_devices[i].lastSeen = now;
            s_devices[i].packetCount++;
            return;
        }
    }
    // Not found, add new
    for (int i=0; i<MAX_DEVICES; i++) {
        if (!s_device_inUse[i]) {
            s_device_inUse[i] = true;
            memcpy(s_devices[i].mac, mac, 6);
            s_devices[i].rssi = rssi;
            s_devices[i].packetCount = 1;
            s_devices[i].firstSeen = now;
            s_devices[i].lastSeen  = now;
            s_device_count++;
            return;
        }
    }
    ESP_LOGW(TAG, "Device table full, ignoring new device");
}

// Helper to add a packet to s_packets
static void add_packet(const uint8_t *mac, int8_t rssi, uint16_t length)
{
    time_t now;
    time(&now);

    if (s_packet_count < MAX_PACKETS) {
        // Insert at s_packet_count
        memcpy(s_packets[s_packet_count].mac, mac, 6);
        s_packets[s_packet_count].rssi = rssi;
        s_packets[s_packet_count].length = length;
        s_packets[s_packet_count].timestamp = now;
        s_packet_count++;
    } else {
        // Optionally, you could implement a ring buffer approach
        ESP_LOGW(TAG, "Packet list full, ignoring new packet");
    }
}

// Promiscuous callback
static void IRAM_ATTR promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (!s_monitoring_active) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_pkt_rx_ctrl_t *rx_ctrl = &pkt->rx_ctrl;

    int8_t rssi = rx_ctrl->rssi;
    uint16_t length = pkt->rx_ctrl.sig_len;

    const uint8_t *payload = pkt->payload;
    uint8_t mac[6];
    // For demonstration, assume source MAC is at offset 10
    memcpy(mac, payload + 10, 6);

    // Add or update device
    add_or_update_device(mac, rssi);

    // Also log this packet
    add_packet(mac, rssi, length);
}

// Start indefinite
esp_err_t monitoring_start(void)
{
    if (s_monitoring_active) {
        return ESP_FAIL;
    }
    s_monitoring_active = true;
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promiscuous_cb));
    ESP_LOGI(TAG, "Monitoring started. current device_count=%d, packet_count=%d",
             s_device_count, s_packet_count);
    return ESP_OK;
}

// Stop indefinite
esp_err_t monitoring_stop(void)
{
    if (!s_monitoring_active) {
        return ESP_FAIL;
    }
    esp_wifi_set_promiscuous(false);
    s_monitoring_active = false;
    ESP_LOGI(TAG, "Monitoring stopped. final device_count=%d, packet_count=%d",
             s_device_count, s_packet_count);
    return ESP_OK;
}

// Clear device table + packet list
void monitoring_clear(void)
{
    s_device_count = 0;
    s_packet_count = 0;
    for (int i=0; i<MAX_DEVICES; i++) {
        s_device_inUse[i] = false;
    }
    // Optionally zero out s_devices
    memset(s_devices, 0, sizeof(s_devices));
    // Zero out s_packets
    memset(s_packets, 0, sizeof(s_packets));

    ESP_LOGI(TAG, "Cleared device and packet data.");
}

// Return device list
device_info_t* monitoring_get_devices(int *count_out)
{
    if (count_out) *count_out = s_device_count;
    if (s_device_count == 0) return NULL;

    device_info_t *list = malloc(sizeof(device_info_t)* s_device_count);
    if (!list) return NULL;

    int idx=0;
    for (int i=0; i<MAX_DEVICES; i++) {
        if (s_device_inUse[i]) {
            list[idx] = s_devices[i];
            idx++;
        }
    }
    return list;
}

// Return packet list
packet_info_t* monitoring_get_packets(int *count_out)
{
    *count_out = s_packet_count;
    if (s_packet_count == 0) return NULL;

    packet_info_t *list = malloc(sizeof(packet_info_t)* s_packet_count);
    if (!list) return NULL;
    memcpy(list, s_packets, sizeof(packet_info_t)* s_packet_count);
    return list;
}
