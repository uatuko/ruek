#include "rbac-policies.h"

#include <utility>

#include <xid/xid.h>

#include "err/errors.h"

namespace datastore {
RbacPolicy::RbacPolicy(const RbacPolicy::Data &data) noexcept : _data(data), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

RbacPolicy::RbacPolicy(Data &&data) noexcept : _data(std::move(data)), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

RbacPolicy::RbacPolicy(const pg::row_t &r) :
	_data({
		.id   = r["_id"].as<std::string>(),
		.name = r["name"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void RbacPolicy::addPrincipal(const principal_t &pid) const {
	std::string_view qry = R"(
		insert into "rbac-policies_identities" (
			policy_id,
			identity_id
		) values (
			$1::text,
			$2::text
		);
	)";

	try {
		pg::exec(qry, id(), pid);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidRbacPolicyOrPrincipal();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateRbacPolicyPrincipal();
	}
}

void RbacPolicy::addRole(const role_t &rid) const {
	std::string_view qry = R"(
		insert into "rbac-policies_roles" (
			policy_id,
			role_id
		) values (
			$1::text,
			$2::text
		);
	)";

	try {
		pg::exec(qry, id(), rid);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidRbacPolicyOrRole();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateRbacPolicyRole();
	}
}

void RbacPolicy::store() const {
	std::string_view qry = R"(
		insert into "rbac-policies" as t (
			_id,
			_rev,
			name
		) values (
			$1::text,
			$2::integer,
			$3::text
		)
		on conflict (_id)
		do update
			set (
				_rev,
				name
			) = (
				excluded._rev + 1,
				$3::text
			)
			where t._rev = $2::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.id, _rev, _data.name);
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateRbacPolicy();
	}

	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

const RbacPolicy::principals_t RbacPolicy::principals() const {
	std::string_view qry = R"(
		select
			identity_id
		from
			"rbac-policies_identities"
		where
			policy_id = $1::text;
	)";

	auto res = pg::exec(qry, id());

	principals_t members;
	for (const auto &r : res) {
		members.insert(r["identity_id"].as<principal_t>());
	}

	return members;
}

const RbacPolicy::roles_t RbacPolicy::roles() const {
	std::string_view qry = R"(
		select
			role_id
		from
			"rbac-policies_roles"
		where
			policy_id = $1::text;
	)";

	auto res = pg::exec(qry, id());

	roles_t roles;
	for (const auto &r : res) {
		roles.insert(r["role_id"].as<role_t>());
	}

	return roles;
}
} // namespace datastore
