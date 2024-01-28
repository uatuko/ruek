package v1

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"testing"

	"github.com/gin-gonic/gin"
	"github.com/rs/xid"
	"github.com/stretchr/testify/require"
)

func TestCreateFile(t *testing.T) {
	router := gin.New()
	router.POST("/files", createFile)

	users, err := createUsers("segment", 1)
	require.NoError(t, err)
	defer deleteUsers(users)
	headers := map[string]string{
		"user-id": users[0].Id,
	}

	fileReq := CreateFileRequest{
		Name: "File Name",
	}

	t.Run("FailPrincipalNotFound", func(t *testing.T) {
		headers := map[string]string{
			"user-id": "unicorn",
		}

		resp, err := RouteHttp(router, "POST", "/files", fileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusInternalServerError, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		expedtedResp := "\"rpc error: code = InvalidArgument desc = [sentium:1.3.2.400] Invalid principal for record\""
		require.Equal(t, expedtedResp, string(respBody))
	})

	t.Run("FailMissingName", func(t *testing.T) {
		resp, err := RouteHttp(router, "POST", "/files", CreateFileRequest{}, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusBadRequest, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		require.Equal(
			t,
			"\"Key: 'CreateFileRequest.Name' Error:Field validation for 'Name' failed on the 'required' tag\"",
			string(respBody),
		)
	})

	t.Run("Success", func(t *testing.T) {
		resp, err := RouteHttp(router, "POST", "/files", fileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusCreated, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var file File
		json.Unmarshal(respBody, &file)
		defer filesDelete([]File{file}, headers["user-id"])

		expectedResp := File{
			Id:   file.Id,
			Name: fileReq.Name,
			Role: "owner",
		}

		require.Equal(t, expectedResp, file)
	})
}

func TestListFiles(t *testing.T) {
	router := gin.New()
	router.GET("/files", listFiles)

	users, err := createUsers("segment", 1)
	require.NoError(t, err)
	defer deleteUsers(users)
	userId := users[0].Id
	headers := map[string]string{
		"user-id": userId,
	}

	numFiles := 5
	files, err := filesCreate(numFiles, userId)
	require.NoError(t, err)
	defer filesDelete(files, userId)

	t.Run("SuccessWithPaginationLimitNoToken", func(t *testing.T) {
		path := fmt.Sprintf("/files?pagination_limit=%d", numFiles-1)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFilesResp ListFilesResponse
		json.Unmarshal(respBody, &listFilesResp)

		require.NotEmpty(t, listFilesResp.PaginationToken)
		require.Len(t, listFilesResp.Files, numFiles-1)
	})

	t.Run("SuccessNoPaginationNoFiles", func(t *testing.T) {
		headers := map[string]string{
			"Userid": "unicorn",
		}

		resp, err := RouteHttp(router, "GET", "/files", nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFilesResp ListFilesResponse
		json.Unmarshal(respBody, &listFilesResp)

		require.Equal(t, "", listFilesResp.PaginationToken)
		require.Empty(t, listFilesResp.Files)
	})

	t.Run("successWithTokenNoLimit", func(t *testing.T) {
		// Run first search to iobrtain a token
		limit := 3
		path := fmt.Sprintf("/files?pagination_limit=%d", limit)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFilesResp ListFilesResponse
		json.Unmarshal(respBody, &listFilesResp)

		token := listFilesResp.PaginationToken
		require.NotEmpty(t, token)

		// Second request using token
		path = fmt.Sprintf("/files?pagination_token=%s", token)
		resp, err = RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		respBody, err = io.ReadAll(resp.Body)
		require.NoError(t, err)

		json.Unmarshal(respBody, &listFilesResp)

		require.Empty(t, listFilesResp.PaginationToken)
		require.Len(t, listFilesResp.Files, numFiles-limit)
	})

	t.Run("SuccessMaxFilesReturned", func(t *testing.T) {
		path := fmt.Sprintf("/files?pagination_limit=%d", numFiles)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFilesResp ListFilesResponse
		json.Unmarshal(respBody, &listFilesResp)

		require.NotEqual(t, "", listFilesResp.PaginationToken)
		require.Len(t, listFilesResp.Files, numFiles)
	})
}

func TestShareFile(t *testing.T) {
	router := gin.New()
	router.POST("/files/:file/user:share", shareFile)

	// Create users
	users, err := createUsers(xid.New().String(), 5)
	require.NoError(t, err)
	ownerId := users[0].Id
	editorId := users[1].Id
	viewerId := users[2].Id
	noAccessUser := users[3].Id
	shareeId := users[4].Id
	defer deleteUsers(users)

	// Create and share file with editor and viewer
	files, err := filesCreate(1, ownerId)
	require.NoError(t, err)
	file := files[0]
	defer filesDelete(files, ownerId)

	err = filesShare(file, editorId, "editor")
	require.NoError(t, err)

	err = filesShare(file, viewerId, "viewer")
	require.NoError(t, err)

	shareFileReq := ShareFileRequest{
		Role:   "editor",
		UserId: shareeId,
	}

	headers := map[string]string{
		"user-id": ownerId,
	}

	t.Run("FailMissingSharee", func(t *testing.T) {
		req := ShareFileRequest{
			Role: "editor",
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", req, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusBadRequest, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"Key: 'ShareFileRequest.UserId' Error:Field validation for 'UserId' failed on the 'required' tag\"",
			string(respBody),
		)
	})

	t.Run("FailMissingRole", func(t *testing.T) {
		req := ShareFileRequest{
			UserId: shareeId,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", req, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusBadRequest, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"Key: 'ShareFileRequest.Role' Error:Field validation for 'Role' failed on the 'required' tag\"",
			string(respBody),
		)
	})

	t.Run("FailInvalidRole", func(t *testing.T) {
		req := ShareFileRequest{
			Role:   "invalid",
			UserId: shareeId,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", req, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusBadRequest, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"Key: 'ShareFileRequest.Role' Error:Field validation for 'Role' failed on the 'oneof' tag\"",
			string(respBody),
		)
	})

	t.Run("FailNoAccess", func(t *testing.T) {
		headers := map[string]string{
			"user-id": noAccessUser,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusForbidden, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"no access to file\"",
			string(respBody),
		)
	})

	t.Run("FailRoleCannotShare", func(t *testing.T) {
		headers := map[string]string{
			"user-id": viewerId,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusForbidden, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"role cannot share (sharer role: viewer)\"",
			string(respBody),
		)
	})

	t.Run("FailCannotGiveHigherPermissions", func(t *testing.T) {
		headers := map[string]string{
			"user-id": editorId,
		}

		shareFileReq := ShareFileRequest{
			Role:   "owner",
			UserId: shareeId,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusForbidden, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"cannot share to higher role (sharer role: editor, sharee role: owner)\"",
			string(respBody),
		)
	})

	t.Run("FailShareeNotFound", func(t *testing.T) {
		shareFileReq := ShareFileRequest{
			UserId: "unicorn",
			Role:   "editor",
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusInternalServerError, resp.StatusCode)
		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(t, "\"rpc error: code = InvalidArgument desc = [sentium:1.3.2.400] Invalid principal for record\"", string(respBody))
	})

	t.Run("SuccessSharedByOwner", func(t *testing.T) {
		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)
	})

	t.Run("SuccessSharedByEditor", func(t *testing.T) {
		headers := map[string]string{
			"user-id": editorId,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/user:share", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)
	})
}
