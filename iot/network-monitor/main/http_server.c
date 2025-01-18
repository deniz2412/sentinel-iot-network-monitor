// http_server.c
#include "esp_http_server.h"
#include "esp_log.h"
#include "http_server.h"
#include "endpoints.h"

static const char *TAG = "HTTP_SERVER";
static httpd_handle_t s_server = NULL;

void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80; // or any custom port

    if (httpd_start(&s_server, &config) == ESP_OK) {
        // Register endpoints
        // AP scanning
    /*

        httpd_uri_t get_aps_uri = {
            .uri       = "/access-points",
            .method    = HTTP_GET,
            .handler   = get_access_points_handler
        };
        httpd_register_uri_handler(s_server, &get_aps_uri);
    */
        // Monitoring start/stop
        httpd_uri_t start_uri = {
            .uri       = "/start-monitoring",
            .method    = HTTP_POST,
            .handler   = post_start_monitoring_handler
        };
        httpd_register_uri_handler(s_server, &start_uri);

        httpd_uri_t stop_uri = {
            .uri       = "/stop-monitoring",
            .method    = HTTP_POST,
            .handler   = post_stop_monitoring_handler
        };
        httpd_register_uri_handler(s_server, &stop_uri);
        /*
        httpd_uri_t clear_uri = {
            .uri       = "/clear-monitoring",
            .method    = HTTP_POST,
            .handler   = post_clear_monitoring_handler
        };
        httpd_register_uri_handler(s_server, &clear_uri);

        // GET /devices
        httpd_uri_t get_devices_uri = {
            .uri       = "/devices",
            .method    = HTTP_GET,
            .handler   = get_devices_handler
        };
        httpd_register_uri_handler(s_server, &get_devices_uri);

        // GET /packets
        httpd_uri_t get_packets_uri = {
            .uri       = "/packets",
            .method    = HTTP_GET,
            .handler   = get_packets_handler
        };
        httpd_register_uri_handler(s_server, &get_packets_uri);
        */
        ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
