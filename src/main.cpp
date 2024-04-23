#include <cstdio>

#include <grpcxx/server.h>

#include "db/db.h"
#include "svc/svc.h"

int main() {
	try {
		db::init();
	} catch (const std::exception &e) {
		std::cout << "[FATAL] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	grpcxx::server server;

	svc::Authz a;
	server.add(a.service());

	svc::Entities e;
	server.add(e.service());

	svc::Principals p;
	server.add(p.service());

	std::printf("Listening on [127.0.0.1:8080] ...\n");
	server.run("127.0.0.1", 8080);

	return 0;
}
