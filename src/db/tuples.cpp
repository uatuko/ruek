#include "tuples.h"

#include <fmt/core.h>
#include <xid/xid.h>

#include "err/errors.h"

#include "common.h"

namespace db {
Tuple::Tuple(const Tuple::Data &data) noexcept : _data(data), _id(xid::next()), _rev(0), _rid() {
	sanitise();
}

Tuple::Tuple(Tuple::Data &&data) noexcept :
	_data(std::move(data)), _id(xid::next()), _rev(0), _rid() {
	sanitise();
}

Tuple::Tuple(const pg::row_t &r) :
	_data({
		.attrs        = r["attrs"].as<Data::attrs_t>(),
		.lEntityId    = r["l_entity_id"].as<std::string>(),
		.lEntityType  = r["l_entity_type"].as<std::string>(),
		.lPrincipalId = r["l_principal_id"].as<Data::pid_t>(),
		.relation     = r["relation"].as<std::string>(),
		.rEntityId    = r["r_entity_id"].as<std::string>(),
		.rEntityType  = r["r_entity_type"].as<std::string>(),
		.rPrincipalId = r["r_principal_id"].as<Data::pid_t>(),
		.spaceId      = r["space_id"].as<std::string>(),
		.strand       = r["strand"].as<std::string>(),
	}),
	_id(r["_id"].as<std::string>()), _rev(r["_rev"].as<int>()), _rid(r["_rid"].as<rid_t>()) {}

bool Tuple::discard(std::string_view id) {
	std::string_view qry = R"(
		delete from tuples
		where
			_id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	return (res.affected_rows() == 1);
}

std::optional<Tuple> Tuple::lookup(
	std::string_view spaceId, Entity left, Entity right, std::string_view relation,
	std::string_view strand) {

	auto results = LookupTuples(spaceId, left, relation, right, strand, "", 1);
	if (results.empty()) {
		return std::nullopt;
	}

	return results.front();
}

Tuple Tuple::retrieve(std::string_view id) {
	std::string_view qry = R"(
		select
			space_id,
			strand,
			l_entity_type, l_entity_id,
			relation,
			r_entity_type, r_entity_id,
			attrs,
			l_principal_id, r_principal_id,
			_id, _rid, _rev
		from tuples
		where _id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DbTupleNotFound();
	}

	return Tuple(res[0]);
}

void Tuple::sanitise() noexcept {
	if (_data.lPrincipalId) {
		_data.lEntityId   = *_data.lPrincipalId;
		_data.lEntityType = common::principal_entity_v;
	}

	if (_data.rPrincipalId) {
		_data.rEntityId   = *_data.rPrincipalId;
		_data.rEntityType = common::principal_entity_v;
	}
}

