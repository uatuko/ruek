package v1

import (
	"context"
	"net/http"
	"strconv"

	"github.com/gin-gonic/gin"
	"github.com/rs/xid"
	"google.golang.org/protobuf/types/known/structpb"

	sentium_grpc "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
)

type CreateFileRequest struct {
	Name string `json:"name" validate:"required"`
}

func (req *CreateFileRequest) Validate() error {
	validate := getValidator()
	if err := validate.Struct(req); err != nil {
		return err
	}

	return nil
}

type File struct {
	Id   string `json:"id"`
	Name string `json:"name"`
	Role string `json:"role"`
}

type ListFilesResponse struct {
	Files           []File `json:"files"`
	PaginationToken string `json:"pagination_token"`
}

type ShareFileRequest struct {
	Role   string `json:"role"`
	UserId string `json:"id"`
}

func createFile(c *gin.Context) {
	// Read the request body
	var request CreateFileRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if err := request.Validate(); err != nil {
		c.JSON(http.StatusBadRequest, err.Error())
		return
	}

	// Map request
	resourceId := xid.New().String()
	attrs, err := structpb.NewStruct(map[string]interface{}{
		"name": request.Name,
		"role": "owner",
	})
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
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
		Role: "owner",
	}

	c.JSON(http.StatusCreated, resp)
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
		if attrs == nil || attrs.Fields["name"] == nil || attrs.Fields["role"] == nil {
			continue
		}

		resp.Files = append(resp.Files, File{
			Id:   resource.GetId(),
			Name: attrs.Fields["name"].GetStringValue(),
			Role: attrs.Fields["role"].GetStringValue(),
		})
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

	role := authzCheckResponse.Attrs.Fields["role"].GetStringValue()
	if !canShare(role) {
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

func canShare(role string) bool {
	if role != "owner" && role != "editor" {
		return false
	}

	return true
}
