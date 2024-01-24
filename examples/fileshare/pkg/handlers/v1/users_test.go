package v1

import (
	"encoding/json"
	"io"
	"net/http"
	"testing"

	"github.com/gin-gonic/gin"
	"github.com/rs/xid"
	"github.com/stretchr/testify/require"
)

func TestCreateUser(t *testing.T) {
	router := gin.New()
	router.POST("/users", createUser)

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

	expectedResp := User{
		Id:   user.Id,
		Name: userReq.Name,
	}

	require.Equal(t, expectedResp, user)
}

func TestListUsers(t *testing.T) {
	router := gin.New()
	router.GET("/users", listUsers)

	segment := xid.New().String()
	numUsers := 5
	users, err := createUsers(segment, numUsers)
	require.NoError(t, err)
	defer deleteUsers(users)

	t.Run("SuccessWithUsers", func(t *testing.T) {
		listUsersReq := ListUsersRequest{Segment: segment}
		resp, err := RouteHttp(router, "GET", "/users", listUsersReq, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.Empty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, numUsers)
		require.Equal(t, users, listUsersResp.Users)
	})

	t.Run("SuccessNoUsers", func(t *testing.T) {
		listUsersReq := ListUsersRequest{Segment: xid.New().String()}
		resp, err := RouteHttp(router, "GET", "/users", listUsersReq, nil)
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
		paginationLimit := uint32(numUsers - 1)
		listUsersReq := ListUsersRequest{
			PaginationLimit: paginationLimit,
			Segment:         segment,
		}
		resp, err := RouteHttp(router, "GET", "/users", listUsersReq, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.NotEmpty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, numUsers-1)
	})

	t.Run("SuccessWithPaginationLimitAndToken", func(t *testing.T) {
		// First search
		paginationLimit := uint32(numUsers - 1)
		listUsersReq := ListUsersRequest{
			PaginationLimit: paginationLimit,
			Segment:         segment,
		}
		resp, err := RouteHttp(router, "GET", "/users", listUsersReq, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err := io.ReadAll(resp.Body)
		require.NoError(t, err)

		var listUsersResp ListUsersResponse
		json.Unmarshal(respBody, &listUsersResp)

		require.NotEmpty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, numUsers-1)

		// Second search
		listUsersReq.PaginationToken = listUsersResp.PaginationToken
		resp, err = RouteHttp(router, "GET", "/users", listUsersReq, nil)
		require.NoError(t, err)

		require.Equal(t, http.StatusOK, resp.StatusCode)

		respBody, err = io.ReadAll(resp.Body)
		require.NoError(t, err)

		json.Unmarshal(respBody, &listUsersResp)

		require.Empty(t, listUsersResp.PaginationToken)
		require.Len(t, listUsersResp.Users, 1)
	})
}
