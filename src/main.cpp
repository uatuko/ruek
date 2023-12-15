#include <cstdio>

#include <grpcxx/server.h>

#include "db/db.h"
#include "svc/principals.h"

int main() {
	try {
		db::init();
	} catch (const std::exception &e) {
		std::cout << "[FATAL] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	grpcxx::server server;

	svc::Principals p;
	server.add(p.svc());

	std::printf("Listening on [127.0.0.1:7000] ...\n");
	server.run("127.0.0.1", 7000);

	return 0;
}
