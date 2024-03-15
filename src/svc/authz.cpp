#include "authz.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "err/errors.h"

#include "common.h"

namespace svc {
namespace authz {
template <>
rpcCheck::result_type Impl::call<rpcCheck>(
	grpcxx::context &ctx, const rpcCheck::request_type &req) {
	auto r = db::Record::lookup(
		ctx.meta(common::space_id_v), req.principal_id(), req.resource_type(), req.resource_id());
	return {grpcxx::status::code_t::ok, map(r)};
}

template <>
rpcGrant::result_type Impl::call<rpcGrant>(
	grpcxx::context &ctx, const rpcGrant::request_type &req) {
	// Upsert if exists
	if (auto r = db::Record::lookup(
			ctx.meta(common::space_id_v),
			req.principal_id(),
			req.resource_type(),
			req.resource_id());
		r) {
		if (req.has_attrs()) {
			std::string attrs;
			google::protobuf::util::MessageToJsonString(req.attrs(), &attrs);

			r->attrs(std::move(attrs));
			r->store();
		}

		return {grpcxx::status::code_t::ok, map(r.value())};
	}

	auto r = map(ctx, req);
	r.store();

	return {grpcxx::status::code_t::ok, map(r)};
}

template <>
rpcRevoke::result_type Impl::call<rpcRevoke>(
	grpcxx::context &ctx, const rpcRevoke::request_type &req) {
	db::Record::discard(
		ctx.meta(common::space_id_v), req.principal_id(), req.resource_type(), req.resource_id());
	return {grpcxx::status::code_t::ok, rpcRevoke::response_type()};
}

google::rpc::Status Impl::exception() noexcept {
	google::rpc::Status status;
	status.set_code(google::rpc::UNKNOWN);

	try {
		std::rethrow_exception(std::current_exception());
	} catch (const err::DbRecordInvalidData &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::DbRecordInvalidPrincipalId &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::DbRevisionMismatch &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(std::string(e.str()));
	} catch (const std::exception &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(e.what());
	}

	return status;
}

db::Record Impl::map(
	const grpcxx::context &ctx, const rpcGrant::request_type &from) const noexcept {
	db::Record to({
		.principalId  = from.principal_id(),
		.resourceId   = from.resource_id(),
		.resourceType = from.resource_type(),
		.spaceId      = std::string(ctx.meta(common::space_id_v)),
	});

	if (from.has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(from.attrs(), &attrs);

		to.attrs(std::move(attrs));
	}

	return to;
}

rpcCheck::response_type Impl::map(const std::optional<db::Record> &from) const noexcept {
	rpcCheck::response_type to;
	if (!from) {
		to.set_ok(false);
		return to;
	}

	to.set_ok(true);
	if (from->attrs()) {
		google::protobuf::util::JsonStringToMessage(*from->attrs(), to.mutable_attrs());
	}

	return to;
}

rpcGrant::response_type Impl::map(const db::Record &from) const noexcept {
	return {};
}
} // namespace authz
} // namespace svc
