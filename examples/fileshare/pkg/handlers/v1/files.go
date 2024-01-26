package v1

import (
	"context"
	"net/http"
	"strconv"

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

type File struct {
	Id    string `json:"id"`
	Name  string `json:"name"`
	Owner string `json:"owner"`
	Type  string `json:"type"`
}

type ListFilesResponse struct {
	Files           []File `json:"files"`
	PaginationToken string `json:"pagination_token"`
}

type ShareFileRequest struct {
	UserId string `json:"id"`
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

	authzGrantRequest := sentium_grpc.AuthzGrantRequest{
		PrincipalId:  c.GetHeader("user-id"),
		ResourceId:   resourceId,
		ResourceType: "files",
		Attrs:        attrs,
	}

	// Grant access to file
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
	resp := File{
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

func getResourcesClient() (sentium_grpc.ResourcesClient, error) {
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	return sentium_grpc.NewResourcesClient(conn), nil
}

func deleteFile(c *gin.Context) {}

func getFile(c *gin.Context) {}

func listFiles(c *gin.Context) {
	// Map request
	resourcesListReq := sentium_grpc.ResourcesListRequest{
		PrincipalId:  c.GetHeader("user-id"),
		ResourceType: "files",
	}

	if limit, ok := c.GetQuery("pagination_limit"); ok {
		l64, _ := strconv.ParseUint(limit, 10, 32)
		l32 := uint32(l64)
		resourcesListReq.PaginationLimit = &l32
	}

	if token, ok := c.GetQuery("pagination_token"); ok {
		resourcesListReq.PaginationToken = &token
	}

	resourcesClient, err := getResourcesClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	resourcesListResp, err := resourcesClient.List(context.Background(), &resourcesListReq)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map response
	resp := ListFilesResponse{
		Files: []File{},
	}

	if resourcesListResp.PaginationToken != nil {
		resp.PaginationToken = *resourcesListResp.PaginationToken
	}

	for _, resource := range resourcesListResp.Resources {
		attrs := resource.GetAttrs()
		if attrs == nil {
			continue
		}

		file := File{
			Id:   resource.GetId(),
			Name: attrs.Fields["name"].String(),
			Type: resource.GetType(),
		}

		if attrs.Fields["name"] != nil {
			file.Name = attrs.Fields["name"].GetStringValue()
		}

		if attrs.Fields["owner"] != nil {
			file.Owner = attrs.Fields["owner"].GetStringValue()
		}

		resp.Files = append(resp.Files, file)
	}

	c.JSON(http.StatusOK, resp)
}

func shareFile(c *gin.Context) {
	var request ShareFileRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	resourceId := c.Param("file")

	// Check requestor has access to shared resource
	authzClient, err := getAuthzClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	authzCheckRequest := sentium_grpc.AuthzCheckRequest{
		PrincipalId:  c.GetHeader("user-id"),
		ResourceId:   resourceId,
		ResourceType: "files",
	}

	authzCheckResponse, err := authzClient.Check(context.Background(), &authzCheckRequest)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if !authzCheckResponse.GetOk() {
		c.JSON(http.StatusForbidden, nil)
		return
	}

	// Share resource
	authzGrantRequest := sentium_grpc.AuthzGrantRequest{
		PrincipalId:  request.UserId,
		ResourceId:   resourceId,
		ResourceType: "files",
	}

	if _, err := authzClient.Grant(context.Background(), &authzGrantRequest); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	c.JSON(http.StatusNoContent, nil)
}
