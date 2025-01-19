package api

import (
	"github.com/gin-gonic/gin"
)

func StartServer(port string) error {
	r := gin.Default()
	r.GET("/status", statusHandler)
	r.GET("/metrics", metricsHandler)
	return r.Run(":" + port)
}
