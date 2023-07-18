#include <iostream>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <viper/viper.h>

#include "datastore/datastore.h"
#include "service/gatekeeper.h"
#include "service/interceptors/logger.h"
#include "svc/events.h"

int main() {
	try {
		viper::init("app", std::filesystem::current_path() / "conf");
	} catch (const std::exception &e) {
		std::cout << "[FATAL] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	auto conf = *viper::conf();

	datastore::config c = {
		.redis =
			{
				.host = conf["redis.host"].get<std::string>(),
				.port = static_cast<int>(conf["redis.port"].get<long>()),
			},
	};

	try {
		datastore::init(c);
	} catch (const std::exception &e) {
		std::cout << "[FATAL] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	grpc::ServerBuilder builder;
	builder.AddListeningPort(std::string{conf["tcp.address"]}, grpc::InsecureServerCredentials());

	std::array<std::unique_ptr<grpc::Service>, 2> services = {
		std::make_unique<svc::Events>(),
		std::make_unique<service::Gatekeeper>(),
	};

	for (const auto &s : services) {
		builder.RegisterService(s.get());
	}

	// Interceptors
	{
		using factory_t = std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>;
		std::vector<factory_t> creators;

		if (conf["app.debug"].get<bool>()) {
			factory_t logger(new service::interceptors::LoggerFactory());
			creators.push_back(std::move(logger));
		}

		if (creators.size() > 0) {
			builder.experimental().SetInterceptorCreators(std::move(creators));
		}
	}

	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	server->Wait();

	return EXIT_SUCCESS;
}
