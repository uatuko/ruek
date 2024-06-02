#pragma once
#include <deque>
#include <optional>
#include <string_view>

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
	rpcCheck::result_type call<rpcCheck>(grpcxx::context &ctx, const rpcCheck::request_type &req);

	template <>
	rpcCreate::result_type call<rpcCreate>(
		grpcxx::context &ctx, const rpcCreate::request_type &req);

	template <>
	rpcDelete::result_type call<rpcDelete>(
		grpcxx::context &ctx, const rpcDelete::request_type &req);

	template <>
	rpcListLeft::result_type call<rpcListLeft>(
		grpcxx::context &ctx, const rpcListLeft::request_type &req);

	template <>
	rpcListRight::result_type call<rpcListRight>(
		grpcxx::context &ctx, const rpcListRight::request_type &req);

	google::rpc::Status exception() noexcept;

private:
	struct graph_t {
		std::int32_t          cost;
		std::deque<db::Tuple> path;
	};

	struct spot_t {
		std::int32_t             cost;
		std::optional<db::Tuple> tuple;
	};

	db::Tuple map(const grpcxx::context &ctx, const rpcCreate::request_type &from) const noexcept;

	rpcCreate::response_type map(const db::Tuple &from) const noexcept;

	void map(const db::Tuple &from, sentium::api::v1::Tuple *to) const noexcept;
	void map(
		const db::Tuples                                            &from,
		google::protobuf::RepeatedPtrField<sentium::api::v1::Tuple> *to) const noexcept;

	// Check for a relation between left and right entities using the `graph` algorithm.
	graph_t graph(
		std::string_view spaceId, db::Tuple::Entity left, std::string_view relation,
		db::Tuple::Entity right, std::uint16_t limit) const;

	// Check for a relation between left and right entities using the `spot` algorithm.
	spot_t spot(
		std::string_view spaceId, db::Tuple::Entity left, std::string_view relation,
		db::Tuple::Entity right, std::uint16_t limit) const;
};
} // namespace relations
} // namespace svc
