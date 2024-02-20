package v1

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/rs/xid"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/structpb"

	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
)

type CreateUserRequest struct {
	Name string `json:"name" validate:"required"`
}

func (req *CreateUserRequest) Validate() error {
	validate := getValidator()
	if err := validate.Struct(req); err != nil {
		return err
	}

	return nil
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
	ctx := c.Request.Context()
	// Read the request body
	var request CreateUserRequest
	if err := c.ShouldBindJSON(&request); err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	if err := request.Validate(); err != nil {
		c.JSON(http.StatusBadRequest, err.Error())
		return
	}

	// Map request
	principalId := xid.New().String()
	attrs, err := structpb.NewStruct(map[string]interface{}{
		"name": request.Name,
	})
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
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

	principal, err := principalClient.Create(ctx, &principalsCreateRequest)
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

func deleteUser(c *gin.Context) {
	ctx := c.Request.Context()
	userId := c.Param("user")

	// Delete principal
	principalsClient, err := getPrincipalsClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	principalDeleteRequest := sentium.PrincipalsDeleteRequest{
		Id: userId,
	}

	if _, err = principalsClient.Delete(ctx, &principalDeleteRequest); err != nil {
		if stts, ok := status.FromError(err); ok {
			if stts.Code() == codes.NotFound {
				c.Status((http.StatusNotFound))
				return
			}
		}

		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	c.Status(http.StatusNoContent)
}

func getUser(c *gin.Context) {
	ctx := c.Request.Context()
	userId := c.Param("user")

	// Map request
	principalRetriveReq := &sentium.PrincipalsRetrieveRequest{
		Id: userId,
	}

	// Get principal
	principalsClient, err := getPrincipalsClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	principalRetriveResp, err := principalsClient.Retrieve(ctx, principalRetriveReq)
	if err != nil {
		if stts, ok := status.FromError(err); ok {
			if stts.Code() == codes.NotFound {
				c.Status((http.StatusNotFound))
				return
			}
		}

		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	// Map response
	user := &User{
		Id:      principalRetriveResp.Id,
		Segment: *principalRetriveResp.Segment,
	}

	attrs := principalRetriveResp.GetAttrs()
	if attrs != nil && attrs.Fields["name"] != nil {
		user.Name = attrs.Fields["name"].GetStringValue()
	}

	c.JSON(http.StatusOK, user)
}

func listUsers(c *gin.Context) {
	ctx := c.Request.Context()
	// Map request
	paginationLimit, paginationToken := getPaginationParams(c)
	principalsListReq := sentium.PrincipalsListRequest{
		PaginationLimit: paginationLimit,
		PaginationToken: paginationToken,
	}

	if segment, ok := c.GetQuery("segment"); ok {
		principalsListReq.Segment = &segment
	}

	// List principals
	principalClient, err := getPrincipalsClient()
	if err != nil {
		c.JSON(http.StatusInternalServerError, err.Error())
		return
	}

	principalsList, err := principalClient.List(ctx, &principalsListReq)
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

		user := User{
			Id:   principal.Id,
			Name: attrs.Fields["name"].GetStringValue(),
		}

		if principal.Segment != nil {
			user.Segment = *principal.Segment
		}

		resp.Users = append(resp.Users, user)
	}

	c.JSON(http.StatusOK, resp)
}
