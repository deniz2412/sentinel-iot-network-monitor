#include "config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "CONFIG_MGR";

bool read_config(config_t *cfg)
{
    if (!cfg) {
        ESP_LOGE(TAG, "Null cfg pointer");
        return false;
    }

    // Open the file
    FILE *f = fopen("/spiffs/config.json", "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open /spiffs/config.json");
        return false;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Read file into a buffer
    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        ESP_LOGE(TAG, "Malloc fail reading config file");
        return false;
    }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error in config");
        return false;
    }

    // Start retrieving fields
    // 1) Wi-Fi
    cJSON *wifi_obj = cJSON_GetObjectItem(root, "wifi");
    if (wifi_obj) {
        cJSON *jSsid = cJSON_GetObjectItem(wifi_obj, "ssid");
        cJSON *jPass = cJSON_GetObjectItem(wifi_obj, "password");
        if (jSsid && cJSON_IsString(jSsid)) {
            strncpy(cfg->wifi_ssid, jSsid->valuestring, sizeof(cfg->wifi_ssid) - 1);
            cfg->wifi_ssid[sizeof(cfg->wifi_ssid) - 1] = '\0';
        }
        if (jPass && cJSON_IsString(jPass)) {
            strncpy(cfg->wifi_pass, jPass->valuestring, sizeof(cfg->wifi_pass) - 1);
            cfg->wifi_pass[sizeof(cfg->wifi_pass) - 1] = '\0';
        }
    } else {
        ESP_LOGW(TAG, "No 'wifi' object in config");
    }

    // 2) MQTT
    cJSON *mqtt_obj = cJSON_GetObjectItem(root, "mqtt");
    if (mqtt_obj) {
        cJSON *jUri = cJSON_GetObjectItem(mqtt_obj, "uri");
        cJSON *jUser = cJSON_GetObjectItem(mqtt_obj, "user");
        cJSON *jPass = cJSON_GetObjectItem(mqtt_obj, "password");

        if (jUri && cJSON_IsString(jUri)) {
            strncpy(cfg->mqtt_uri, jUri->valuestring, sizeof(cfg->mqtt_uri) - 1);
            cfg->mqtt_uri[sizeof(cfg->mqtt_uri) - 1] = '\0';
        }
        if (jUser && cJSON_IsString(jUser)) {
            strncpy(cfg->mqtt_user, jUser->valuestring, sizeof(cfg->mqtt_user) - 1);
            cfg->mqtt_user[sizeof(cfg->mqtt_user) - 1] = '\0';
        }
        if (jPass && cJSON_IsString(jPass)) {
            strncpy(cfg->mqtt_pass, jPass->valuestring, sizeof(cfg->mqtt_pass) - 1);
            cfg->mqtt_pass[sizeof(cfg->mqtt_pass) - 1] = '\0';
        }
    } else {
        ESP_LOGW(TAG, "No 'mqtt' object in config");
    }

    // If you want to log results:
    ESP_LOGI(TAG, "Config loaded: WiFi SSID=%s, WiFi PASS=%s, MQTT HOST=%s, USER=%s, PASS=%s",
        cfg->wifi_ssid, cfg->wifi_pass, cfg->mqtt_uri, cfg->mqtt_user, cfg->mqtt_pass);

    cJSON_Delete(root);
    return true;
}
