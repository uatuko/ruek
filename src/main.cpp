#include <cstdio>
#include <string_view>

#include <unistd.h>

#include <grpcxx/server.h>

#include "db/db.h"
#include "svc/svc.h"

int main(int argc, char *argv[]) {
	extern char *optarg;
	extern int   optind;

	std::string_view ipv4 = "127.0.0.1";
	int              port = 8080;

	int opt;
	while ((opt = getopt(argc, argv, "4:p:")) != -1) {
		switch (opt) {
		case '4':
			ipv4 = optarg;
			break;

		case 'p':
			port = std::atoi(optarg);
			if (port < 1 || port > 65535) {
				port = 8080;
			}

			break;

		default:
			std::fprintf(stderr, "Usage: %s [-4 ipv4] [-p port]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

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

	svc::Relations r;
	server.add(r.service());

	std::printf("Listening on [127.0.0.1:8080] ...\n");
	server.run(ipv4, port);

	return 0;
}
