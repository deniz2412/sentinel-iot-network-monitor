#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"

#include <time.h>
#include <sys/time.h>

#include "wifi_manager.h"
#include "http_server.h"

// Change these to your default network
#define DEFAULT_WIFI_SSID     "Wokwi-GUEST"
#define DEFAULT_WIFI_PASSWORD ""

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

    ESP_LOGI(TAG, "Initializing Wi-Fi manager...");
    wifi_manager_init();

    // Connect to default AP
    ESP_LOGI(TAG, "Connecting to default Wi-Fi: SSID=%s PASS=%s", 
        DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
    esp_err_t conn_err = wifi_manager_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
    if (conn_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect with default creds, err=0x%x", conn_err);
        // Not fatal
    }

    // If we want to ensure IP is obtained before setting time, do it in Wi-Fi event
    // or you can do a short delay here, or wait for ip_event. For demonstration:
    vTaskDelay(pdMS_TO_TICKS(8000)); // wait a bit to connect
    init_sntp(); // fetch real UTC time

    // Start HTTP server
    ESP_LOGI(TAG, "Starting HTTP server...");
    start_http_server();

    // Idle loop
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
