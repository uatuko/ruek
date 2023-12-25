#include "principals.h"

#include <fmt/core.h>
#include <xid/xid.h>

#include "err/errors.h"

namespace db {
Principal::Principal(const Principal::Data &data) noexcept : _data(data), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Principal::Principal(Data &&data) noexcept : _data(std::move(data)), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Principal::Principal(const pg::row_t &r) :
	_data({
		.attrs   = r["attrs"].as<Data::attrs_t>(),
		.id      = r["id"].as<std::string>(),
		.segment = r["segment"].as<Data::segment_t>(),
	}),
	_rev(r["_rev"].as<int>()) {}

bool Principal::discard(const std::string &id) {
	std::string_view qry = R"(
		delete from principals
		where
			id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	return (res.affected_rows() == 1);
}

Principal Principal::retrieve(const std::string &id) {
	std::string_view qry = R"(
		select
			_rev,
			id,
			segment,
			attrs
		from principals
		where
			id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DbPrincipalNotFound();
	}

	return Principal(res[0]);
}

void Principal::store() {
	std::string_view qry = R"(
		insert into principals as t (
			_rev,
			id,
			segment,
			attrs
		) values (
			$1::integer,
			$2::text,
			$3::text,
			$4::jsonb
		)
		on conflict (id)
		do update
			set (
				_rev,
				segment,
				attrs
			) = (
				excluded._rev + 1,
				$3::text,
				$4::jsonb
			)
			where t._rev = $1::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(qry, _rev, _data.id, _data.segment, _data.attrs);
	} catch (pqxx::check_violation &) {
		throw err::DbPrincipalInvalidData();
	}

	if (res.empty()) {
		throw err::DbRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

Principals ListPrincipals(
	Principal::Data::segment_t segment, std::string_view lastId, std::uint16_t count) {
	std::string where;
	if (segment) {
		where = "where segment = $1::text";
	} else {
		where = "where segment is null";
	}

	if (!lastId.empty()) {
		if (segment) {
			where += " and id < $2::text";
		} else {
			where += " and id < $1::text";
		}
	}

	const std::string qry = fmt::format(
		R"(
			select
				_rev,
				id,
				segment,
				attrs
			from principals
			{}
			order by id desc
			limit {:d};
		)",
		where,
		count);

	db::pg::result_t res;
	if (segment && !lastId.empty()) {
		res = pg::exec(qry, segment, lastId);
	} else if (segment) {
		res = pg::exec(qry, segment);
	} else if (!lastId.empty()) {
		res = pg::exec(qry, lastId);
	} else {
		res = pg::exec(qry);
	}

	Principals principals;
	principals.reserve(res.affected_rows());
	for (const auto &r : res) {
		principals.emplace_back(r);
	}

	return principals;
}
} // namespace db
