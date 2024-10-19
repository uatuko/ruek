#include "principals.h"

#include <fmt/core.h>
#include <xid/xid.h>

#include "err/errors.h"

#include "detail.h"

namespace db {
Principal::Principal(const Principal::Data &data) noexcept : _data(data), _rev(detail::rng()) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Principal::Principal(Data &&data) noexcept : _data(std::move(data)), _rev(detail::rng()) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Principal::Principal(const pg::row_t &r) :
	_data({
		.attrs   = r["attrs"].as<Data::attrs_t>(),
		.id      = r["id"].as<std::string>(),
		.segment = r["segment"].as<Data::segment_t>(),
		.spaceId = r["space_id"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

bool Principal::discard(std::string_view spaceId, const std::string &id) {
	std::string_view qry = R"(
		delete from principals
		where
			space_id = $1::text
			and id = $2::text;
	)";

	auto res = pg::exec(qry, spaceId, id);
	return (res.affected_rows() == 1);
}

Principal Principal::retrieve(std::string_view spaceId, const std::string &id) {
	std::string_view qry = R"(
		select
			space_id,
			id,
			segment,
			attrs,
			_rev
		from principals
		where
			space_id = $1::text
			and id = $2::text;
	)";

	auto res = pg::exec(qry, spaceId, id);
	if (res.empty()) {
		throw err::DbPrincipalNotFound();
	}

	return Principal(res[0]);
}

void Principal::store() {
	std::string_view qry = R"(
		insert into principals as t (
			space_id,
			id,
			segment,
			attrs,
			_rev
		) values (
			$1::text,
			$2::text,
			$3::text,
			$4::jsonb,
			$5::integer
		)
		on conflict (space_id, id)
		do update
			set (
				segment,
				attrs,
				_rev
			) = (
				$3::text,
				$4::jsonb,
				excluded._rev + 1
			)
			where t._rev = $5::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.spaceId, _data.id, _data.segment, _data.attrs, _rev);
	} catch (pqxx::check_violation &) {
		throw err::DbPrincipalInvalidData();
	}

	if (res.empty()) {
		throw err::DbRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

Principals ListPrincipals(
	std::string_view spaceId, Principal::Data::segment_t segment, std::string_view lastId,
	std::uint16_t count) {
	std::string where = "where space_id = $1::text";
	if (segment) {
		where += " and segment = $2::text";
	} else {
		where += " and segment is null";
	}

	if (!lastId.empty()) {
		if (segment) {
			where += " and id < $3::text";
		} else {
			where += " and id < $2::text";
		}
	}

	const std::string qry = fmt::format(
		R"(
			select
				space_id,
				id,
				segment,
				attrs,
				_rev
			from principals
			{}
			order by id desc
			limit {:d};
		)",
		where,
		count);

	db::pg::result_t res;
	if (segment && !lastId.empty()) {
		res = pg::exec(qry, spaceId, segment, lastId);
	} else if (segment) {
		res = pg::exec(qry, spaceId, segment);
	} else if (!lastId.empty()) {
		res = pg::exec(qry, spaceId, lastId);
	} else {
		res = pg::exec(qry, spaceId);
	}

	Principals principals;
	principals.reserve(res.affected_rows());
	for (const auto &r : res) {
		principals.emplace_back(r);
	}

	return principals;
}
} // namespace db
