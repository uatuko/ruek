#include <chrono>

#include <grpcpp/support/server_interceptor.h>

namespace svc {
namespace interceptors {
class Logger final : public grpc::experimental::Interceptor {
public:
	Logger(grpc::experimental::ServerRpcInfo *rpc);
	~Logger();

	void Intercept(grpc::experimental::InterceptorBatchMethods *methods) override;

private:
	grpc::experimental::ServerRpcInfo *_rpc;

	std::chrono::high_resolution_clock::time_point _start;
};

class LoggerFactory final : public grpc::experimental::ServerInterceptorFactoryInterface {
public:
	grpc::experimental::Interceptor *CreateServerInterceptor(
		grpc::experimental::ServerRpcInfo *info) override {
		return new Logger(info);
	}
};
} // namespace interceptors
} // namespace svc
