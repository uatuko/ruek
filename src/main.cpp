#include <cstdio>

#include <grpcxx/server.h>

#include "svc/principals.h"

int main() {
	grpcxx::server server;

	svc::Principals p(server);

	std::printf("Listening on [127.0.0.1:7000] ...\n");
	server.run("127.0.0.1", 7000);

	return 0;
}
