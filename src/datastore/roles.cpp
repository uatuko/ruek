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
		.id          = r["_id"].as<std::string>(),
		.name        = r["name"].as<std::string>(),
		.permissions = r["permissions"].as<permissions_t>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void Role::store() const {
	std::string_view qry = R"(
		insert into roles as t (
			_id,
			_rev,
			name,
			permissions
		) values (
			$1::text,
			$2::integer,
			$3::text,
			$4::text[]
		)
		on conflict (_id)
		do update
			set (
				_rev,
				name,
				permissions
			) = (
				excluded._rev + 1,
				$3::text,
				$4::text[]
			)
			where t._rev = $2::integer
		returning _rev;
	)";

	// FIXME: add pqxx support for permissions_t
	//        ref: https://github.com/jtv/libpqxx/blob/master/include/pqxx/doc/datatypes.md
	std::vector<std::string> perms(_data.permissions.size());
	perms.assign(_data.permissions.begin(), _data.permissions.end());

	auto res = pg::exec(qry, _data.id, _rev, _data.name, perms);
	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

Role RetrieveRole(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			name,
			permissions
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
