package v1

import (
	sentium "github.com/sentium/examples/fileshare/pkg/pb/sentium/api/v1"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var authzClient sentium.AuthzClient
var principalsClient sentium.PrincipalsClient
var resourcesClient sentium.ResourcesClient

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
