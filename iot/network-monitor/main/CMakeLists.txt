
idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "http_server.c"
        "endpoints.c"
        "monitoring_v2.c"
        "config_manager.c"
        "mqtt_manager.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        json
        spiffs
        mqtt  
)
spiffs_create_partition_image(spiffs spiffs_data FLASH_IN_PROJECT)


