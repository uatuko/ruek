package v1

import (
	"encoding/json"
	"io"
	"net/http"
	"testing"

	"github.com/gin-gonic/gin"
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
