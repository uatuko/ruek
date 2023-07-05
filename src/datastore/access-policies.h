#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"
#include "redis.h"

namespace datastore {
class AccessPolicy {
public:
	using principal_t = std::string;
	// using principals_t = std::set<principal_t>;

	using resource_t  = std::string;
	using resources_t = std::set<resource_t>;

	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	AccessPolicy(const Data &data) noexcept;
	AccessPolicy(Data &&data) noexcept;

	AccessPolicy(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	void store() const;
	void discard() const;

	void add_access(const principal_t &principal, const resource_t &resource) const;
	void add_identity_principal(const std::string principal_id) const;
	void add_collection_principal(const std::string principal_id) const;

private:
	Data        _data;
	mutable int _rev;
};

using AccessPolicies = std::vector<AccessPolicy>;

AccessPolicy RetrieveAccessPolicy(const std::string &id);

void DeleteAccess(
	const AccessPolicy::principal_t &principal, const AccessPolicy::resource_t &resource);
std::vector<AccessPolicy> CheckAccess(
	const AccessPolicy::principal_t &principal, const AccessPolicy::resource_t &resource);
} // namespace datastore
