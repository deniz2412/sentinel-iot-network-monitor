package api

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

func statusHandler(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{"status": "OK"})
}

func metricsHandler(c *gin.Context) {
	// Placeholder for metrics logic
	c.JSON(http.StatusOK, gin.H{"metrics": "Not implemented yet"})
}
