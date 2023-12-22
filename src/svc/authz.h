#pragma once
#include <google/rpc/status.pb.h>

#include "db/records.h"
#include "sentium/api/v1/authz.grpcxx.pb.h"

namespace svc {
namespace authz {
using namespace sentium::api::v1::Authz;

class Impl {
public:
	using service_type = Service;

	template <typename T>
	typename T::result_type call(grpcxx::context &, const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

	template <>
	rpcCheck::result_type call<rpcCheck>(grpcxx::context &ctx, const rpcCheck::request_type &req);

	template <>
	rpcGrant::result_type call<rpcGrant>(grpcxx::context &ctx, const rpcGrant::request_type &req);

	template <>
	rpcRevoke::result_type call<rpcRevoke>(
		grpcxx::context &ctx, const rpcRevoke::request_type &req);

	google::rpc::Status exception() noexcept;

private:
	rpcCheck::response_type map(const std::optional<db::Record> &from) const noexcept;

	db::Record              map(const rpcGrant::request_type &from) const noexcept;
	rpcGrant::response_type map(const db::Record &from) const noexcept;
};
} // namespace authz
} // namespace svc
