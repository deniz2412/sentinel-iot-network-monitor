#include "monitoring.h"
#include "mqtt_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Logging tag
static const char *TAG = "MONITORING";

// Global monitoring state
static bool s_monitoring_active = false;

// Device tracking
static device_info_t s_devices[MAX_DEVICES];
static bool s_device_inUse[MAX_DEVICES];
static int s_device_count = 0;

// Packet tracking
static packet_info_t s_packets[MAX_PACKETS];
static int s_packet_count = 0;

// Access Point tracking
static access_point_info_t s_access_points[MAX_ACCESS_POINTS];
static bool s_ap_inUse[MAX_ACCESS_POINTS];
static int s_ap_count = 0;

// Promiscuous mode queue (if needed in future extensions)
static QueueHandle_t packet_queue;

// Helper function to convert time_t to ISO 8601 string
static void time_to_iso8601(time_t t, char *buffer, size_t bufsize) {
    struct tm timeinfo;
    gmtime_r(&t, &timeinfo); // Use UTC time
    // Format: YYYY-MM-DDTHH:MM:SSZ
    strftime(buffer, bufsize, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
}

// Helper function to get authentication mode as string
static void get_auth_mode_string(int authmode, char *buffer, size_t bufsize) {
    switch (authmode) {
        case WIFI_AUTH_OPEN:
            snprintf(buffer, bufsize, "OPEN");
            break;
        case WIFI_AUTH_WEP:
            snprintf(buffer, bufsize, "WEP");
            break;
        case WIFI_AUTH_WPA_PSK:
            snprintf(buffer, bufsize, "WPA-PSK");
            break;
        case WIFI_AUTH_WPA2_PSK:
            snprintf(buffer, bufsize, "WPA2-PSK");
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            snprintf(buffer, bufsize, "WPA/WPA2-PSK");
            break;
        case WIFI_AUTH_WPA3_PSK:
            snprintf(buffer, bufsize, "WPA3-PSK");
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            snprintf(buffer, bufsize, "WPA2/WPA3-PSK");
            break;
        default:
            snprintf(buffer, bufsize, "UNKNOWN");
            break;
    }
}

// Helper function to add or update a device in s_devices
static void add_or_update_device(const uint8_t *mac, int8_t rssi) {
    time_t now;
    time(&now);

    // Check if device already exists
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (s_device_inUse[i] && memcmp(s_devices[i].mac, mac, 6) == 0) {
            // Update existing device
            s_devices[i].rssi = rssi;
            s_devices[i].lastSeen = now;
            s_devices[i].packetCount++;
            return;
        }
    }

    // Add new device if there's space
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!s_device_inUse[i]) {
            s_device_inUse[i] = true;
            memcpy(s_devices[i].mac, mac, 6);
            s_devices[i].rssi = rssi;
            s_devices[i].packetCount = 1;
            s_devices[i].firstSeen = now;
            s_devices[i].lastSeen = now;
            s_device_count++;
            return;
        }
    }

    ESP_LOGW(TAG, "Device table full, ignoring new device");
}

// Helper function to add a packet to s_packets
static void add_packet(const uint8_t *mac, int8_t rssi, uint16_t length) {
    time_t now;
    time(&now);

    if (s_packet_count < MAX_PACKETS) {
        memcpy(s_packets[s_packet_count].mac, mac, 6);
        s_packets[s_packet_count].rssi = rssi;
        s_packets[s_packet_count].length = length;
        s_packets[s_packet_count].timestamp = now;
        s_packet_count++;
    } else {
        ESP_LOGW(TAG, "Packet list full, ignoring new packet");
    }
}

// Helper function to add or update an access point in s_access_points
static void add_or_update_access_point(const wifi_ap_record_t *ap) {
    time_t now;
    time(&now);

    // Check if AP already exists
    for (int i = 0; i < MAX_ACCESS_POINTS; i++) {
        if (s_ap_inUse[i] && memcmp(s_access_points[i].mac, ap->bssid, 6) == 0) {
            // Update existing AP
            s_access_points[i].rssi = ap->rssi;
            s_access_points[i].timestamp = now;
            return;
        }
    }

    // Add new AP if there's space
    for (int i = 0; i < MAX_ACCESS_POINTS; i++) {
        if (!s_ap_inUse[i]) {
            s_ap_inUse[i] = true;
            strncpy(s_access_points[i].ssid, (const char *)ap->ssid, sizeof(s_access_points[i].ssid) - 1);
            s_access_points[i].ssid[sizeof(s_access_points[i].ssid) - 1] = '\0';
            s_access_points[i].rssi = ap->rssi;
            memcpy(s_access_points[i].mac, ap->bssid, 6);
            s_access_points[i].channel = ap->primary;
            get_auth_mode_string(ap->authmode, s_access_points[i].authMode, sizeof(s_access_points[i].authMode));
            s_access_points[i].timestamp = now;
            s_ap_count++;
            return;
        }
    }

    ESP_LOGW(TAG, "Access Point table full, ignoring new AP");
}

