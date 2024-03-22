package main

import (
	"context"
	"log"
	"os"
	"os/signal"

	"github.com/gin-contrib/cors"
	"github.com/gin-contrib/logger"
	"github.com/gin-gonic/gin"

	"github.com/sentium/examples/fileshare/pkg/handlers"
)

func main() {
	if err := run(); err != nil {
		log.Fatalln(err)
	}
}

func run() (err error) {
	// Main running context during app lifecycle
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	// Setup server
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
	srvErr := make(chan error, 1)
	go func() {
		srvErr <- engine.Run("localhost:3000")
	}()

	// Handle channels
	select {
	case err = <-srvErr:
		// Error from server
		return
	case <-ctx.Done():
		// Wait for first CTRL+C
		// Stop receiving signal notifications as soon as possible.
		stop()
		log.Println("shutting down gracefully, press Ctrl+C again to force")
	}

	return
}
