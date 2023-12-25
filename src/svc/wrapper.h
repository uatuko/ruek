#pragma once

#include <cstdio>
#include <string>

#include <google/rpc/status.pb.h>

#include "encoding/b64.h"

// Forward declarations
namespace grpcxx {
class context;
}

namespace svc {
template <class Impl> class Wrapper {
public:
	Wrapper() : _impl(), _service(*this) {}

	template <typename T>
	typename T::result_type call(grpcxx::context &ctx, const typename T::request_type &req) {
		typename T::result_type result;
		try {
			result = _impl.template call<T>(ctx, req);
		} catch (...) {
			google::rpc::Status s = _impl.exception();
			std::printf("Error: %s\n", s.message().c_str());

			std::string data;
			s.SerializeToString(&data);

			result = {
				{static_cast<grpcxx::status::code_t>(s.code()), encoding::b64::encode(data)},
				std::nullopt};
		}

		return result;
	}

	constexpr auto &service() noexcept { return _service; }

private:
	Impl                        _impl;
	typename Impl::service_type _service;
};
} // namespace svc
