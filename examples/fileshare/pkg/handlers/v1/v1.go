package v1

import (
	"github.com/gin-gonic/gin"
)

func Init(router *gin.RouterGroup) {
	router.DELETE("/files/:file", deleteFile)

	router.GET("/files/:file", getFile)
	router.GET("/files", listFiles)

	router.POST("/files", createFile)
	router.POST("/files/:file/_share", shareFile)

	// User management
}
