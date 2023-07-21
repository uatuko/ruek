#include "logger.h"

#include "logger/logger.h"

namespace svc {
namespace interceptors {
Logger::Logger(grpc::experimental::ServerRpcInfo *rpc) :
	_rpc(rpc), _start(std::chrono::high_resolution_clock::now()) {}

Logger::~Logger() {
	std::chrono::duration<double, std::milli> diff =
		std::chrono::high_resolution_clock::now() - _start;

	logger::info("svc", "latency_ms", diff.count(), "method", _rpc->method());
}

void Logger::Intercept(grpc::experimental::InterceptorBatchMethods *methods) {
	methods->Proceed();
}
} // namespace interceptors
} // namespace svc
