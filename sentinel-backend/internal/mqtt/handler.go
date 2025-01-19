package mqtt

import (
	"context"
	"encoding/json"
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/influxdb"
	"github.com/deniz2412/sentinel-iot-network-monitor/pkg/models"
	amqp "github.com/rabbitmq/amqp091-go"
	"log"
)

var influxClient *influxdb.Client

func SetInfluxClient(client *influxdb.Client) {
	influxClient = client
}

func HandleMessages(queue string, msgs <-chan amqp.Delivery) {
	for msg := range msgs {
		log.Printf("Message received from queue %s", queue)
		processMessage(msg.RoutingKey, msg.Body)
	}
}

func processMessage(RoutingKey string, body []byte) {
	ctx := context.Background()

	switch RoutingKey {
	case "sentinel.packets":
		var packetList models.PacketList
		if err := json.Unmarshal(body, &packetList); err != nil {
			log.Printf("Invalid packet data: %v\nMessage: %s", err, string(body))
			return
		}
		log.Printf("Processed %d packets", len(packetList.Packets))
		for _, packet := range packetList.Packets {
			tags := map[string]string{"mac": packet.MAC}
			fields := map[string]interface{}{
				"rssi":   packet.RSSI,
				"length": packet.Length,
			}
			if err := influxClient.WriteData(ctx, "packets", tags, fields, packet.Timestamp); err != nil {
				log.Printf("Error writing data to influx: %v", err)
			}
		}
	case "sentinel.devices":
		var device models.Device
		if err := json.Unmarshal(body, &device); err != nil {
			log.Printf("Invalid device data: %v", err)
			return
		}
		tags := map[string]string{"mac": device.MAC}
		fields := map[string]interface{}{
			"rssi": device.RSSI,
		}
		if err := influxClient.WriteData(ctx, "devices", tags, fields, device.LastSeen); err != nil {
			log.Printf("Error writing data to influx: %v", err)
		}
	case "sentinel.access-points":
		var ap models.AccessPoint
		if err := json.Unmarshal(body, &ap); err != nil {
			log.Printf("Invalid access point data: %v", err)
			return
		}
		tags := map[string]string{"mac": ap.MAC, "ssid": ap.SSID}
		fields := map[string]interface{}{
			"rssi":     ap.RSSI,
			"authMode": ap.AuthMode,
			"channel":  ap.Channel,
		}
		if err := influxClient.WriteData(ctx, "access-points", tags, fields, ap.Timestamp); err != nil {
			log.Printf("Error writing data to influx: %v", err)
		}
	default:
		log.Printf("Unknown queue: %s", RoutingKey)
	}
}
