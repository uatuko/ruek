#include "relations.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "err/errors.h"

#include "common.h"

namespace svc {
namespace relations {
template <>
rpcCreate::result_type Impl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req) {
	auto tuple = map(ctx, req);
	tuple.store();

	rpcCreate::response_type response = map(tuple);
	response.set_cost(1);

	return {grpcxx::status::code_t::ok, response};
}

google::rpc::Status Impl::exception() noexcept {
	google::rpc::Status status;
	status.set_code(google::rpc::UNKNOWN);

	try {
		std::rethrow_exception(std::current_exception());
	} catch (const err::DbTupleInvalidData &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const std::exception &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(e.what());
	}

	return status;
}

db::Tuple Impl::map(
	const grpcxx::context &ctx, const rpcCreate::request_type &from) const noexcept {
	db::Tuple to({
		.lEntityId   = from.left_entity().id(),
		.lEntityType = from.left_entity().type(),
		.relation    = from.relation(),
		.rEntityId   = from.right_entity().id(),
		.rEntityType = from.right_entity().type(),
		.spaceId     = std::string(ctx.meta(common::space_id_v)),
		.strand      = from.strand(),
	});

	if (from.has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(from.attrs(), &attrs);

		to.attrs(std::move(attrs));
	}

	if (from.has_left_principal_id()) {
		to.lPrincipalId(from.left_principal_id());
	}

	if (from.has_right_principal_id()) {
		to.rPrincipalId(from.right_principal_id());
	}

	return to;
}

rpcCreate::response_type Impl::map(const db::Tuple &from) const noexcept {
	rpcCreate::response_type to;

	auto *tuple = to.mutable_tuple();
	map(from, tuple);

	return to;
}

void Impl::map(const db::Tuple &from, sentium::api::v1::Tuple *to) const noexcept {
	to->set_id(from.id());
	to->set_space_id(from.spaceId());

	if (from.lPrincipalId()) {
		to->set_left_principal_id(*from.lPrincipalId());
	} else {
		auto *entity = to->mutable_left_entity();
		entity->set_id(from.lEntityId());
		entity->set_type(from.lEntityType());
	}

	to->set_relation(from.relation());

	if (from.rPrincipalId()) {
		to->set_right_principal_id(*from.rPrincipalId());
	} else {
		auto *entity = to->mutable_right_entity();
		entity->set_id(from.rEntityId());
		entity->set_type(from.rEntityType());
	}

	if (!from.strand().empty()) {
		to->set_strand(from.strand());
	}

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to->mutable_attrs());
	}

	if (from.rid()) {
		to->set_ref_id(*from.rid());
	}
}
} // namespace relations
} // namespace svc
