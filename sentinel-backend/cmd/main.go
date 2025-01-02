package main

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/logger"
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/server"
)

func main() {
	//err := config.LoadConfig()
	//if err != nil {
	//	log.Fatalf("Error loading config: %v", err)
	//}
	//logger.Init()
	logger.InitLogger()
	server.StartServer()

}
