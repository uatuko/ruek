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

	try {
		pg::exec(qry, _data.id);
	} catch (const pqxx::check_violation &) {
		throw err::DatastoreInvalidPermissionData();
	} catch (const pqxx::unique_violation &) {
		throw err::DatastoreDuplicatePermission();
	}
}

Permissions ListPermissions() {
	std::string_view qry = R"(
		select
			_id
		from permissions;
	)";

	auto res = pg::exec(qry);

	Permissions permissions;
	for (const auto &r : res) {
		permissions.push_back(Permission(r));
	}

	return permissions;
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

Permissions RetrievePermissionsByRole(const std::string &rid) {
	std::string_view qry = R"(
		select
			_id
		from permissions
		inner join "roles_permissions" on _id=permission_id
		where role_id = $1::text;
	)";

	auto res = pg::exec(qry, rid);

	Permissions permissions;
	for (const auto &r : res) {
		permissions.push_back(Permission(r));
	}

	return permissions;
}
} // namespace datastore
