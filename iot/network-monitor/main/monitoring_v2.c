#include "monitoring_v2.h"
#include "mqtt_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <string.h>
#include <time.h>


static const char *TAG = "MONITORING";

// Global variables and data structures
static bool s_monitoring_active = false;
static SemaphoreHandle_t device_list_mutex = NULL;
static SemaphoreHandle_t mqtt_mutex = NULL;
// Arrays and counts

static QueueHandle_t packet_queue = NULL;
static access_point_info_t s_access_points[MAX_ACCESS_POINTS];
static int s_ap_count = 0;

static observed_device_t observed_devices[MAX_OBSERVED_DEVICES];
static int observed_device_count = 0;


static void IRAM_ATTR promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!s_monitoring_active) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint16_t length = pkt->rx_ctrl.sig_len;
    if (length < 24) return;

    const uint8_t *payload = pkt->payload;
    uint16_t fc = payload[0] | (payload[1] << 8);
    uint8_t frame_type = (fc >> 2) & 0x3;
    if (frame_type != 2) return;  // Only process data frames

    int8_t rssi = pkt->rx_ctrl.rssi;
    uint8_t mac[6];
    memcpy(mac, payload + 10, 6);

    // Update observed devices list safely using ISR-safe functions
    if (device_list_mutex != NULL) {
        BaseType_t higherTaskWoken = pdFALSE;
        if (xSemaphoreTakeFromISR(device_list_mutex, &higherTaskWoken) == pdTRUE) {
            int found = 0;
            time_t current_time = time(NULL);  // Caution: using time() in ISR

            for (int i = 0; i < observed_device_count; i++) {
                if (memcmp(observed_devices[i].mac, mac, 6) == 0) {
                    observed_devices[i].rssi = rssi;
                    observed_devices[i].lastSeen = current_time;
                    found = 1;
                    break;
                }
            }

            if (!found && observed_device_count < MAX_OBSERVED_DEVICES) {
                memcpy(observed_devices[observed_device_count].mac, mac, 6);
                observed_devices[observed_device_count].rssi = rssi;
                observed_devices[observed_device_count].lastSeen = current_time;
                // Initialize IP as unknown if necessary:
                // ip4_addr_set_zero(&(observed_devices[observed_device_count].ip));
                observed_device_count++;
            }

            xSemaphoreGiveFromISR(device_list_mutex, &higherTaskWoken);
            portYIELD_FROM_ISR(higherTaskWoken);
        }
    }

    // Create a packet_info_t structure to enqueue
    packet_info_t packet;
    memcpy(packet.mac, mac, 6);
    packet.rssi = rssi;
    packet.length = length;
    packet.timestamp = time(NULL); 

    // Enqueue the packet if the queue is available
    if (packet_queue != NULL) {
        BaseType_t ret = xQueueSendFromISR(packet_queue, &packet, NULL);
        if (ret != pdPASS) {
            ESP_EARLY_LOGW(TAG, "Packet queue full, dropping packet");
        } else {
            ESP_EARLY_LOGI(TAG, "Packet enqueued successfully");
        }
    }
}

// Helper functions for time and other
static void time_to_iso8601(time_t t, char *buffer, size_t bufsize) {
    struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
    strftime(buffer, bufsize, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
}

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
        default:
            snprintf(buffer, bufsize, "UNKNOWN");
            break;
    }
}

// Mutex-protected MQTT publish
static void safe_mqtt_publish(const char *topic_suffix, const char *message) {
    if (!mqtt_mutex) return;
    if (xSemaphoreTake(mqtt_mutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
        mqtt_publish(topic_suffix, message);
        xSemaphoreGive(mqtt_mutex);
    } else {
        ESP_LOGW(TAG, "Timeout acquiring MQTT mutex for topic %s", topic_suffix);
    }
}

/* Access Point Scan Task */
static void scan_access_points() {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active = { .min = 100, .max = 300 },
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
        ESP_LOGE(TAG, "Memory allocation for AP list failed");
        return;
    }

    if (esp_wifi_scan_get_ap_records(&ap_count, ap_list) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP records");
        free(ap_list);
        return;
    }

    // Reset AP count before adding new ones
    s_ap_count = 0;
    memset(s_access_points, 0, sizeof(s_access_points));

    for (int i = 0; i < ap_count && i < MAX_ACCESS_POINTS; i++) {
        wifi_ap_record_t *ap = &ap_list[i];
        time_t now;
        time(&now);

        strncpy(s_access_points[s_ap_count].ssid, (const char *)ap->ssid, sizeof(s_access_points[s_ap_count].ssid) - 1);
        s_access_points[s_ap_count].ssid[sizeof(s_access_points[s_ap_count].ssid) - 1] = '\0';
        s_access_points[s_ap_count].rssi = ap->rssi;
        memcpy(s_access_points[s_ap_count].mac, ap->bssid, 6);
        s_access_points[s_ap_count].channel = ap->primary;
        get_auth_mode_string(ap->authmode, s_access_points[s_ap_count].authMode, sizeof(s_access_points[s_ap_count].authMode));
        s_access_points[s_ap_count].timestamp = now;
        s_ap_count++;
    }

    free(ap_list);
}

