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
