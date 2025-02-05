#ifndef MONITORING_H
#define MONITORING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

// Maximum number of devices and packets to track
#define MAX_DEVICES 50
#define MAX_PACKETS 200
#define MAX_ACCESS_POINTS 100

// Structure to hold device information
typedef struct {
    uint8_t mac[6];
    int8_t rssi;
    uint32_t packetCount;
    time_t firstSeen;
    time_t lastSeen;
} device_info_t;

// Structure to hold packet information
typedef struct {
    uint8_t mac[6];
    int8_t rssi;
    uint16_t length;
    time_t timestamp;
} packet_info_t;

// Structure to hold access point information
typedef struct {
    char ssid[32];
    int8_t rssi;
    uint8_t mac[6];
    int channel;
    char authMode[16];
    time_t timestamp;
} access_point_info_t;

// Start and stop monitoring functions
esp_err_t monitoring_start(void);
esp_err_t monitoring_stop(void);
void monitoring_clear(void);

// Retrieve data functions
device_info_t* monitoring_get_devices(int *count_out);
packet_info_t* monitoring_get_packets(int *count_out);
access_point_info_t* monitoring_get_access_points(int *count_out);

#endif // MONITORING_H
