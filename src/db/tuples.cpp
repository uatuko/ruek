#include "tuples.h"

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
	std::string_view spaceId, std::string_view lPrincipalId, std::string_view rEntityType,
	std::string_view rEntityId) {

	return lookup(
		spaceId, "", common::principal_entity_v, lPrincipalId, "", rEntityType, rEntityId);
}

std::optional<Tuple> Tuple::lookup(
	std::string_view spaceId, std::string_view strand, std::string_view lEntityType,
	std::string_view lEntityId, std::string_view relation, std::string_view rEntityType,
	std::string_view rEntityId) {

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
		where
			space_id = $1::text
			and strand = $2::text
			and l_entity_type = $3::text and l_entity_id = $4::text
			and relation = $5::text
			and r_entity_type = $6::text and r_entity_id = $7::text
		;
	)";

	auto res =
		pg::exec(qry, spaceId, strand, lEntityType, lEntityId, relation, rEntityType, rEntityId);
	if (res.empty()) {
		return std::nullopt;
	}

	return Tuple(res[0]);
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
} // namespace db
