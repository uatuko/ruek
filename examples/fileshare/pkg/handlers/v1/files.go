package v1

import (
	"context"
	"net/http"

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
	Id   string `json:"id" validate:"required"`
	Name string `json:"name" validate:"required"`
	Role string `json:"role" validate:"required,oneof=editor owner viewer"`
}

func (f *File) Validate() error {
	validate := getValidator()
	if err := validate.Struct(f); err != nil {
		return err
	}

	return nil
}

type ListFilesResponse struct {
	Files           []File `json:"files"`
	PaginationToken string `json:"pagination_token"`
}

type ShareFileRequest struct {
	Role   string `json:"role" validate:"required,oneof=editor owner viewer"`
	UserId string `json:"id" validate:"required"`
}

func (req *ShareFileRequest) Validate() error {
	validate := getValidator()
	if err := validate.Struct(req); err != nil {
		return err
	}

	return nil
}

type FileUser struct {
	Id      string `json:"id"`
	Name    string `json:"name"`
	Role    string `json:"role"`
	Segment string `json:"segment"`
}

type ListFileUsersResponse struct {
	PaginationToken string     `json:"pagination_token"`
	Users           []FileUser `json:"users"`
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

func getFile(c *gin.Context) {
	resourceId := c.Param("file")
	userId := c.GetHeader("user-id")

	// Check requestor has access to file
	authzClient, err := getAuthzClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	authzCheckRequest := sentium_grpc.AuthzCheckRequest{
		PrincipalId:  userId,
		ResourceId:   resourceId,
		ResourceType: "files",
	}

	authzCheckResponse, err := authzClient.Check(context.Background(), &authzCheckRequest)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if !authzCheckResponse.GetOk() {
		c.JSON(http.StatusForbidden, "no access to file")
		return
	}

	// Get the file
	resourcesListReq := sentium_grpc.ResourcesListRequest{
		PrincipalId:  c.GetHeader("user-id"),
		ResourceType: "files",
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
	var file *File
	for _, resource := range resourcesListResp.Resources {
		if resource.Id != resourceId {
			continue
		}

		attrs := resource.GetAttrs()
		if attrs == nil || attrs.Fields["name"] == nil || attrs.Fields["role"] == nil {
			continue
		}

		file = &File{
			Id:   resource.GetId(),
			Name: attrs.Fields["name"].GetStringValue(),
			Role: attrs.Fields["role"].GetStringValue(),
		}

		break
	}

	if file == nil {
		c.JSON(http.StatusNotFound, "not found")
	}

	c.JSON(http.StatusOK, file)
}

func listFiles(c *gin.Context) {
	// Map request
	paginationLimit, paginationToken := getPaginationParams(c)
	resourcesListReq := sentium_grpc.ResourcesListRequest{
		PaginationLimit: paginationLimit,
		PaginationToken: paginationToken,
		PrincipalId:     c.GetHeader("user-id"),
		ResourceType:    "files",
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

func listFileUsers(c *gin.Context) {
	fileId := c.Param("file")
	userId := c.GetHeader("user-id")
	paginationLimit, paginationToken := getPaginationParams(c)

	// Check requestor has access to file
	role, err := getRole(userId, fileId)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if role == "" {
		c.JSON(http.StatusForbidden, "no access to file")
		return
	}

	// Map request
	listPrinipalsReq := sentium_grpc.ResourcesListPrincipalsRequest{
		ResourceId:      fileId,
		ResourceType:    "files",
		PaginationLimit: paginationLimit,
		PaginationToken: paginationToken,
	}

	// List Principals
	resourcesClient, err := getResourcesClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	listPrincipalsResp, err := resourcesClient.ListPrincipals(context.Background(), &listPrinipalsReq)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map response
	resp := ListFileUsersResponse{
		Users: []FileUser{},
	}

	if listPrincipalsResp.PaginationToken != nil {
		resp.PaginationToken = *listPrincipalsResp.PaginationToken
	}

	for _, principal := range listPrincipalsResp.Principals {
		attrs := principal.GetAttrs()
		if attrs == nil || attrs.Fields["name"] == nil || attrs.Fields["role"] == nil {
			continue
		}

		user, err := getUser(principal.Id)
		if err != nil {
			c.JSON(http.StatusInternalServerError, err.Error())
			return
		}

		resp.Users = append(resp.Users, FileUser{
			Id:      principal.Id,
			Name:    user.Name,
			Role:    attrs.Fields["role"].GetStringValue(),
			Segment: user.Segment,
		})
	}

	c.JSON(http.StatusOK, resp)
}

func shareFile(c *gin.Context) {
	resourceId := c.Param("file")
	userId := c.GetHeader("user-id")
	var request ShareFileRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if err := request.Validate(); err != nil {
		c.JSON(http.StatusBadRequest, err.Error())
		return
	}

	// Check requestor has access to shared resource
	role, err := getRole(userId, resourceId)
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if role == "" {
		c.JSON(http.StatusForbidden, "no access to file")
		return
	}

	// Check requestor can share file
	if err := canShare(role, request.Role); err != nil {
		c.JSON(http.StatusForbidden, err.Error())
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
