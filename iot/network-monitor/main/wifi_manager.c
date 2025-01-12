#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WIFI_MGR";

static bool s_is_connected = false;
static bool s_is_connecting = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
        s_is_connecting = false;
        s_is_connected = true;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED");
        s_is_connecting = false;
        s_is_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
    }
}

void wifi_manager_init(void)
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
    if (s_is_connecting || s_is_connected) {
        ESP_LOGW(TAG, "Already connecting or connected");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    s_is_connecting = true;
    s_is_connected = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Connecting to SSID:%s PASS:%s", ssid, password);
    } else {
        s_is_connecting = false;
        ESP_LOGE(TAG, "connect failed (0x%x)", err);
    }
    return err;
}

esp_err_t wifi_manager_disconnect(void)
{
    if (!s_is_connected) {
        ESP_LOGW(TAG, "Not connected, can't disconnect");
        return ESP_FAIL;
    }
    s_is_connecting = false;
    s_is_connected = false;

    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "disconnect failed (0x%x)", err);
    } else {
        ESP_LOGI(TAG, "Disconnected from AP");
    }
    return err;
}

bool wifi_manager_is_connected(void)
{
    return s_is_connected;
}

bool wifi_manager_is_connecting(void)
{
    return s_is_connecting;
}

esp_err_t wifi_manager_scan(wifi_ap_info_t **ap_list_out, int *ap_count_out)
{
    if (!ap_list_out || !ap_count_out) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_scan_config_t scanConf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE
    };

    ESP_LOGI(TAG, "Starting scan (blocking)...");
    esp_err_t err = esp_wifi_scan_start(&scanConf, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan_start failed (0x%x)", err);
        return err;
    }

    uint16_t num = 0;
    esp_wifi_scan_get_ap_num(&num);
    if (num == 0) {
        *ap_count_out = 0;
        *ap_list_out = NULL;
        ESP_LOGW(TAG, "No AP found");
        return ESP_OK;
    }

    wifi_ap_record_t *records = malloc(sizeof(wifi_ap_record_t) * num);
    if (!records) {
        return ESP_ERR_NO_MEM;
    }
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&num, records));

    wifi_ap_info_t *ap_list = malloc(sizeof(wifi_ap_info_t) * num);
    if (!ap_list) {
        free(records);
        return ESP_ERR_NO_MEM;
    }

    for (int i=0; i<num; i++) {
        strncpy(ap_list[i].ssid, (char *)records[i].ssid, 32);
        ap_list[i].ssid[32] = '\0';
        ap_list[i].rssi = records[i].rssi;
    }
    free(records);

    *ap_count_out = num;
    *ap_list_out = ap_list;
    ESP_LOGI(TAG, "Scan done, found %d", num);
    return ESP_OK;
}
