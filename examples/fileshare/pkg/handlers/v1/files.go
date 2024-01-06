package v1

import (
	"context"
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/rs/xid"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/protobuf/types/known/structpb"

	sentium_grpc "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
)

type CreateFileRequest struct {
	Name string `json:"name"`
	Type string `json:"type"`
}

type CreateFileResponse struct {
	Id   string `json:"id"`
	Name string `json:"name"`
	Type string `json:"type"`
}

func createFile(c *gin.Context) {
	// Read the request body
	var request CreateFileRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.Error(err).SetType(gin.ErrorTypePublic)
		return
	}

	// Map request
	resourceId := xid.New().String()
	attrs, err := structpb.NewStruct(map[string]interface{}{
		"name": request.Name,
		"role": "owner",
		"type": request.Type,
	})
	if err != nil {
		c.Error(err).SetType(gin.ErrorTypePublic)
		return
	}

	// Grant access to file
	authzGrantRequest := sentium_grpc.AuthzGrantRequest{
		PrincipalId:  c.Request.Header["Userid"][0],
		ResourceId:   resourceId,
		ResourceType: "files",
		Attrs:        attrs,
	}

	authzClient, err := getAuthzClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if _, err := authzClient.Grant(context.Background(), &authzGrantRequest); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map response
	resp := CreateFileResponse{
		Id:   resourceId,
		Name: request.Name,
		Type: request.Type,
	}

	c.JSON(http.StatusCreated, resp)
}

func getAuthzClient() (sentium_grpc.AuthzClient, error) {
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	return sentium_grpc.NewAuthzClient(conn), nil
}

func deleteFile(c *gin.Context) {}

func getFile(c *gin.Context) {}

func listFiles(c *gin.Context) {}

func shareFile(c *gin.Context) {}
