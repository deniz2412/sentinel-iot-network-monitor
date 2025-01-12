idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "http_server.c"
        "endpoints.c"
        "monitoring.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        json         # For JSON parsing if needed
)
