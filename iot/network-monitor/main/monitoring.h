#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DEVICES 50
#define MAX_PACKETS 200

typedef struct {
    uint8_t mac[6];
    int8_t  rssi;
    uint32_t packetCount;
    time_t firstSeen;
    time_t lastSeen;
} device_info_t;

typedef struct {
    uint8_t mac[6];
    int8_t  rssi;
    uint16_t length;
    time_t timestamp;
} packet_info_t;

/**
 * Start indefinite promiscuous mode. Devices + packets are tracked.
 */
esp_err_t monitoring_start(void);

/**
 * Stop indefinite promiscuous mode.
 */
esp_err_t monitoring_stop(void);

/**
 * Clear both device table and packet list.
 */
void monitoring_clear(void);

/**
 * Retrieve a copy of the device table. Caller frees.
 */
device_info_t* monitoring_get_devices(int *count_out);

/**
 * Retrieve a copy of the packet list. Caller frees.
 */
packet_info_t* monitoring_get_packets(int *count_out);

#ifdef __cplusplus
}
#endif
