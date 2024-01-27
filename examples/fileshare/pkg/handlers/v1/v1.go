package v1

import (
	"github.com/gin-gonic/gin"
	"github.com/go-playground/validator/v10"
)

var validate *validator.Validate

func Init(router *gin.RouterGroup) {
	router.DELETE("/files/:file", deleteFile)

	router.GET("/files", listFiles)
	router.GET("/files/:file", getFile)

	router.POST("/files", createFile)
	router.POST("/files/:file/user:share", shareFile)

	// User management
	router.GET("/users", listUsers)

	router.POST("/users", createUser)
}

func getValidator() *validator.Validate {
	if validate != nil {
		return validate
	}

	validate := validator.New()
	return validate
}
