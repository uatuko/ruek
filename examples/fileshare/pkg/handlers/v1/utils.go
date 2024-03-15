package v1

import (
	"context"
	"fmt"
	"strconv"

	"github.com/gin-gonic/gin"
	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var authzClient sentium.AuthzClient
var principalsClient sentium.PrincipalsClient
var resourcesClient sentium.ResourcesClient

var shareOrder = map[string]int{
	"owner":  0,
	"editor": 1,
	"viewer": 2,
}

func canDelete(role string) error {
	if role == "viewer" {
		return fmt.Errorf("cannot delee file (role: %s)", role)
	}

	return nil
}

func canShare(sharerRole string, sharedRole string) error {
	if sharerRole != "owner" && sharerRole != "editor" {
		return fmt.Errorf("role cannot share (sharer role: %s)", sharerRole)
	}

	if shareOrder[sharerRole] > shareOrder[sharedRole] {
		return fmt.Errorf(
			"cannot share to higher role (sharer role: %s, sharee role: %s)",
			sharerRole,
			sharedRole,
		)
	}

	return nil
}

func canUnshare(requestorRole string, unsharedRole string) error {
	if shareOrder[requestorRole] > shareOrder[unsharedRole] {
		return fmt.Errorf(
			"cannot unshare from a higher role (requestor role: %s, unshared role: %s)",
			requestorRole,
			unsharedRole,
		)
	}

	return nil
}

func getAuthzClient() (sentium.AuthzClient, error) {
	if authzClient != nil {
		return authzClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:8080", opts...)
	if err != nil {
		return nil, err
	}

	authzClient = sentium.NewAuthzClient(conn)
	return authzClient, nil
}

func getPrincipalsClient() (sentium.PrincipalsClient, error) {
	if principalsClient != nil {
		return principalsClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:8080", opts...)
	if err != nil {
		return nil, err
	}

	principalsClient = sentium.NewPrincipalsClient(conn)
	return principalsClient, nil
}

func getResourcesClient() (sentium.ResourcesClient, error) {
	if resourcesClient != nil {
		return resourcesClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:8080", opts...)
	if err != nil {
		return nil, err
	}

	resourcesClient := sentium.NewResourcesClient(conn)
	return resourcesClient, nil
}

// Check the principal has access to resource
func getRole(ctx context.Context, principalId string, fileId string) (string, error) {
	authzClient, err := getAuthzClient()
	if err != nil {
		return "", err
	}

	authzCheckRequest := sentium.AuthzCheckRequest{
		PrincipalId:  principalId,
		ResourceId:   fileId,
		ResourceType: "files",
	}

	authzCheckResponse, err := authzClient.Check(ctx, &authzCheckRequest)
	if err != nil {
		return "", err
	}

	if !authzCheckResponse.GetOk() {
		return "", nil
	}

	return authzCheckResponse.Attrs.Fields["role"].GetStringValue(), nil
}

func getUserFromPrincipalId(ctx context.Context, principalId string) (*User, error) {
	principalsClient, err := getPrincipalsClient()
	if err != nil {
		return nil, err
	}

	req := &sentium.PrincipalsRetrieveRequest{
		Id: principalId,
	}
	principal, err := principalsClient.Retrieve(ctx, req)
	if err != nil {
		return nil, err
	}

	user := &User{
		Id: principal.Id,
	}

	attrs := principal.GetAttrs()
	if attrs != nil && attrs.Fields["name"] != nil {
		user.Name = attrs.Fields["name"].GetStringValue()
	}

	if principal.Segment != nil {
		user.Segment = *principal.Segment
	}

	return user, nil
}

func getPaginationParams(c *gin.Context) (*uint32, *string) {
	var paginationLimit *uint32
	if limit, ok := c.GetQuery("pagination_limit"); ok {
		l64, _ := strconv.ParseUint(limit, 10, 32)
		l32 := uint32(l64)
		paginationLimit = &l32
	}

	var paginationToken *string
	if token, ok := c.GetQuery("pagination_token"); ok {
		paginationToken = &token
	}

	return paginationLimit, paginationToken
}
