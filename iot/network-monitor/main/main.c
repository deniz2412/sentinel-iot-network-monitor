#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "lwip/apps/sntp.h"
#include "config_manager.h"

#include <time.h>
#include <sys/time.h>

#include "wifi_manager.h"
#include "http_server.h"
#include "mqtt_manager.h"

static const char *TAG = "MAIN";

/**
 * Initialize SNTP to get real UTC time
 */
static void init_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Optionally wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int max_retries = 10;
    while (timeinfo.tm_year < (2025 - 1900) && ++retry < max_retries) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, max_retries);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    if (timeinfo.tm_year >= (2025 - 1900)) {
        ESP_LOGI(TAG, "System time set to: %s", asctime(&timeinfo));
    } else {
        ESP_LOGW(TAG, "Time not set yet, continuing...");
    }
}

/**
 * Mounts configurations
 */
static void mount_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Failed to mount or format SPIFFS (0x%x)", ret);
    } else {
        ESP_LOGI("SPIFFS", "SPIFFS mounted successfully");
    }


}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    mount_spiffs();

    config_t config;
    memset(&config,0,sizeof(config));

    bool got_config = read_config(&config);

    ESP_LOGI(TAG, "Initializing Wi-Fi manager...");
    wifi_manager_init();

    // Connect to default AP
    ESP_LOGI(TAG, "Connecting to Wi-Fi: SSID=%s PASS=%s", 
        config.wifi_ssid, config.wifi_pass);
    esp_err_t conn_err = wifi_manager_connect(config.wifi_ssid, config.wifi_pass);
    if (conn_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect with default creds, err=0x%x", conn_err);
        // Not fatal
    }
    esp_log_level_set("*", ESP_LOG_DEBUG);

    vTaskDelay(pdMS_TO_TICKS(8000)); // wait a bit to connect
    init_sntp(); // fetch real UTC time

    // Start HTTP server
    start_http_server();

    mqtt_init(config.mqtt_uri, config.mqtt_user, config.mqtt_pass);
    // Idle loop
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
