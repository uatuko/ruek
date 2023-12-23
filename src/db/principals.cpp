#include "principals.h"

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
		.attrs    = r["attrs"].as<Data::attrs_t>(),
		.id       = r["id"].as<std::string>(),
		.parentId = r["parent_id"].as<Data::pid_t>(),
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
			parent_id,
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
			parent_id,
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
				parent_id,
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
		res = pg::exec(qry, _rev, _data.id, _data.parentId, _data.attrs);
	} catch (pqxx::check_violation &) {
		throw err::DbPrincipalInvalidData();
	} catch (pg::fkey_violation_t &) {
		throw err::DbPrincipalInvalidParentId();
	}

	if (res.empty()) {
		throw err::DbRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}
} // namespace db
