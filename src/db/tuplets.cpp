#include "tuplets.h"

#include <fmt/core.h>

#include "err/errors.h"

namespace db {
Tuplet::Tuplet(const pg::row_t &r) :
	_hash(r["_hash"].as<std::int64_t>()), _id(r["_id"].as<std::string>()),
	_relation(r["relation"].as<std::string>()), _strand(r["strand"].as<strand_t>()) {}

Tuplets TupletsList(
	std::string_view spaceId, std::optional<Tuple::Entity> left, std::optional<Tuple::Entity> right,
	std::optional<std::string_view> relation, std::uint16_t count) {

	if (left && right) {
		throw err::DbTupletsInvalidListArgs();
	}

	std::int64_t hv;
	std::string  hash, strand;
	std::string  where = "where space_id = $1::text";

	if (left) {
		hv      = left->hash();
		hash    = "_r_hash";
		strand  = "null";
		where  += " and _l_hash = $2::bigint";
	} else if (right) {
		hv      = right->hash();
		hash    = "_l_hash";
		strand  = "strand";
		where  += " and _r_hash = $2::bigint";
	} else {
		throw err::DbTupletsInvalidListArgs();
	}

	if (relation) {
		where += " and relation = $3::text";
	}

	const std::string qry = fmt::format(
		R"(
			select
				_id,
				{} as _hash,
				relation,
				{} as strand
			from tuples
			{}
			order by _hash desc
			limit {:d}
		)",
		hash,
		strand,
		where,
		count);

	db::pg::result_t res;
	if (relation) {
		res = pg::exec(qry, spaceId, hv, relation);
	} else {
		res = pg::exec(qry, spaceId, hv);
	}

	Tuplets tuplets;
	tuplets.reserve(res.affected_rows());
	for (const auto &r : res) {
		tuplets.emplace_back(r);
	}

	return tuplets;
}
} // namespace db
