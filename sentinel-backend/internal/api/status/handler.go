package status

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/logger"
	"github.com/gin-gonic/gin"
)

func Handler(c *gin.Context) {
	status := map[string]string{
		"status": "ok",
	}
	
	logger.Logger.Info("Status Requested")

	c.JSON(200, status)
}
