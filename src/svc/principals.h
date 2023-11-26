#pragma once

#include <grpcxx/server.h>

#include "sentium/api/v1/principals.grpcxx.pb.h"

namespace svc {
class Principals {
public:
	Principals(grpcxx::server &s);

	template <typename T> typename T::result_type call(const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

private:
	sentium::api::v1::Principals::Service _svc;
};
} // namespace svc