void Tuple::store() {
	std::string_view qry = R"(
		insert into tuples as t (
			space_id,
			strand,
			l_entity_type, l_entity_id,
			relation,
			r_entity_type, r_entity_id,
			attrs,
			l_principal_id, r_principal_id,
			_id, _rid, _rev
		) values (
			$1::text,
			$2::text,
			$3::text, $4::text,
			$5::text,
			$6::text, $7::text,
			$8::jsonb,
			$9::text, $10::text,
			$11::text, $12::text, $13::integer
		)
		on conflict (_id)
		do update
			set (
				attrs,
				_rev
			) = (
				$8::jsonb,
				excluded._rev + 1
			)
			where t._rev = $13::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(
			qry,
			_data.spaceId,
			_data.strand,
			_data.lEntityType,
			_data.lEntityId,
			_data.relation,
			_data.rEntityType,
			_data.rEntityId,
			_data.attrs,
			_data.lPrincipalId,
			_data.rPrincipalId,
			_id,
			_rid,
			_rev);
	} catch (pqxx::check_violation &) {
		throw err::DbTupleInvalidData();
	} catch (pg::fkey_violation_t &) {
		throw err::DbTupleInvalidKey();
	}

	if (res.empty()) {
		throw err::DbRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

Tuple::Entity::Entity(std::string_view pid) noexcept :
	_id(pid), _type(common::principal_entity_v) {}

Tuple::Entity::Entity(std::string_view type, std::string_view id) noexcept : _id(id), _type(type) {}

Tuples ListTuples(
	std::string_view spaceId, std::optional<Tuple::Entity> left, std::optional<Tuple::Entity> right,
	std::optional<std::string_view> relation, std::string_view lastId, std::uint16_t count) {

	if (left && right) {
		throw err::DbTuplesInvalidListArgs();
	}

	Tuple::Entity entity;
	std::string   where = "where space_id = $1::text";
	std::string   sort;
	if (left) {
		entity  = *left;
		where  += " and l_entity_type = $2::text and l_entity_id = $3::text";
		sort    = "r_entity_id";
	} else if (right) {
		entity  = *right;
		where  += " and r_entity_type = $2::text and r_entity_id = $3::text";
		sort    = "l_entity_id";
	} else {
		throw err::DbTuplesInvalidListArgs();
	}

	if (relation) {
		where += " and relation = $4::text";
	}

	if (!lastId.empty()) {
		if (relation) {
			where += fmt::format(" and {} < $5::text", sort);
		} else {
			where += fmt::format(" and {} < $4::text", sort);
		}
	}

	const std::string qry = fmt::format(
		R"(
			select
				space_id,
				strand,
				l_entity_type, l_entity_id,
				relation,
				r_entity_type, r_entity_id,
				attrs,
				l_principal_id, r_principal_id,
				_id, _rid, _rev
			from tuples
			{}
			order by {} desc
			limit {:d};
		)",
		where,
		sort,
		count);

	db::pg::result_t res;
	if (relation && !lastId.empty()) {
		res = pg::exec(qry, spaceId, entity.type(), entity.id(), relation, lastId);
	} else if (relation) {
		res = pg::exec(qry, spaceId, entity.type(), entity.id(), relation);
	} else if (!lastId.empty()) {
		res = pg::exec(qry, spaceId, entity.type(), entity.id(), lastId);
	} else {
		res = pg::exec(qry, spaceId, entity.type(), entity.id());
	}

	Tuples tuples;
	tuples.reserve(res.affected_rows());
	for (const auto &r : res) {
		tuples.emplace_back(r);
	}

	return tuples;
}

Tuples LookupTuples(
	std::string_view spaceId, Tuple::Entity left, std::string_view relation, Tuple::Entity right,
	std::optional<std::string_view> strand, std::string_view lastId, std::uint16_t count) {
	std::string where = R"(
		where
			space_id = $1::text
			and l_entity_type = $2::text and l_entity_id = $3::text
			and relation = $4::text
			and r_entity_type = $5::text and r_entity_id = $6::text
	)";

	// Looking up with a strand can only yield at most one result (due to unique key constraint).
	// Last id is ignored if strand has a value.
	if (strand) {
		where += " and strand = $7::text";
	} else if (!lastId.empty()) {
		where += " and _id < $7::text";
	}

	const std::string qry = fmt::format(
		R"(
			select
				space_id,
				strand,
				l_entity_type, l_entity_id,
				relation,
				r_entity_type, r_entity_id,
				attrs,
				l_principal_id, r_principal_id,
				_id, _rid, _rev
			from tuples
			{}
			order by _id desc
			limit {:d};
		)",
		where,
		count);

	db::pg::result_t res;
	if (strand) {
		res = pg::exec(
			qry, spaceId, left.type(), left.id(), relation, right.type(), right.id(), strand);
	} else if (!lastId.empty()) {
		res = pg::exec(
			qry, spaceId, left.type(), left.id(), relation, right.type(), right.id(), lastId);
	} else {
		res = pg::exec(qry, spaceId, left.type(), left.id(), relation, right.type(), right.id());
	}

	Tuples tuples;
	tuples.reserve(res.affected_rows());
	for (const auto &r : res) {
		tuples.emplace_back(r);
	}

	return tuples;
}
} // namespace db
