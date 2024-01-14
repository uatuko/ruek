package v1

import (
	"context"
	"encoding/json"
	"io"
	"net/http"
	"testing"

	"github.com/gin-gonic/gin"
	sentium_grpc "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
	"github.com/stretchr/testify/require"
)

func TestCreateFileFailOnPrincipalNotFound(t *testing.T) {
	router := gin.New()
	router.POST("/files", createFile)

	fileReq := CreateFileRequest{
		Name: "Best Test File",
		Type: "best-file-type",
	}

	headers := map[string]string{
		"Userid": "unicorn",
	}
	resp, err := RouteHttp(router, "POST", "/files", fileReq, headers)
	require.NoError(t, err)

	require.Equal(t, http.StatusInternalServerError, resp.StatusCode)

	respBody, err := io.ReadAll(resp.Body)
	require.NoError(t, err)

	expedtedResp := "\"rpc error: code = InvalidArgument desc = [sentium:1.3.2.400] Invalid principal for record\""
	require.Equal(t, expedtedResp, string(respBody))
}

func TestCreateFileSuccess(t *testing.T) {
	router := gin.New()
	router.POST("/files", createFile)

	fileReq := CreateFileRequest{
		Name: "Best Test File",
		Type: "best-file-type",
	}

	headers := map[string]string{
		"Userid": "1234",
	}
	resp, err := RouteHttp(router, "POST", "/files", fileReq, headers)
	require.NoError(t, err)

	require.Equal(t, http.StatusCreated, resp.StatusCode)

	respBody, err := io.ReadAll(resp.Body)
	require.NoError(t, err)

	var file File
	json.Unmarshal(respBody, &file)

	expectedResp := File{
		Id:   file.Id,
		Name: fileReq.Name,
		Type: fileReq.Type,
	}

	require.Equal(t, expectedResp, file)
}

func TestShareFileFailNoAccess(t *testing.T) {
	router := gin.New()
	router.POST("/files/:file/user:share", shareFile)

	headers := map[string]string{
		"Userid": "1234",
	}

	shareFileReq := ShareFileRequest{
		UserId: "5678",
	}

	resp, err := RouteHttp(router, "POST", "/files/1234/user:share", shareFileReq, headers)
	require.NoError(t, err)

	require.Equal(t, http.StatusForbidden, resp.StatusCode)
}

func TestShareFileFailNoShareeUser(t *testing.T) {
	router := gin.New()
	router.POST("/files/:file/user:share", shareFile)

	uid := "1234"
	resourceId := "abcd"
	headers := map[string]string{
		"Userid": uid,
	}

	shareFileReq := ShareFileRequest{
		UserId: "unicorn",
	}

	authzClient, err := getAuthzClient()
	require.NoError(t, err)

	authzGrantRequest := sentium_grpc.AuthzGrantRequest{
		PrincipalId:  uid,
		ResourceId:   resourceId,
		ResourceType: "files",
	}

	_, err = authzClient.Grant(context.Background(), &authzGrantRequest)
	require.NoError(t, err)

	resp, err := RouteHttp(router, "POST", "/files/"+resourceId+"/user:share", shareFileReq, headers)
	require.NoError(t, err)

	require.Equal(t, http.StatusInternalServerError, resp.StatusCode)
	respBody, err := io.ReadAll(resp.Body)
	require.NoError(t, err)
	require.Equal(t, "\"rpc error: code = InvalidArgument desc = [sentium:1.3.2.400] Invalid principal for record\"", string(respBody))
}

func TestShareFileSuccess(t *testing.T) {
	router := gin.New()
	router.POST("/files/:file/user:share", shareFile)

	uid := "1234"
	resourceId := "abcd"
	headers := map[string]string{
		"Userid": uid,
	}

	shareFileReq := ShareFileRequest{
		UserId: "5678",
	}

	authzClient, err := getAuthzClient()
	require.NoError(t, err)

	authzGrantRequest := sentium_grpc.AuthzGrantRequest{
		PrincipalId:  uid,
		ResourceId:   resourceId,
		ResourceType: "files",
	}

	_, err = authzClient.Grant(context.Background(), &authzGrantRequest)
	require.NoError(t, err)

	resp, err := RouteHttp(router, "POST", "/files/"+resourceId+"/user:share", shareFileReq, headers)
	require.NoError(t, err)

	require.Equal(t, http.StatusNoContent, resp.StatusCode)
}
