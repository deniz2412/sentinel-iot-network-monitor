#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void mqtt_init(const char *uri, const char *user, const char *password);
void mqtt_publish(const char *topic_suffix, const char *message);

#endif // MQTT_MANAGER_H
