#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// A structure for your configuration
typedef struct {
    // Wi-Fi
    char wifi_ssid[64];
    char wifi_pass[64];

    // MQTT
    char mqtt_uri[128];
    char mqtt_user[64];
    char mqtt_pass[64];

    // Potential future fields
    // ...
} config_t;

/**
 * @brief Reads /spiffs/config.json and fills out *cfg with Wi-Fi and MQTT info
 * @param cfg  Pointer to your config_t
 * @return true if success, false if an error occurred
 */
bool read_config(config_t *cfg);

#ifdef __cplusplus
}
#endif
