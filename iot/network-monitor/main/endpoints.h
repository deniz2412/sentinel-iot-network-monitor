#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

// GET /access-points
esp_err_t get_access_points_handler(httpd_req_t *req);

// POST /start-monitoring
esp_err_t post_start_monitoring_handler(httpd_req_t *req);

// POST /stop-monitoring
esp_err_t post_stop_monitoring_handler(httpd_req_t *req);

// GET /devices (list indefinite approach’s device table)
esp_err_t get_devices_handler(httpd_req_t *req);

// Clears device + packet data
esp_err_t post_clear_monitoring_handler(httpd_req_t *req);

// GET /devices
esp_err_t get_devices_handler(httpd_req_t *req);

// GET /packets
esp_err_t get_packets_handler(httpd_req_t *req);


#ifdef __cplusplus
}
#endif
