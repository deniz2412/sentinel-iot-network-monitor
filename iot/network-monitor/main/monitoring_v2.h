#ifndef MONITORING_H
#define MONITORING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"
#include "freertos/queue.h"

// Configuration constants
#define MAX_DEVICES 50
#define MAX_ACCESS_POINTS 100
#define MAX_OBSERVED_DEVICES 100
#define PACKET_QUEUE_LENGTH 800
#define MAX_BATCH_SIZE 200

extern QueueHandle_t packet_queue;

typedef struct {
    uint8_t mac[6];
    int8_t rssi;
    time_t lastSeen;
} observed_device_t;

typedef struct {
    uint8_t mac[6];
    int8_t rssi;
    uint16_t length;
    time_t timestamp;
} packet_info_t;

typedef struct {
    char ssid[32];
    int8_t rssi;
    uint8_t mac[6];
    int channel;
    char authMode[16];
    time_t timestamp;
} access_point_info_t;

// Monitoring control functions
esp_err_t monitoring_start(void);
esp_err_t monitoring_stop(void);
void monitoring_clear(void);

// Functions to start tasks (internal use, declared here for clarity)
void start_access_point_scan_task(void *param);
void start_device_scan_task(void *param);
void start_packet_batch_task(void *param);

#endif // MONITORING_H
