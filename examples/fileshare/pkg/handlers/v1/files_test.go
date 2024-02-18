package v1

import (
	"context"
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
	ctx := context.Background()
	router := gin.New()
	router.POST("/files", createFile)

	users, err := usersCreate(ctx, nil, 1)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
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
		defer filesDelete(ctx, []File{file}, headers["user-id"])

		expectedResp := File{
			Id:   file.Id,
			Name: fileReq.Name,
			Role: "owner",
		}

		require.Equal(t, expectedResp, file)
	})
}

func TestDeleteFile(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.DELETE("/files/:file", deleteFile)

	users, err := usersCreate(ctx, nil, 4)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	owner := users[0]
	editor := users[1]
	viewer := users[2]
	noAccess := users[3]

	files, err := filesCreate(ctx, 3, owner.Id)
	require.NoError(t, err)
	defer filesDelete(ctx, files, owner.Id)
	deleteByOwner := files[0]
	deleteByEditor := files[1]
	deleteByViewer := files[2]

	// Share files
	err = filesShare(ctx, deleteByOwner, editor.Id, "editor")
	require.NoError(t, err)

	err = filesShare(ctx, deleteByEditor, editor.Id, "editor")
	require.NoError(t, err)

	err = filesShare(ctx, deleteByEditor, viewer.Id, "viewer")
	require.NoError(t, err)

	err = filesShare(ctx, deleteByViewer, viewer.Id, "viewer")
	require.NoError(t, err)

	t.Run("FailNoAccess", func(t *testing.T) {
		headers := map[string]string{
			"user-id": noAccess.Id,
		}

		path := fmt.Sprintf("/files/%s", deleteByViewer.Id)
		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
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

	t.Run("FailViewer", func(t *testing.T) {
		headers := map[string]string{
			"user-id": viewer.Id,
		}

		path := fmt.Sprintf("/files/%s", deleteByViewer.Id)
		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusForbidden, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"cannot delee file (role: viewer)\"",
			string(respBody),
		)
	})

	t.Run("SuccessOwner", func(t *testing.T) {
		// Check owner and editor have acces to file
		exists, err := checkFileExistsForUser(ctx, deleteByOwner.Id, owner.Id)
		require.NoError(t, err)
		require.True(t, exists)

		exists, err = checkFileExistsForUser(ctx, deleteByOwner.Id, editor.Id)
		require.NoError(t, err)
		require.True(t, exists)

		// Send request to delete file
		headers := map[string]string{
			"user-id": owner.Id,
		}

		path := fmt.Sprintf("/files/%s", deleteByOwner.Id)
		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)

		// Check the file was deleted for owner
		exists, err = checkFileExistsForUser(ctx, deleteByOwner.Id, owner.Id)
		require.NoError(t, err)
		require.False(t, exists)

		// Check the file was deleted for editor
		exists, err = checkFileExistsForUser(ctx, deleteByOwner.Id, editor.Id)
		require.NoError(t, err)
		require.False(t, exists)
	})

	t.Run("SuccessEditor", func(t *testing.T) {
		// Check owner, editor and viewer have access to file
		exists, err := checkFileExistsForUser(ctx, deleteByEditor.Id, owner.Id)
		require.NoError(t, err)
		require.True(t, exists)

		exists, err = checkFileExistsForUser(ctx, deleteByEditor.Id, editor.Id)
		require.NoError(t, err)
		require.True(t, exists)

		exists, err = checkFileExistsForUser(ctx, deleteByEditor.Id, viewer.Id)
		require.NoError(t, err)
		require.True(t, exists)

		// Send request to delete file
		headers := map[string]string{
			"user-id": editor.Id,
		}

		path := fmt.Sprintf("/files/%s", deleteByEditor.Id)
		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)

		// Check the file was deleted for owner
		exists, err = checkFileExistsForUser(ctx, deleteByEditor.Id, owner.Id)
		require.NoError(t, err)
		require.False(t, exists)

		// Check the file was deleted for editor
		exists, err = checkFileExistsForUser(ctx, deleteByEditor.Id, editor.Id)
		require.NoError(t, err)
		require.False(t, exists)

		// Check the file was deleted for viewer
		exists, err = checkFileExistsForUser(ctx, deleteByEditor.Id, viewer.Id)
		require.NoError(t, err)
		require.False(t, exists)
	})
}

