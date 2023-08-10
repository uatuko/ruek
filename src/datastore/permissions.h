#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Permission {
public:
	struct Data {
		std::string id;

		bool operator==(const Data &) const noexcept = default;
	};

	Permission(const Data &data) noexcept;
	Permission(Data &&data) noexcept;

	Permission(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }

	void store() const;

private:
	Data _data;
};

using Permissions = std::vector<Permission>;

Permissions ListPermissions();
Permissions RetrievePermissionsByRole(const std::string &rid);
Permission  RetrievePermission(const std::string &id);
} // namespace datastore
