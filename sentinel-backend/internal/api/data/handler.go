package data

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/logger"
	"github.com/gin-gonic/gin"
	"net/http"
)

func Handler(c *gin.Context) {
	data := map[string]interface{}{
		"device_count":    42,
		"signal_strength": "High",
	}
	logger.Logger.Info("Data Requested")
	c.JSON(http.StatusOK, data)

}
