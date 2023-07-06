#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"

namespace datastore {
class RbacPolicy {
public:
	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	struct Rule {
		std::string roleId;
		std::string attrs;

		bool operator==(const Rule &) const noexcept = default;
	};

	struct Principal {
		std::string id;
		enum class Type { Unspecified, Collection, Identity } type;

		bool operator==(const Principal &) const noexcept = default;
	};

	struct Record {
		std::string identityId;
		std::string permission;

		const std::string key() const noexcept {
			return "rbac:(" + identityId + ")>[" + permission + "]";
		};

		bool operator==(const Record &) const noexcept = default;

		std::vector<RbacPolicy> check() const;
	};

	using Rules      = std::vector<Rule>;
	using Principals = std::vector<Principal>;

	RbacPolicy(const Data &data) noexcept;
	RbacPolicy(Data &&data) noexcept;

	RbacPolicy(const pg::row_t &t);

	const std::string &id() const noexcept { return _data.id; }
	const int         &rev() const noexcept { return _rev; }

	const std::string &name() const noexcept { return _data.name; }
	void               name(const std::string &name) noexcept { _data.name = name; }
	void               name(std::string &&name) noexcept { _data.name = std::move(name); }

	const Principals principals() const;
	void             addPrincipal(const Principal principal) const;
	void             addCollection(const std::string collectionId) const;
	void             addIdentity(const std::string IdentityId) const;

	void             addRecord(const Record &record) const;

	const Rules rules() const;
	void        addRule(const Rule &rule) const;

	void store() const;

private:
	Data        _data;
	mutable int _rev;
};

using RbacPolicies = std::vector<RbacPolicy>;
} // namespace datastore
