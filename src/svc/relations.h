#pragma once
#include <google/rpc/status.pb.h>

#include "db/tuples.h"
#include "sentium/api/v1/relations.grpcxx.pb.h"

namespace svc {
namespace relations {
using namespace sentium::api::v1::Relations;

class Impl {
public:
	using service_type = Service;

	template <typename T>
	typename T::result_type call(grpcxx::context &, const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

	template <>
	rpcCreate::result_type call<rpcCreate>(
		grpcxx::context &ctx, const rpcCreate::request_type &req);

	google::rpc::Status exception() noexcept;

private:
	db::Tuple map(const grpcxx::context &ctx, const rpcCreate::request_type &from) const noexcept;

	rpcCreate::response_type map(const db::Tuple &from) const noexcept;

	void map(const db::Tuple &from, sentium::api::v1::Tuple *to) const noexcept;
};
} // namespace relations
} // namespace svc
