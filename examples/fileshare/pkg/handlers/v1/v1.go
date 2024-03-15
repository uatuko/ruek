package v1

import (
	"github.com/gin-gonic/gin"
	"github.com/go-playground/validator/v10"
)

var validate *validator.Validate

func Init(router *gin.RouterGroup) {
	router.DELETE("/files/:file", deleteFile)
	router.DELETE("/files/:file/users/:user", unshareFile)

	router.GET("/files", listFiles)
	router.GET("/files/:file", getFile)
	router.GET("/files/:file/users", listFileUsers)

	router.POST("/files", createFile)
	router.POST("/files/:file/users", shareFile)

	// User management
	router.DELETE("/users/:user", deleteUser)

	router.GET("/users", listUsers)
	router.GET("/users/:user", getUser)

	router.POST("/users", createUser)
}

func getValidator() *validator.Validate {
	if validate != nil {
		return validate
	}

	validate := validator.New()
	return validate
}
