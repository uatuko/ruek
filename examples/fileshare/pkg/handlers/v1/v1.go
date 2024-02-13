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
	router.GET("/files/:file/users", listFileUsers)

	router.POST("/files", createFile)
	router.POST("/files/:file/users", shareFile)
	router.DELETE("/files/:file/users/:user", unshareFile)

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
