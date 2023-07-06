#pragma once

#include <set>
#include <string>
#include <vector>

#include "pg.h"
#include "redis.h"

namespace datastore {
class AccessPolicy {
public:
	using identity_t  = std::string;
	using principal_t = std::string;
	using resource_t  = std::string;

	struct Data {
		std::string id;
		std::string name;

		bool operator==(const Data &) const noexcept = default;
	};

	struct Record {
		identity_t identity_id;
		resource_t resource;

		const std::string key() const noexcept {
			return "access:(" + identity_id + ")>[" + resource + "]";
		};

		Record(const identity_t i, const resource_t r) : identity_id(i), resource(r) {}

		bool operator==(const Record &) const noexcept = default;

		void                      discard() const;
		std::vector<AccessPolicy> check() const;
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

	void add(const Record &record) const;
	void addIdentityPrincipal(const principal_t principalId) const;
	void addCollectionPrincipal(const principal_t principalId) const;

private:
	Data        _data;
	mutable int _rev;
};

using AccessPolicies = std::vector<AccessPolicy>;

AccessPolicy RetrieveAccessPolicy(const std::string &id);
} // namespace datastore
