#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes Wi-Fi subsystem in Station mode
 */
void wifi_manager_init(void);

/**
 * @brief Connect to the given AP
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect if currently connected
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Check if device is connected
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Check if device is in connecting process
 */
bool wifi_manager_is_connecting(void);

/**
 * @brief Synchronous Wi-Fi scan to find APs
 *        @param ap_list_out pointer to pointer for results (allocated)
 *        @param ap_count_out pointer to int that receives number of APs
 */
typedef struct {
    char ssid[33];
    int rssi;
} wifi_ap_info_t;

esp_err_t wifi_manager_scan(wifi_ap_info_t **ap_list_out, int *ap_count_out);

#ifdef __cplusplus
}
#endif
