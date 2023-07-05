#pragma once

#include <set>
#include <string>

#include "pg.h"

namespace datastore {
class RbacPolicy {
public:
	using principal_t  = std::string;
	using principals_t = std::set<principal_t>;
	using role_t  = std::string;
	using roles_t = std::set<role_t>;
	struct Data {
		std::string id;
    std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	RbacPolicy(const Data &data) noexcept;
	RbacPolicy(Data &&data) noexcept;

	RbacPolicy(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	const principals_t principals() const;
	void               addPrincipal(const principal_t &id) const;
	void               removePrincipal(const principal_t &id) const;

	const roles_t roles() const;
	void          addRole(const role_t &id) const;
	void          removeRole(const role_t &id) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using RbacPolicies = std::vector<RbacPolicy>;
} // namespace datastore
