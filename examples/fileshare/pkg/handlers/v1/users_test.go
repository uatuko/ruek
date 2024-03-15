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
	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

func TestCreateUser(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.POST("/users", createUser)

	t.Run("FailMissingName", func(t *testing.T) {
		resp, err := RouteHttp(router, "POST", "/users", CreateUserRequest{}, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusBadRequest, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)
		require.Equal(
			t,
			"\"Key: 'CreateUserRequest.Name' Error:Field validation for 'Name' failed on the 'required' tag\"",
			string(respBody),
		)
	})

	t.Run("Success", func(t *testing.T) {
		userReq := CreateUserRequest{
			Name: "Best Users",
		}

		resp, err := RouteHttp(router, "POST", "/users", userReq, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusCreated, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var user User
		json.Unmarshal(respBody, &user)
		defer usersDelete(ctx, []User{user})

		expectedResp := User{
			Id:   user.Id,
			Name: userReq.Name,
		}

		require.Equal(t, expectedResp, user)
	})
}

func TestDeleteUser(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.DELETE("/users/:user", deleteUser)

	users, err := usersCreate(ctx, nil, 2)
	require.NoError(t, err)
	defer usersDelete(ctx, users)
	owner := users[0]
	editor := users[1]

	files, err := filesCreate(ctx, 1, users[0].Id)
	require.NoError(t, err)
	defer filesDelete(ctx, files, users[0].Id)

	err = filesShare(ctx, files[0], editor.Id, "editor")
	require.NoError(t, err)

	t.Run("FailNotFound", func(t *testing.T) {
		resp, err := RouteHttp(router, "DELETE", "/users/not-found", nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusNotFound, resp.StatusCode)
	})

	t.Run("Success", func(t *testing.T) {
		path := fmt.Sprintf("/users/%s", owner.Id)
		resp, err := RouteHttp(router, "DELETE", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusNoContent, resp.StatusCode)

		// Check the user does not exist
		principalsClient, err := getPrincipalsClient()
		require.NoError(t, err)

		principalRetriveReq := &sentium.PrincipalsRetrieveRequest{
			Id: owner.Id,
		}

		_, err = principalsClient.Retrieve(ctx, principalRetriveReq)
		require.NotNil(t, err)

		stts, ok := status.FromError(err)
		require.True(t, ok)
		require.Equal(t, codes.NotFound, stts.Code())

		// Check the file access was revoked for deleted user
		ok, err = checkFileExistsForUser(ctx, files[0].Id, owner.Id)
		require.NoError(t, err)
		require.False(t, ok)

		// Check editor still has access to file
		ok, err = checkFileExistsForUser(ctx, files[0].Id, editor.Id)
		require.NoError(t, err)
		require.True(t, ok)
	})
}

func TestGetUser(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.GET("/users/:user", getUser)

	segment := "user-segment"
	users, err := usersCreate(ctx, &segment, 1)
	require.NoError(t, err)
	defer usersDelete(ctx, users)

	t.Run("NotFound", func(t *testing.T) {
		path := fmt.Sprintf("/users/%s", "not-found")
		resp, err := RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusNotFound, resp.StatusCode)
	})

	t.Run("Success", func(t *testing.T) {
		path := fmt.Sprintf("/users/%s", users[0].Id)
		resp, err := RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var actual User
		err = json.Unmarshal(respBody, &actual)
		require.NoError(t, err)
		require.Equal(t, users[0], actual)
	})
}

func TestListUsers(t *testing.T) {
	ctx := context.Background()
	router := gin.New()
	router.GET("/users", listUsers)

	segment := xid.New().String()
	numUsersWithSegment := 5
	usersWithSegment, err := usersCreate(ctx, &segment, numUsersWithSegment)
	require.NoError(t, err)
	defer usersDelete(ctx, usersWithSegment)

	numUsersNoSegment := 2
	usersNoSegment, err := usersCreate(ctx, nil, numUsersNoSegment)
	require.NoError(t, err)
	defer usersDelete(ctx, usersNoSegment)

	t.Run("SuccessWithUsers", func(t *testing.T) {
		path := fmt.Sprintf("/users?segment=%s", segment)
		resp, err := RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.Empty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, numUsersWithSegment)
		require.Equal(t, usersWithSegment, listUsersResp.Users)
	})

	t.Run("SuccessNoUsers", func(t *testing.T) {
		path := fmt.Sprintf("/users?segment=%s", xid.New().String())
		resp, err := RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.Empty(t, listUsersResp.PaginationToken)
		require.Empty(t, listUsersResp.Users)
	})

	t.Run("SuccessWithPaginationLimitNoToken", func(t *testing.T) {
		path := fmt.Sprintf("/users?pagination_limit=%d&segment=%s", numUsersWithSegment-1, segment)
		resp, err := RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.NotEmpty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, numUsersWithSegment-1)
	})

	t.Run("SuccessWithPaginationLimitAndToken", func(t *testing.T) {
		// First search to obtain the token
		path := fmt.Sprintf("/users?pagination_limit=%d&segment=%s", numUsersWithSegment-1, segment)
		resp, err := RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.NotEmpty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, numUsersWithSegment-1)

		// Second search, using the token
		path = fmt.Sprintf(
			"/users?pagination_token=%s&segment=%s",
			listUsersResp.PaginationToken,
			segment,
		)

		resp, err = RouteHttp(router, "GET", path, nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err = io.ReadAll(resp.Body)
		require.NoError(t, err)

		json.Unmarshal(respBody, &listUsersResp)

		require.Empty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, 1)
	})

	t.Run("SuccessNoSegment", func(t *testing.T) {
		resp, err := RouteHttp(router, "GET", "/users", nil, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.Empty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, 2)
	})
}
