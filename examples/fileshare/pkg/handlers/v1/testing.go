package v1

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"

	"github.com/rs/xid"
	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
	"google.golang.org/protobuf/types/known/structpb"
)

func RouteHttp(
	handler http.Handler,
	method string,
	path string,
	request interface{},
	headers map[string]string,
) (*http.Response, error) {
	var buf bytes.Buffer
	if err := json.NewEncoder(&buf).Encode(request); err != nil {
		return nil, err
	}

	req := httptest.NewRequest(method, path, &buf)
	for k, v := range headers {
		req.Header.Set(k, v)
	}

	w := httptest.NewRecorder()
	handler.ServeHTTP(w, req)
	return w.Result(), nil
}

func CheckFileExistsForUser(ctx context.Context, fileId string, userId string) (bool, error) {
	resourcesClient, err := getResourcesClient()
	if err != nil {
		return false, err
	}

	resourcesListReq := sentium.ResourcesListRequest{
		PrincipalId:  userId,
		ResourceType: "files",
	}

	resourcesListResp, err := resourcesClient.List(ctx, &resourcesListReq)
	if err != nil {
		return false, err
	}

	for _, file := range resourcesListResp.Resources {
		if file.Id == fileId {
			return true, nil
		}
	}

	return false, nil
}

func filesCreate(ctx context.Context, numFiles int, ownerId string) ([]File, error) {
	files := []File{}

	authzClient, err := getAuthzClient()
	if err != nil {
		return nil, err
	}

	for i := 0; i < numFiles; i++ {
		attrs, err := structpb.NewStruct(map[string]interface{}{
			"name": fmt.Sprintf("File Name %d", i),
			"role": "owner",
		})
		if err != nil {
			return nil, err
		}

		authzGrantRequest := sentium.AuthzGrantRequest{
			Attrs:        attrs,
			PrincipalId:  ownerId,
			ResourceId:   xid.New().String(),
			ResourceType: "files",
		}

		if _, err := authzClient.Grant(ctx, &authzGrantRequest); err != nil {
			return nil, err
		}

		file := File{
			Id:   authzGrantRequest.ResourceId,
			Name: attrs.Fields["name"].GetStringValue(),
			Role: "owner",
		}

		files = append([]File{file}, files...)
	}

	return files, nil
}

func filesDelete(ctx context.Context, files []File, principalId string) error {
	authzClient, err := getAuthzClient()
	if err != nil {
		return err
	}

	for _, file := range files {
		delReq := sentium.AuthzRevokeRequest{
			PrincipalId:  principalId,
			ResourceId:   file.Id,
			ResourceType: "files",
		}

		if _, err := authzClient.Revoke(ctx, &delReq); err != nil {
			return err
		}
	}

	return nil
}

func filesShare(ctx context.Context, file File, principalId string, role string) error {
	authzClient, err := getAuthzClient()
	if err != nil {
		return err
	}

	attrs, err := structpb.NewStruct(map[string]interface{}{
		"name": file.Name,
		"role": role,
	})
	if err != nil {
		return err
	}

	authzGrantRequest := sentium.AuthzGrantRequest{
		Attrs:        attrs,
		PrincipalId:  principalId,
		ResourceId:   file.Id,
		ResourceType: "files",
	}

	if _, err := authzClient.Grant(ctx, &authzGrantRequest); err != nil {
		return err
	}

	return nil
}

func usersCreate(ctx context.Context, segment *string, numUsers int) ([]User, error) {
	users := []User{}

	principalClient, err := getPrincipalsClient()
	if err != nil {
		return nil, err
	}

	for i := 0; i < numUsers; i++ {
		principalId := xid.New().String()
		attrs, err := structpb.NewStruct(map[string]interface{}{
			"name": fmt.Sprintf("User Name %d", i),
		})
		if err != nil {
			return nil, err
		}

		principalsCreateRequest := sentium.PrincipalsCreateRequest{
			Attrs:   attrs,
			Id:      &principalId,
			Segment: segment,
		}

		principal, err := principalClient.Create(ctx, &principalsCreateRequest)
		if err != nil {
			return nil, err
		}

		user := User{
			Id:   principal.Id,
			Name: principal.Attrs.Fields["name"].GetStringValue(),
		}

		if segment != nil {
			user.Segment = *segment
		}

		users = append([]User{user}, users...)
	}

	return users, nil
}

func usersDelete(ctx context.Context, users []User) error {
	for _, user := range users {
		delReq := sentium.PrincipalsDeleteRequest{
			Id: user.Id,
		}

		if _, err := principalsClient.Delete(ctx, &delReq); err != nil {
			return err
		}
	}

	return nil
}
