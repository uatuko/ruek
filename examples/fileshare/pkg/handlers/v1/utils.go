package v1

import (
	"fmt"

	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var authzClient sentium.AuthzClient
var principalsClient sentium.PrincipalsClient
var resourcesClient sentium.ResourcesClient

var shareOrder = map[string]int{
	"owner":  0,
	"editor": 1,
	"viewer": 2,
}

func canShare(sharerRole string, sharedRole string) error {
	if sharerRole != "owner" && sharerRole != "editor" {
		return fmt.Errorf("role cannot share (sharer role: %s)", sharerRole)
	}

	if shareOrder[sharerRole] > shareOrder[sharedRole] {
		return fmt.Errorf(
			"cannot share to higher role (sharer role: %s, sharee role: %s)",
			sharerRole,
			sharedRole,
		)
	}

	return nil
}

func getAuthzClient() (sentium.AuthzClient, error) {
	if authzClient != nil {
		return authzClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	authzClient = sentium.NewAuthzClient(conn)
	return authzClient, nil
}

func getPrincipalsClient() (sentium.PrincipalsClient, error) {
	if principalsClient != nil {
		return principalsClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	principalsClient = sentium.NewPrincipalsClient(conn)
	return principalsClient, nil
}

func getResourcesClient() (sentium.ResourcesClient, error) {
	if resourcesClient != nil {
		return resourcesClient, nil
	}

	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	}

	conn, err := grpc.Dial("127.0.0.1:7000", opts...)
	if err != nil {
		return nil, err
	}

	resourcesClient := sentium.NewResourcesClient(conn)
	return resourcesClient, nil
}
