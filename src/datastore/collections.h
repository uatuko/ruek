#pragma once

#include <set>
#include <string>
#include <vector>

#include "access-policies.h"
#include "pg.h"
#include "rbac-policies.h"

namespace datastore {
class Collection {
public:
	using member_t  = std::string;
	using members_t = std::set<member_t>;

	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	Collection(const Data &data) noexcept;
	Collection(Data &&data) noexcept;

	Collection(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	const AccessPolicies accessPolicies() const;
	void                 add(const member_t &id) const;
	const members_t      members() const;
	const RbacPolicies   rbacPolicies() const;
	void                 remove(const member_t &id) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using Collections = std::vector<Collection>;

Collection                  RetrieveCollection(const std::string &id);
const Collection::members_t RetrieveCollectionMembers(const std::string &id);
} // namespace datastore
