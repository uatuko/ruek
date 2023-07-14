#include "collections.h"

#include <iostream>
#include <utility>

#include <xid/xid.h>

#include "err/errors.h"

namespace datastore {
Collection::Collection(const Collection::Data &data) noexcept : _data(data), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Collection::Collection(Data &&data) noexcept : _data(std::move(data)), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Collection::Collection(const pg::row_t &r) :
	_data({
		.id   = r["_id"].as<std::string>(),
		.name = r["name"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void Collection::add(const member_t &mid) const {
	std::string_view qry = R"(
		insert into collections_identities (
			collection_id,
			identity_id
		) values (
			$1::text,
			$2::text
		);
	)";

	try {
		pg::exec(qry, _data.id, mid);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidCollectionOrMember();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateCollectionMember();
	}
}

const AccessPolicies Collection::accessPolicies() const {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			name,
			rules
		from
			"access-policies_collections"
		inner join "access-policies" on _id = policy_id
		where
			collection_id = $1::text;
	)";

	auto res = pg::exec(qry, _data.id);

	AccessPolicies policies;
	for (const auto &r : res) {
		auto policy = AccessPolicy(r);
		policies.push_back(policy);
	}

	return policies;
}

const Collection::members_t Collection::members() const {
	std::string_view qry = R"(
		select
			identity_id
		from
			collections_identities
		where
			collection_id = $1::text;
	)";

	auto res = pg::exec(qry, _data.id);

	members_t members;
	for (const auto &r : res) {
		members.insert(r["identity_id"].as<member_t>());
	}

	return members;
}

const RbacPolicies Collection::rbacPolicies() const {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			name
		from
			"rbac-policies_collections"
		inner join "rbac-policies" on _id = policy_id
		where
			collection_id = $1::text;
	)";

	auto res = pg::exec(qry, _data.id);

	RbacPolicies policies;
	for (const auto &r : res) {
		auto policy = RbacPolicy(r);
		policies.push_back(policy);
	}

	return policies;
}

void Collection::remove(const member_t &mid) const {
	std::string_view qry = R"(
		delete from collections_identities
		where
			collection_id = $1::text and
			identity_id = $2::text;
	)";

	pg::exec(qry, _data.id, mid);
}

void Collection::store() const {
	std::string_view qry = R"(
		insert into collections as t (
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

	auto res = pg::exec(qry, _data.id, _rev, _data.name);
	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

Collection RetrieveCollection(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			name
		from collections
		where
			_id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DatastoreCollectionNotFound();
	}

	return Collection(res[0]);
}

const Collection::members_t RetrieveCollectionMembers(const std::string &id) {
	std::string_view qry = R"(
		select
			identity_id
		from collections_identities
		where
			collection_id = $1::text;
	)";

	auto res = pg::exec(qry, id);

	Collection::members_t members;
	for (const auto &r : res) {
		members.insert(r["identity_id"].as<Collection::member_t>());
	}

	return members;
}
} // namespace datastore
