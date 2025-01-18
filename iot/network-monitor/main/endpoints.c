#include "endpoints.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "monitoring_v2.h"

/** 
esp_err_t get_access_points_handler(httpd_req_t *req)
{
    wifi_ap_info_t *ap_list = NULL;
    int ap_count = 0;
    esp_err_t err = wifi_manager_scan(&ap_list, &ap_count);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Scan failed");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_CreateArray();
    for (int i=0; i<ap_count; i++) {
        cJSON *apObj = cJSON_CreateObject();
        cJSON_AddStringToObject(apObj, "ssid", ap_list[i].ssid);
        cJSON_AddNumberToObject(apObj, "rssi", ap_list[i].rssi);
        cJSON_AddItemToArray(root, apObj);
    }
    char *jsonStr = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, jsonStr);
    free(jsonStr);
    free(ap_list);
    return ESP_OK;
}
*/
esp_err_t post_start_monitoring_handler(httpd_req_t *req)
{
    esp_err_t err = monitoring_start();
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "{\"message\":\"Monitoring started.\"}");
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Already active or error");
    }
    return ESP_OK;
}

esp_err_t post_stop_monitoring_handler(httpd_req_t *req)
{
    esp_err_t err = monitoring_stop();
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "{\"message\":\"Monitoring stopped.\"}");
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not active or error");
    }
    return ESP_OK;
}
/** 
esp_err_t post_clear_monitoring_handler(httpd_req_t *req)
{
    monitoring_clear();
    httpd_resp_sendstr(req, "{\"message\":\"Monitoring data cleared.\"}");
    return ESP_OK;
}

// Return indefinite device table
esp_err_t get_devices_handler(httpd_req_t *req)
{
    int count = 0;
    device_info_t *dev_list = monitoring_get_devices(&count);
    if (!dev_list || count == 0) {
        httpd_resp_sendstr(req, "[]");
        return ESP_OK;
    }

    cJSON *root = cJSON_CreateArray();
    for (int i=0; i<count; i++) {
        cJSON *obj = cJSON_CreateObject();

        // MAC
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 dev_list[i].mac[0], dev_list[i].mac[1], dev_list[i].mac[2],
                 dev_list[i].mac[3], dev_list[i].mac[4], dev_list[i].mac[5]);
        cJSON_AddStringToObject(obj, "mac", mac_str);

        cJSON_AddNumberToObject(obj, "rssi", dev_list[i].rssi);
        cJSON_AddNumberToObject(obj, "packetCount", dev_list[i].packetCount);

        // Timestamps in seconds
        cJSON_AddNumberToObject(obj, "firstSeen", dev_list[i].firstSeen);
        cJSON_AddNumberToObject(obj, "lastSeen", dev_list[i].lastSeen);

        // If you want a human string:
        // struct tm tm_info;
        // char timeBuf[64];
        // localtime_r(&dev_list[i].firstSeen, &tm_info);
        // strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm_info);
        // cJSON_AddStringToObject(obj, "firstSeenStr", timeBuf);

        cJSON_AddItemToArray(root, obj);
    }

    char *jsonStr = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, jsonStr);
    free(jsonStr);
    free(dev_list);
    return ESP_OK;
}

// Return indefinite packet list
esp_err_t get_packets_handler(httpd_req_t *req)
{
    int count = 0;
    packet_info_t *pkt_list = monitoring_get_packets(&count);
    if (!pkt_list || count == 0) {
        httpd_resp_sendstr(req, "[]");
        return ESP_OK;
    }

    cJSON *root = cJSON_CreateArray();
    for (int i=0; i<count; i++) {
        cJSON *obj = cJSON_CreateObject();

        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 pkt_list[i].mac[0], pkt_list[i].mac[1], pkt_list[i].mac[2],
                 pkt_list[i].mac[3], pkt_list[i].mac[4], pkt_list[i].mac[5]);
        cJSON_AddStringToObject(obj, "mac", mac_str);
        cJSON_AddNumberToObject(obj, "rssi", pkt_list[i].rssi);
        cJSON_AddNumberToObject(obj, "length", pkt_list[i].length);
        cJSON_AddNumberToObject(obj, "timestamp", pkt_list[i].timestamp);

        cJSON_AddItemToArray(root, obj);
    }

    char *jsonStr = cJSON_Print(root);
    cJSON_Delete(root);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, jsonStr);
    free(jsonStr);
    free(pkt_list);
    return ESP_OK;
}
*/