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

func createUsers(segment string, numUsers int) ([]User, error) {
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
			Segment: &segment,
		}

		principal, err := principalClient.Create(context.Background(), &principalsCreateRequest)
		if err != nil {
			return nil, err
		}

		user := User{
			Id:      principal.Id,
			Name:    principal.Attrs.Fields["name"].GetStringValue(),
			Segment: segment,
		}

		users = append([]User{user}, users...)
	}

	return users, nil
}

func deleteUsers(users []User) error {
	for _, user := range users {
		delReq := sentium.PrincipalsDeleteRequest{
			Id: user.Id,
		}

		if _, err := principalsClient.Delete(context.Background(), &delReq); err != nil {
			return err
		}
	}

	return nil
}

func filesCreate(numFiles int, principalId string, role string) ([]File, error) {
	files := []File{}

	authzClient, err := getAuthzClient()
	if err != nil {
		return nil, err
	}

	attrs, err := structpb.NewStruct(map[string]interface{}{
		"name": "File Name",
		"role": role,
	})
	if err != nil {
		return nil, err
	}

	for i := 0; i < numFiles; i++ {
		authzGrantRequest := sentium.AuthzGrantRequest{
			Attrs:        attrs,
			PrincipalId:  principalId,
			ResourceId:   xid.New().String(),
			ResourceType: "files",
		}

		if _, err := authzClient.Grant(context.Background(), &authzGrantRequest); err != nil {
			return nil, err
		}

		file := File{
			Id:   authzGrantRequest.ResourceId,
			Name: "",
			Role: role,
		}

		files = append([]File{file}, files...)
	}

	return files, nil
}

func filesDelete(files []File, principalId string) error {
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

		if _, err := authzClient.Revoke(context.Background(), &delReq); err != nil {
			return err
		}
	}

	return nil
}

func filesShare(file File, principalId string, role string) error {
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

	if _, err := authzClient.Grant(context.Background(), &authzGrantRequest); err != nil {
		return err
	}

	return nil
}
