package v1

import (
	"github.com/gin-gonic/gin"
)

func Init(router *gin.RouterGroup) {
	router.DELETE("/files/:file", deleteFile)

	router.GET("/files", listFiles)
	router.GET("/files/:file", getFile)

	router.POST("/files", createFile)
	router.POST("/files/:file/user:share", shareFile)

	// User management
	router.POST("/users", createUser)
}
