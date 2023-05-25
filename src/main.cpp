#include <iostream>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <viper/viper.h>

#include "datastore/datastore.h"
#include "service/grpc.h"

int main() {
	try {
		viper::init("app", std::filesystem::current_path() / "conf");
	} catch (const std::exception &e) {
		std::cout << "[FATAL] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	try {
		datastore::init();
	} catch (const std::exception &e) {
		std::cout << "[FATAL] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	auto conf = *viper::conf();

	service::Grpc       service;
	grpc::ServerBuilder builder;
	builder.AddListeningPort(std::string{conf["tcp.address"]}, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	server->Wait();

	return EXIT_SUCCESS;
}