func TestGetFile(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.GET("/files/:file", getFile)

	users, err := usersCreate(ctx, nil, 2)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	userWithAccess := users[0]
	userNoAccess := users[1]

	files, err := filesCreate(ctx, 1, userWithAccess.Id)
	require.NoError(t, err)
	defer filesDelete(ctx, files, userWithAccess.Id)
	fileId := files[0].Id

	t.Run("FailNoAccess", func(t *testing.T) {
		headers := map[string]string{
			"user-id": userNoAccess.Id,
		}

		path := fmt.Sprintf("/files/%s", fileId)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
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

	t.Run("FailNotExists", func(t *testing.T) {
		headers := map[string]string{
			"user-id": userWithAccess.Id,
		}

		resp, err := RouteHttp(router, "GET", "/files/1234", nil, headers)
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

	t.Run("Success", func(t *testing.T) {
		headers := map[string]string{
			"user-id": userWithAccess.Id,
		}

		path := fmt.Sprintf("/files/%s", fileId)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var actual File
		err = json.Unmarshal(respBody, &actual)
		require.NoError(t, err)
		require.Equal(t, files[0], actual)
	})
}

func TestListFiles(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.GET("/files", listFiles)

	users, err := usersCreate(ctx, nil, 1)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	userId := users[0].Id
	headers := map[string]string{
		"user-id": userId,
	}

	numFiles := 5
	files, err := filesCreate(ctx, numFiles, userId)
	require.NoError(t, err)
	defer filesDelete(ctx, files, userId)

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

	t.Run("SuccessWithTokenNoLimit", func(t *testing.T) {
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

func TestListFileUsers(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.GET("/files/:file/users", listFileUsers)

	// Create users
	users, err := usersCreate(ctx, nil, 4)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	owner := users[0]
	editor := users[1]
	viewer := users[2]
	noAccessUser := users[3]

	// Create files
	files, err := filesCreate(ctx, 1, owner.Id)
	require.NoError(t, err)
	defer filesDelete(ctx, files, owner.Id)
	file := files[0]

	// Share files
	err = filesShare(ctx, file, editor.Id, "editor")
	require.NoError(t, err)

	err = filesShare(ctx, file, viewer.Id, "viewer")
	require.NoError(t, err)

	usersWithAccess := []FileUser{
		{
			Id:   owner.Id,
			Name: owner.Name,
			Role: "owner",
		},
		{
			Id:   editor.Id,
			Name: editor.Name,
			Role: "editor",
		},
		{
			Id:   viewer.Id,
			Name: viewer.Name,
			Role: "viewer",
		},
	}

	t.Run("FailFileDoesNotExist", func(t *testing.T) {
		headers := map[string]string{
			"user-id": owner.Id,
		}

		resp, err := RouteHttp(router, "GET", "/files/1234/users", nil, headers)
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

	t.Run("FailUserDoesNotExist", func(t *testing.T) {
		headers := map[string]string{
			"user-id": "1234",
		}

		path := fmt.Sprintf("/files/%s/users", file.Id)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
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

	t.Run("FailNoFileAccess", func(t *testing.T) {
		headers := map[string]string{
			"user-id": noAccessUser.Id,
		}

		path := fmt.Sprintf("/files/%s/users", file.Id)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
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

	t.Run("SuccessOwner", func(t *testing.T) {
		headers := map[string]string{
			"user-id": owner.Id,
		}

		path := fmt.Sprintf("/files/%s/users", file.Id)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFileUsersResp ListFileUsersResponse
		json.Unmarshal(respBody, &listFileUsersResp)

		require.Len(t, listFileUsersResp.Users, 3)
		require.ElementsMatch(t, usersWithAccess, listFileUsersResp.Users)
	})

	t.Run("SuccessEditor", func(t *testing.T) {
		headers := map[string]string{
			"user-id": editor.Id,
		}

		path := fmt.Sprintf("/files/%s/users", file.Id)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFileUsersResp ListFileUsersResponse
		json.Unmarshal(respBody, &listFileUsersResp)

		require.Len(t, listFileUsersResp.Users, 3)
		require.ElementsMatch(t, usersWithAccess, listFileUsersResp.Users)
	})

	t.Run("SuccessViewer", func(t *testing.T) {
		headers := map[string]string{
			"user-id": viewer.Id,
		}

		path := fmt.Sprintf("/files/%s/users", file.Id)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFileUsersResp ListFileUsersResponse
		json.Unmarshal(respBody, &listFileUsersResp)

		require.Len(t, listFileUsersResp.Users, 3)
		require.ElementsMatch(t, usersWithAccess, listFileUsersResp.Users)
	})

	t.Run("seccessOwnerWithPaginationLimit", func(t *testing.T) {
		headers := map[string]string{
			"user-id": owner.Id,
		}

		numUsers := 2
		path := fmt.Sprintf("/files/%s/users?pagination_limit=%d", file.Id, numUsers)
		resp, err := RouteHttp(router, "GET", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listFileUsersResp ListFileUsersResponse
		json.Unmarshal(respBody, &listFileUsersResp)

		require.Len(t, listFileUsersResp.Users, numUsers)
		require.NotEmpty(t, listFileUsersResp.PaginationToken)
	})
}

func TestShareFile(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.POST("/files/:file/users", shareFile)

	// Create users
	segment := xid.New().String()
	users, err := usersCreate(ctx, &segment, 5)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	ownerId := users[0].Id
	editorId := users[1].Id
	viewerId := users[2].Id
	noAccessUser := users[3].Id
	shareeId := users[4].Id

	// Create and share file with editor and viewer
	files, err := filesCreate(ctx, 1, ownerId)
	require.NoError(t, err)
	file := files[0]
	defer filesDelete(ctx, files, ownerId)

	err = filesShare(ctx, file, editorId, "editor")
	require.NoError(t, err)

	err = filesShare(ctx, file, viewerId, "viewer")
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", req, headers)
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", req, headers)
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", req, headers)
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", shareFileReq, headers)
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", shareFileReq, headers)
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", shareFileReq, headers)
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

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusInternalServerError, resp.StatusCode)
		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(t, "\"rpc error: code = InvalidArgument desc = [sentium:1.3.2.400] Invalid principal for record\"", string(respBody))
	})

	t.Run("SuccessSharedByOwner", func(t *testing.T) {
		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)
	})

	t.Run("SuccessSharedByEditor", func(t *testing.T) {
		headers := map[string]string{
			"user-id": editorId,
		}

		resp, err := RouteHttp(router, "POST", "/files/"+file.Id+"/users", shareFileReq, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)
	})
}

func TestUnshareFile(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.DELETE("/files/:file/users/:user", unshareFile)

	// Create users
	users, err := usersCreate(ctx, nil, 7)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	owner := users[0]
	editor1 := users[1]
	editor2 := users[2]
	viewer1 := users[3]
	viewer2 := users[4]
	viewer3 := users[5]
	noAccessUser := users[6]

	// Create file
	files, err := filesCreate(ctx, 1, owner.Id)
	require.NoError(t, err)
	defer filesDelete(ctx, files, owner.Id)
	file := files[0]

	// Share file
	err = filesShare(ctx, file, editor1.Id, "editor")
	require.NoError(t, err)
	err = filesShare(ctx, file, editor2.Id, "editor")
	require.NoError(t, err)

	err = filesShare(ctx, file, viewer1.Id, "viewer")
	require.NoError(t, err)
	err = filesShare(ctx, file, viewer2.Id, "viewer")
	require.NoError(t, err)
	err = filesShare(ctx, file, viewer3.Id, "viewer")
	require.NoError(t, err)

	t.Run("FailNoAccessToFile", func(t *testing.T) {
		path := fmt.Sprintf("/files/%s/users/%s", file.Id, viewer1.Id)
		headers := map[string]string{
			"user-id": noAccessUser.Id,
		}

		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
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

	t.Run("FailUnsharingHigherRole", func(t *testing.T) {
		path := fmt.Sprintf("/files/%s/users/%s", file.Id, editor1.Id)
		headers := map[string]string{
			"user-id": viewer1.Id,
		}

		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusUnprocessableEntity, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"cannot unshare from a higher role (requestor role: viewer, unshared role: editor)\"",
			string(respBody),
		)
	})

	t.Run("SuccessOwnerUnshareEditor", func(t *testing.T) {
		// Check user the file will be unshared woth has access to file
		role, err := getRole(ctx, editor1.Id, file.Id)
		require.NoError(t, err)
		require.Equal(t, "editor", role)

		// Unshare file
		path := fmt.Sprintf("/files/%s/users/%s", file.Id, editor1.Id)
		headers := map[string]string{
			"user-id": owner.Id,
		}

		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)

		role, err = getRole(ctx, editor1.Id, file.Id)
		require.NoError(t, err)
		require.Empty(t, role)
	})

	t.Run("SuccessEditorUnshareViewer", func(t *testing.T) {
		// Check user the file will be unshared woth has access to file
		role, err := getRole(ctx, viewer1.Id, file.Id)
		require.NoError(t, err)
		require.Equal(t, "viewer", role)

		// Unshare file
		path := fmt.Sprintf("/files/%s/users/%s", file.Id, viewer1.Id)
		headers := map[string]string{
			"user-id": editor2.Id,
		}

		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)

		role, err = getRole(ctx, viewer1.Id, file.Id)
		require.NoError(t, err)
		require.Empty(t, role)
	})

	t.Run("SuccessViewerUnshareViewer", func(t *testing.T) {
		// Check user the file will be unshared woth has access to file
		role, err := getRole(ctx, viewer2.Id, file.Id)
		require.NoError(t, err)
		require.Equal(t, "viewer", role)

		// Unshare file
		path := fmt.Sprintf("/files/%s/users/%s", file.Id, viewer2.Id)
		headers := map[string]string{
			"user-id": viewer3.Id,
		}

		resp, err := RouteHttp(router, "DELETE", path, nil, headers)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)

		role, err = getRole(ctx, viewer2.Id, file.Id)
		require.NoError(t, err)
		require.Empty(t, role)
	})
}
