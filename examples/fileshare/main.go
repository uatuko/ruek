package main

import (
	"github.com/gin-contrib/cors"
	"github.com/gin-contrib/logger"
	"github.com/gin-gonic/gin"

	"github.com/sentium/examples/fileshare/pkg/handlers"
)

func main() {
	engine := gin.New()
	engine.Use(logger.SetLogger()) // logger middleware
	engine.Use(gin.Recovery())     // recovery middleware

	// cors
	config := cors.DefaultConfig()
	config.AllowAllOrigins = true
	config.AllowHeaders = append(config.AllowHeaders, "user-id")
	engine.Use(cors.New(config))

	engine.NoRoute(func(c *gin.Context) {
		c.JSON(404, gin.H{
			"error": gin.H{
				"message": "Not found",
			},
		})
	})

	router := engine.Group("/")
	handlers.Init(router)

	// Serve HTTP in a goroutine
	engine.Run("localhost:3000")
}
