#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class Role {
public:
	using permissions_t = std::set<std::string>;

	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	Role(const Data &data) noexcept;
	Role(Data &&data) noexcept;

	Role(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	void store() const;
	void addPermission(const std::string &pid) const;

private:
	Data        _data;
	mutable int _rev;
};

using Roles = std::vector<Role>;

Role RetrieveRole(const std::string &id);
} // namespace datastore
