#include "roles.h"

#include <xid/xid.h>

#include "err/errors.h"

namespace datastore {
Role::Role(const Role::Data &data) noexcept : _data(data), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Role::Role(Data &&data) noexcept : _data(std::move(data)), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Role::Role(const pg::row_t &r) :
	_data({
		.id   = r["_id"].as<std::string>(),
		.name = r["name"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void Role::store() const {
	std::string_view qry = R"(
		insert into roles as t (
			_id,
			_rev,
			name
		) values (
			$1::text,
			$2::integer,
			$3::text
		)
		on conflict (_id)
		do update
			set (
				_rev,
				name
			) = (
				excluded._rev + 1,
				$3::text
			)
			where t._rev = $2::integer
		returning _rev;
	)";

	auto res = pg::exec(qry, _data.id, _rev, _data.name);
	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

void Role::addPermission(const std::string &pid) const {
	std::string_view qry = R"(
		insert into "roles_permissions" (
			role_id,
			permission_id
		) values (
			$1::text,
			$2::text
		)
	)";

	try {
		pg::exec(qry, _data.id, pid);
	} catch (pg::fkey_violation_t &e) {
		throw err::DatastoreInvalidRoleOrPermission();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateRoleOrPermission();
	}
}

Role RetrieveRole(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			name
		from roles
		where
			_id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DatastoreRoleNotFound();
	}

	return Role(res[0]);
}
} // namespace datastore