// Promiscuous callback to capture packets and extract client device MAC addresses
static void IRAM_ATTR promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!s_monitoring_active) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint16_t length = pkt->rx_ctrl.sig_len;

    // Minimum frame size for data frames
    if (length < 24) return;

    const uint8_t *payload = pkt->payload;

    // Parse Frame Control field
    uint16_t fc = payload[0] | (payload[1] << 8);
    uint8_t frame_type = (fc >> 2) & 0x3;

    // Only process data frames (type 2)
    if (frame_type != 2) return;

    int8_t rssi = pkt->rx_ctrl.rssi;

    // Extract source MAC address from Address 2 field (offset 10)
    uint8_t mac[6];
    memcpy(mac, payload + 10, 6);

    add_or_update_device(mac, rssi);
    add_packet(mac, rssi, length);
}

// Task to periodically publish devices to MQTT
static void publish_devices_task(void *param) {
    char firstSeenStr[25];
    char lastSeenStr[25];
    char mac_str[18];

    while (s_monitoring_active) {
        int count = 0;
        device_info_t *devices = monitoring_get_devices(&count);
        if (devices && count > 0) {
            for (int i = 0; i < count; i++) {
                time_to_iso8601(devices[i].firstSeen, firstSeenStr, sizeof(firstSeenStr));
                time_to_iso8601(devices[i].lastSeen, lastSeenStr, sizeof(lastSeenStr));

                snprintf(mac_str, sizeof(mac_str),
                         "%02x:%02x:%02x:%02x:%02x:%02x",
                         devices[i].mac[0], devices[i].mac[1], devices[i].mac[2],
                         devices[i].mac[3], devices[i].mac[4], devices[i].mac[5]);

                char json[512];
                snprintf(json, sizeof(json),
                         "{\"mac\":\"%s\","
                         "\"rssi\":%d,"
                         "\"packetCount\":%lu,"
                         "\"firstSeen\":\"%s\","
                         "\"lastSeen\":\"%s\"}",
                         mac_str,
                         devices[i].rssi,
                         devices[i].packetCount,
                         firstSeenStr,
                         lastSeenStr);

                mqtt_publish("devices", json);
            }
            free(devices);
            s_device_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Publish every 10 seconds
    }
    vTaskDelete(NULL);
}

// Task to periodically publish packets to MQTT
static void publish_packets_task(void *param) {
    char timestampStr[25];
    char mac_str[18];

    while (s_monitoring_active) {
        int count = 0;
        packet_info_t *packets = monitoring_get_packets(&count);
        if (packets && count > 0) {
            for (int i = 0; i < count; i++) {
                time_to_iso8601(packets[i].timestamp, timestampStr, sizeof(timestampStr));

                snprintf(mac_str, sizeof(mac_str),
                         "%02x:%02x:%02x:%02x:%02x:%02x",
                         packets[i].mac[0], packets[i].mac[1], packets[i].mac[2],
                         packets[i].mac[3], packets[i].mac[4], packets[i].mac[5]);

                char json[512];
                snprintf(json, sizeof(json),
                         "{\"mac\":\"%s\","
                         "\"rssi\":%d,"
                         "\"length\":%u,"
                         "\"timestamp\":\"%s\"}",
                         mac_str,
                         packets[i].rssi,
                         packets[i].length,
                         timestampStr);

                mqtt_publish("packets", json);
            }
            free(packets);
            s_packet_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Publish every 10 seconds
    }
    vTaskDelete(NULL);
}

// Task to periodically publish access points to MQTT
static void publish_access_points_task(void *param) {
    char timestampStr[25];
    char mac_str[18];

    while (s_monitoring_active) {
        int count = 0;
        access_point_info_t *access_points = monitoring_get_access_points(&count);
        if (access_points && count > 0) {
            for (int i = 0; i < count; i++) {
                time_to_iso8601(access_points[i].timestamp, timestampStr, sizeof(timestampStr));

                snprintf(mac_str, sizeof(mac_str),
                         "%02x:%02x:%02x:%02x:%02x:%02x",
                         access_points[i].mac[0], access_points[i].mac[1], access_points[i].mac[2],
                         access_points[i].mac[3], access_points[i].mac[4], access_points[i].mac[5]);

                char json[512];
                snprintf(json, sizeof(json),
                         "{\"ssid\":\"%s\","
                         "\"rssi\":%d,"
                         "\"mac\":\"%s\","
                         "\"channel\":%d,"
                         "\"authMode\":\"%s\","
                         "\"timestamp\":\"%s\"}",
                         access_points[i].ssid,
                         access_points[i].rssi,
                         mac_str,
                         access_points[i].channel,
                         access_points[i].authMode,
                         timestampStr);

                mqtt_publish("access-points", json);
            }
            free(access_points);
            s_ap_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Publish every 10 seconds
    }
    vTaskDelete(NULL);
}

// Function to scan and collect access points
static void scan_access_points(void) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300
            }
        },
    };

    if (esp_wifi_scan_start(&scan_config, true) != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi scan failed");
        return;
    }

    uint16_t ap_count = 0;
    if (esp_wifi_scan_get_ap_num(&ap_count) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP count");
        return;
    }

    if (ap_count == 0) {
        ESP_LOGI(TAG, "No APs found");
        return;
    }

    wifi_ap_record_t *ap_list = malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_list) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP list");
        return;
    }

    if (esp_wifi_scan_get_ap_records(&ap_count, ap_list) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP records");
        free(ap_list);
        return;
    }

    for (int i = 0; i < ap_count && i < MAX_ACCESS_POINTS; i++) {
        wifi_ap_record_t *ap = &ap_list[i];
        add_or_update_access_point(ap);
    }

    free(ap_list);
}

