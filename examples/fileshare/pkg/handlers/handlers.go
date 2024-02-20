package handlers

import (
	"net/http"

	"github.com/gin-gonic/gin"

	v1 "github.com/sentium/examples/fileshare/pkg/handlers/v1"
)

func Init(router *gin.RouterGroup) {
	router.GET("/", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"version": "1.0.0",
		})
	})

	router.GET("/healthz", healthz)

	v1Router := router.Group("/v1")
	{
		v1.Init(v1Router)
	}
}

func healthz(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{})
}
