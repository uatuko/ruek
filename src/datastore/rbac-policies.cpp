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
		.id    = r["_id"].as<std::string>(),
		.name   = r["name"].as<std::string>(),
		.rules = r["rules"].as<rules_t>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void RbacPolicy::addMember(const member_t &mid) const {
	std::string_view qry = R"(
		insert into rbac-policies_identities (
			policy_id,
			identity_id
		) values (
			$1::text,
			$2::text
		);
	)";

	try {
		pg::exec(qry, id(), mid);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidCollectionOrMember();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateCollectionMember();
	}
}

void RbacPolicy::addRole(const role_t &rid) const {
	std::string_view qry = R"(
		insert into rbac-policies_roles (
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
		throw err::DatastoreInvalidCollectionOrMember();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateCollectionMember();
	}
}

void RbacPolicy::store() const {
	std::string_view qry = R"(
		insert into rbac-policies as t (
			_id,
			_rev,
			name,
			rules
		) values (
			$1::text,
			$2::integer,
			$3::text,
			$4::jsonb
		)
		on conflict (_id)
		do update
			set (
				_rev,
				name,
				rules
			) = (
				excluded._rev + 1,
				$3::text,
				$4::jsonb
			)
			where t._rev = $2::integer
		returning _rev;
	)";

		// FIXME: add pqxx support for rules_t
	//        ref: https://github.com/jtv/libpqxx/blob/master/include/pqxx/doc/datatypes.md
	std::vector<std::string> rules(_data.rules.size());
	rules.assign(_data.rules.begin(), _data.rules.end());

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.id, _rev, _data.name, rules);
	} catch (pqxx::check_violation &) {
		throw err::DatastoreInvalidIdentityData();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateIdentity();
	}

	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

const RbacPolicy::members_t RbacPolicy::members() const {
	std::string_view qry = R"(
		select
			identity_id
		from
			rbac-policies_identities
		where
			policy_id = $1::text;
	)";

	auto res = pg::exec(qry, id());

	members_t members;
	for (const auto &r : res) {
		members.insert(r["identity_id"].as<member_t>());
	}

	return members;
}

void RbacPolicy::removeMember(const member_t &mid) const {
	std::string_view qry = R"(
		delete from rbac-policies_identities
		where
			policy_id = $1::text and
			identity_id = $2::text;
	)";

	pg::exec(qry, id(), mid);
}

void RbacPolicy::removeRole(const role_t &rid) const {
	std::string_view qry = R"(
		delete from rbac-policies_roles
		where
			policy_id = $1::text and
			role_id = $2::text;
	)";

	pg::exec(qry, id(), rid);
}

const RbacPolicy::roles_t RbacPolicy::roles() const {
	std::string_view qry = R"(
		select
			role_id
		from
			rbac-policies_roles
		where
			policy_id = $1::text;
	)";

	auto res = pg::exec(qry, id());

	roles_t members;
	for (const auto &r : res) {
		members.insert(r["role_id"].as<role_t>());
	}

	return members;
}
} // namespace datastore
