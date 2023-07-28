#include "permissions.h"

#include <xid/xid.h>

#include "err/errors.h"

namespace datastore {
Permission::Permission(const Permission::Data &data) noexcept : _data(data) {}
Permission::Permission(Data &&data) noexcept : _data(std::move(data)) {}

Permission::Permission(const pg::row_t &r) :
	_data({
		.id = r["_id"].as<std::string>(),
	}) {}

void Permission::store() const {
	std::string_view qry = R"(
		insert into permissions (
			_id
		) values (
			$1::text
		);
	)";

	pg::exec(qry, _data.id);
}

Permission RetrievePermission(const std::string &id) {
	std::string_view qry = R"(
		select
			_id
		from permissions
		where _id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DatastorePermissionNotFound();
	}

	return Permission(res[0]);
}

Permissions RetrieveRolePermissions(const std::string &rid) {
	std::string_view qry = R"(
		select
			permission_id
		from "roles_permissions"
		where role_id = $1::text;
	)";

	auto res = pg::exec(qry, rid);

	Permissions permissions;
	for (const auto &r : res) {
		permissions.insert(r["permission_id"].as<std::string>());
	}

	return permissions;
}
} // namespace datastore