void start_access_point_scan_task(void *param) {
    while (s_monitoring_active) {
        // Configure and start Wi-Fi scan for APs
        scan_access_points();
        // For each detected AP, create JSON and publish
        char timestampStr[25];
        time_t now;
        time(&now);
        time_to_iso8601(now, timestampStr, sizeof(timestampStr));
        
        for (int i = 0; i < s_ap_count; i++) {
            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str),
                     "%02x:%02x:%02x:%02x:%02x:%02x",
                     s_access_points[i].mac[0], s_access_points[i].mac[1], 
                     s_access_points[i].mac[2], s_access_points[i].mac[3], 
                     s_access_points[i].mac[4], s_access_points[i].mac[5]);

            char json[512];
            snprintf(json, sizeof(json),
                     "{\"ssid\":\"%s\",\"rssi\":%d,\"mac\":\"%s\",\"channel\":%d,\"authMode\":\"%s\",\"timestamp\":\"%s\"}",
                     s_access_points[i].ssid, s_access_points[i].rssi, mac_str,
                     s_access_points[i].channel, s_access_points[i].authMode, timestampStr);

            safe_mqtt_publish("access-points", json);
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait 10 seconds before next scan
    }
    vTaskDelete(NULL);
}

void start_device_scan_task(void *param) {
    char timestampStr[25];
    char mac_str[18];

    while (s_monitoring_active) {
        if (device_list_mutex && xSemaphoreTake(device_list_mutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
            int local_count = observed_device_count;
            observed_device_t local_devices[MAX_OBSERVED_DEVICES];
            memcpy(local_devices, observed_devices, sizeof(observed_device_t) * local_count);
            xSemaphoreGive(device_list_mutex);

            time_t now;
            time(&now);
            time_to_iso8601(now, timestampStr, sizeof(timestampStr));

            for (int i = 0; i < local_count; i++) {
                snprintf(mac_str, sizeof(mac_str),
                         "%02x:%02x:%02x:%02x:%02x:%02x",
                         local_devices[i].mac[0], local_devices[i].mac[1],
                         local_devices[i].mac[2], local_devices[i].mac[3],
                         local_devices[i].mac[4], local_devices[i].mac[5]);

                char json[1024];
                snprintf(json, sizeof(json),
                         "{\"mac\":\"%s\",\"rssi\":%d,\"lastSeen\":\"%s\"\"}",
                         mac_str,
                         local_devices[i].rssi,
                         timestampStr
                         );

                safe_mqtt_publish("devices", json);
            }
        } else {
            ESP_LOGW(TAG, "Timeout acquiring device_list_mutex");
        }

        vTaskDelay(pdMS_TO_TICKS(60000)); // Wait one minute before next scan
    }

    ESP_LOGI(TAG, "Device scan task terminating");
    vTaskDelete(NULL);
}

void start_packet_batch_task(void *param) {
    const int batch_threshold = 20; // Batch size before publishing
    packet_info_t batch[batch_threshold];
    int batch_count = 0;
    const TickType_t queue_wait_time = pdMS_TO_TICKS(5000); // Queue wait time

    while (s_monitoring_active) {
        // Wait for packets from the queue
        packet_info_t packet;
        if (xQueueReceive(packet_queue, &packet, queue_wait_time) == pdPASS) {
            batch[batch_count++] = packet; // Add packet to the batch

            // Check if batch size reached the threshold
            if (batch_count >= batch_threshold) {
                // Convert batch to JSON and publish
                size_t json_size = 1024 + (batch_count * 200); // Adjust size estimation
                char *json_buffer = malloc(json_size);
                if (json_buffer) {
                    memset(json_buffer, 0, json_size);
                    size_t offset = snprintf(json_buffer, json_size, "{\"packets\":[");

                    for (int i = 0; i < batch_count; i++) {
                        char timestampStr[25];
                        time_to_iso8601(batch[i].timestamp, timestampStr, sizeof(timestampStr));
                        char mac_str[18];
                        snprintf(mac_str, sizeof(mac_str),
                                 "%02x:%02x:%02x:%02x:%02x:%02x",
                                 batch[i].mac[0], batch[i].mac[1], batch[i].mac[2],
                                 batch[i].mac[3], batch[i].mac[4], batch[i].mac[5]);

                        offset += snprintf(json_buffer + offset, json_size - offset,
                                           "{\"mac\":\"%s\",\"rssi\":%d,\"length\":%u,\"timestamp\":\"%s\"}",
                                           mac_str, batch[i].rssi, batch[i].length, timestampStr);

                        if (i < batch_count - 1) {
                            offset += snprintf(json_buffer + offset, json_size - offset, ",");
                        }
                    }
                    snprintf(json_buffer + offset, json_size - offset, "]}");

                    // Publish to MQTT
                    safe_mqtt_publish("packets", json_buffer);
                    free(json_buffer);
                } else {
                    ESP_LOGE(TAG, "Failed to allocate JSON buffer for packet batch");
                }

                // Reset the batch
                batch_count = 0;
            }
        } else {
            // Timeout while waiting for packets
            if (batch_count > 0) {
                // Publish remaining packets even if batch isn't full
                size_t json_size = 1024 + (batch_count * 200);
                char *json_buffer = malloc(json_size);
                if (json_buffer) {
                    memset(json_buffer, 0, json_size);
                    size_t offset = snprintf(json_buffer, json_size, "{\"packets\":[");

                    for (int i = 0; i < batch_count; i++) {
                        char timestampStr[25];
                        time_to_iso8601(batch[i].timestamp, timestampStr, sizeof(timestampStr));
                        char mac_str[18];
                        snprintf(mac_str, sizeof(mac_str),
                                 "%02x:%02x:%02x:%02x:%02x:%02x",
                                 batch[i].mac[0], batch[i].mac[1], batch[i].mac[2],
                                 batch[i].mac[3], batch[i].mac[4], batch[i].mac[5]);

                        offset += snprintf(json_buffer + offset, json_size - offset,
                                           "{\"mac\":\"%s\",\"rssi\":%d,\"length\":%u,\"timestamp\":\"%s\"}",
                                           mac_str, batch[i].rssi, batch[i].length, timestampStr);

                        if (i < batch_count - 1) {
                            offset += snprintf(json_buffer + offset, json_size - offset, ",");
                        }
                    }
                    snprintf(json_buffer + offset, json_size - offset, "]}");

                    // Publish to MQTT
                    safe_mqtt_publish("packets", json_buffer);
                    free(json_buffer);
                } else {
                    ESP_LOGE(TAG, "Failed to allocate JSON buffer for partial packet batch");
                }

                // Reset the batch
                batch_count = 0;
            }
        }
    }

    ESP_LOGI(TAG, "Packet batch task terminating");
    vTaskDelete(NULL);
}

/* Monitoring Start and Stop Functions */
esp_err_t monitoring_start(void) {
    if (s_monitoring_active) return ESP_FAIL;
    s_monitoring_active = true;

    // Initialize mutex for MQTT publishing
    mqtt_mutex = xSemaphoreCreateMutex();
    if (mqtt_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT mutex");
        return ESP_FAIL;
    }
    
    // Create mutex for protecting the observed devices list
    device_list_mutex = xSemaphoreCreateMutex();
    if (!device_list_mutex) {
        ESP_LOGE(TAG, "Failed to create device list mutex");
        // Cleanup previously created mutexes if necessary
        if (mqtt_mutex) {
            vSemaphoreDelete(mqtt_mutex);
            mqtt_mutex = NULL;
        }
        return ESP_FAIL;
    }

    packet_queue = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(packet_info_t));
    if (!packet_queue) {
        ESP_LOGE(TAG, "Failed to create packet queue");
        return ESP_FAIL;
    }

    // Initialize other data structures and counters if needed
    observed_device_count = 0;
    memset(observed_devices, 0, sizeof(observed_devices));

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuous_cb);

    // Create tasks
    if (xTaskCreate(start_access_point_scan_task, "AP_Scan_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create AP scanning task");
        return ESP_FAIL;
    }
    if (xTaskCreate(start_device_scan_task, "Dev_Scan_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Device scanning task");
        return ESP_FAIL;
    }
    if (xTaskCreate(start_packet_batch_task, "Pkt_Batch_Task", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Packet batch task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Monitoring started");
    return ESP_OK;
}

esp_err_t monitoring_stop(void) {
    if (!s_monitoring_active) return ESP_FAIL;
    s_monitoring_active = false;

    if (mqtt_mutex) {
        vSemaphoreDelete(mqtt_mutex);
        mqtt_mutex = NULL;
    }

    if (device_list_mutex) {
        vSemaphoreDelete(device_list_mutex);
        device_list_mutex = NULL;
    }

    ESP_LOGI(TAG, "Monitoring stopped");
    return ESP_OK;
}
