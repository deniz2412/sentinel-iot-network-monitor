package api

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/api/data"
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/api/status"
	"github.com/gin-gonic/gin"
)

func SetupRoutes() *gin.Engine {
	router := gin.Default()

	router.GET("/status", status.Handler)
	router.GET("/api/v1/data", data.Handler)
	return router
}
