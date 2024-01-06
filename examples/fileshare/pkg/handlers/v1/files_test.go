package v1

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
	"testing"

	"github.com/gin-gonic/gin"
	"github.com/stretchr/testify/require"
)

func TestCreateFileFailOnPrincipalNotFound(t *testing.T) {
	router := gin.New()
	router.POST("/files", createFile)

	fileReq := CreateFileRequest{
		Name: "Best Test File",
		Type: "best-file-type",
	}

	var buf bytes.Buffer
	err := json.NewEncoder(&buf).Encode(fileReq)
	require.NoError(t, err)

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

	var buf bytes.Buffer
	err := json.NewEncoder(&buf).Encode(fileReq)
	require.NoError(t, err)

	headers := map[string]string{
		"Userid": "1234",
	}
	resp, err := RouteHttp(router, "POST", "/files", fileReq, headers)
	require.NoError(t, err)

	require.Equal(t, http.StatusCreated, resp.StatusCode)

	respBody, err := io.ReadAll(resp.Body)
	require.NoError(t, err)

	var fileResp CreateFileResponse
	json.Unmarshal(respBody, &fileResp)

	expectedResp := CreateFileResponse{
		Id:   fileResp.Id,
		Name: fileReq.Name,
		Type: fileReq.Type,
	}

	require.Equal(t, expectedResp, fileResp)
}
