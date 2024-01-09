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

type CreateUserRequest struct {
	Name string `json:"name"`
}

type User struct {
	Id   string `json:"id"`
	Name string `json:"name"`
}

func createUser(c *gin.Context) {
	// Read the request body
	var request CreateFileRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.Error(err).SetType(gin.ErrorTypePublic)
		return
	}

	// Map request
	principalId := xid.New().String()
	attrs, err := structpb.NewStruct(map[string]interface{}{
		"name": request.Name,
	})
	if err != nil {
		c.Error(err).SetType(gin.ErrorTypePublic)
		return
	}

	principalsCreateRequest := sentium_grpc.PrincipalsCreateRequest{
		Id:    &principalId,
		Attrs: attrs,
	}

	// Create principal
	principalClient, err := getPrincipalsClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	principal, err := principalClient.Create(context.Background(), &principalsCreateRequest)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map response
	resp := User{
		Id:   principal.Id,
		Name: principal.Attrs.Fields["name"].GetStringValue(),
	}

	c.JSON(http.StatusCreated, resp)
}

func getPrincipalsClient() (sentium_grpc.PrincipalsClient, error) {
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	return sentium_grpc.NewPrincipalsClient(conn), nil
}