// Task to continuously scan for access points
static void access_point_scanning_task(void *param) {
    while (s_monitoring_active) {
        scan_access_points();
        vTaskDelay(pdMS_TO_TICKS(10000)); // Scan every 10 seconds
    }
    vTaskDelete(NULL);
}

// Start indefinite monitoring: packet capturing, AP scanning, and publishing tasks
esp_err_t monitoring_start(void) {
    if (s_monitoring_active) {
        ESP_LOGW(TAG, "Monitoring already active");
        return ESP_FAIL;
    }

    // Initialize tracking structures
    memset(s_device_inUse, 0, sizeof(s_device_inUse));
    memset(s_devices, 0, sizeof(s_devices));
    memset(s_packets, 0, sizeof(s_packets));
    memset(s_ap_inUse, 0, sizeof(s_ap_inUse));
    memset(s_access_points, 0, sizeof(s_access_points));
    s_device_count = 0;
    s_packet_count = 0;
    s_ap_count = 0;

    // Enable promiscuous mode and set callback
    esp_err_t ret = esp_wifi_set_promiscuous(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set promiscuous mode: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_promiscuous_rx_cb(promiscuous_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set promiscuous callback: %s", esp_err_to_name(ret));
        esp_wifi_set_promiscuous(false);
        return ret;
    }

    s_monitoring_active = true;

    // Start access point scanning task
    if (xTaskCreate(access_point_scanning_task, "AP_Scan_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create AP scanning task");
        esp_wifi_set_promiscuous(false);
        s_monitoring_active = false;
        return ESP_FAIL;
    }

    // Start publishing tasks
    if (xTaskCreate(publish_devices_task, "Pub_Devices_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Publish Devices task");
        s_monitoring_active = false;
        esp_wifi_set_promiscuous(false);
        return ESP_FAIL;
    }

    if (xTaskCreate(publish_packets_task, "Pub_Packets_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Publish Packets task");
        s_monitoring_active = false;
        esp_wifi_set_promiscuous(false);
        return ESP_FAIL;
    }

    if (xTaskCreate(publish_access_points_task, "Pub_APs_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Publish Access Points task");
        s_monitoring_active = false;
        esp_wifi_set_promiscuous(false);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Indefinite monitoring started");
    return ESP_OK;
}

// Stop indefinite monitoring
esp_err_t monitoring_stop(void) {
    if (!s_monitoring_active) {
        ESP_LOGW(TAG, "Monitoring not active");
        return ESP_FAIL;
    }

    // Stop all tasks by setting monitoring_active to false
    s_monitoring_active = false;

    // Disable promiscuous mode
    esp_err_t ret = esp_wifi_set_promiscuous(false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable promiscuous mode: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Indefinite monitoring stopped");
    return ESP_OK;
}

// Clear all tracking data
void monitoring_clear(void) {
    memset(s_device_inUse, 0, sizeof(s_device_inUse));
    memset(s_devices, 0, sizeof(s_devices));
    memset(s_packets, 0, sizeof(s_packets));
    memset(s_ap_inUse, 0, sizeof(s_ap_inUse));
    memset(s_access_points, 0, sizeof(s_access_points));
    s_device_count = 0;
    s_packet_count = 0;
    s_ap_count = 0;

    ESP_LOGI(TAG, "All monitoring data cleared");
}

// Retrieve devices data
device_info_t* monitoring_get_devices(int *count_out) {
    if (count_out) *count_out = s_device_count;
    if (s_device_count == 0) return NULL;

    device_info_t *list = malloc(sizeof(device_info_t) * s_device_count);
    if (!list) return NULL;

    int idx = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (s_device_inUse[i]) {
            list[idx++] = s_devices[i];
        }
    }
    return list;
}

// Retrieve packets data
packet_info_t* monitoring_get_packets(int *count_out) {
    if (count_out) *count_out = s_packet_count;
    if (s_packet_count == 0) return NULL;

    packet_info_t *list = malloc(sizeof(packet_info_t) * s_packet_count);
    if (!list) return NULL;

    memcpy(list, s_packets, sizeof(packet_info_t) * s_packet_count);
    return list;
}

// Retrieve access points data
access_point_info_t* monitoring_get_access_points(int *count_out) {
    if (count_out) *count_out = s_ap_count;
    if (s_ap_count == 0) return NULL;

    access_point_info_t *list = malloc(sizeof(access_point_info_t) * s_ap_count);
    if (!list) return NULL;

    int idx = 0;
    for (int i = 0; i < MAX_ACCESS_POINTS; i++) {
        if (s_ap_inUse[i]) {
            list[idx++] = s_access_points[i];
        }
    }
    return list;
}
