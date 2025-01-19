package main

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/api"
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/config"
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/influxdb"
	rabbitmq "github.com/deniz2412/sentinel-iot-network-monitor/internal/mqtt"
	"log"
)

func main() {
	// Load configuration
	cfg := config.LoadConfig()

	influxClient := influxdb.NewClient(cfg.InfluxDB)
	defer influxClient.Close()
	rabbitmq.SetInfluxClient(influxClient)
	// Start RabbitMQ consumer
	go rabbitmq.StartConsumer(cfg)

	// Start HTTP server
	log.Printf("Starting HTTP server on port %s", cfg.HTTPPort)
	if err := api.StartServer(cfg.HTTPPort); err != nil {
		log.Fatalf("Failed to start HTTP server: %v", err)
	}
}
