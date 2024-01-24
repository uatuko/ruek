package v1

import (
	"context"
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/rs/xid"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/protobuf/types/known/structpb"

	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
)

var principalsClient sentium.PrincipalsClient

type CreateUserRequest struct {
	Name string `json:"name"`
}

type ListUsersRequest struct {
	PaginationLimit uint32 `json:"pagination_limit"`
	PaginationToken string `json:"pagination_token"`
	Segment         string `json:"segment"`
}

type ListUsersResponse struct {
	PaginationToken string `json:"pagination_token"`
	Users           []User `json:"users"`
}

type User struct {
	Id      string `json:"id"`
	Name    string `json:"name"`
	Segment string `json:"segment"`
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

	principalsCreateRequest := sentium.PrincipalsCreateRequest{
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

func getPrincipalsClient() (sentium.PrincipalsClient, error) {
	if principalsClient != nil {
		return principalsClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	principalsClient = sentium.NewPrincipalsClient(conn)
	return principalsClient, nil
}

func listUsers(c *gin.Context) {
	var request ListUsersRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map request
	principalsListReq := sentium.PrincipalsListRequest{
		PaginationLimit: &request.PaginationLimit,
		PaginationToken: &request.PaginationToken,
		Segment:         &request.Segment,
	}

	// List principals
	principalClient, err := getPrincipalsClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	principalsList, err := principalClient.List(context.Background(), &principalsListReq)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map response
	resp := ListUsersResponse{
		Users: []User{},
	}

	if principalsList.PaginationToken != nil {
		resp.PaginationToken = *principalsList.PaginationToken
	}

	for _, principal := range principalsList.Principals {
		attrs := principal.GetAttrs()
		if attrs == nil || attrs.Fields["name"] == nil {
			continue
		}

		resp.Users = append(resp.Users, User{
			Id:      principal.Id,
			Name:    attrs.Fields["name"].GetStringValue(),
			Segment: *principal.Segment,
		})
	}

	c.JSON(http.StatusOK, resp)
}
