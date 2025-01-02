package server

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/api"
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/logger"
	"go.uber.org/zap"
	"os"
)

func StartServer() {
	router := api.SetupRoutes()

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

	err := router.Run(":" + port)
	if err != nil {
		logger.Logger.Fatal("Failed to start server", zap.Error(err))
	}

}
