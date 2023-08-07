#include "access-policies.h"

#include <utility>

#include <xid/xid.h>

#include "err/errors.h"

#include "collections.h"
#include "redis.h"

namespace datastore {
AccessPolicy::AccessPolicy(const AccessPolicy::Data &data) noexcept : _data(data), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

AccessPolicy::AccessPolicy(Data &&data) noexcept : _data(std::move(data)), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

AccessPolicy::AccessPolicy(const pg::row_t &r) :
	_data({
		.id    = r["_id"].as<std::string>(),
		.name  = r["name"].as<Data::name_t>(),
		.rules = r["rules"].as<Data::rules_t>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void AccessPolicy::addCollection(const AccessPolicy::collection_t &id) const {
	std::string_view qry = R"(
		insert into "access-policies_collections" (
			policy_id,
			collection_id
		) values (
			$1::text,
			$2::text
		);
	)";

	try {
		pg::exec(qry, _data.id, id);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidAccessPolicyOrCollection();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateAccessPolicyCollection();
	}

	const auto members = RetrieveCollectionMembers(id);
	for (const auto &mid : members) {
		for (const auto &rule : _data.rules) {
			Cache cache({
				.identity = mid,
				.policy   = _data.id,
				.rule     = rule,
			});

			cache.store();
		}
	}
}

void AccessPolicy::addIdentity(const AccessPolicy::identity_t &id) const {
	std::string_view qry = R"(
		insert into "access-policies_identities" (
			policy_id,
			identity_id
		) values (
			$1::text,
			$2::text
		);
	)";

	try {
		pg::exec(qry, _data.id, id);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidAccessPolicyOrIdentity();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateAccessPolicyIdentity();
	}

	for (const auto &rule : _data.rules) {
		Cache cache({
			.identity = id,
			.policy   = _data.id,
			.rule     = rule,
		});

		cache.store();
	}
}

void AccessPolicy::removeIdentity(const AccessPolicy::identity_t &id) const {
	std::string_view qry = R"(
		delete from "access-policies_identities"
		where (policy_id, identity_id) = ($1::text, $2::text);
	)";

	try {
		pg::exec(qry, _data.id, id);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidAccessPolicyOrIdentity();
	}

	for (const auto &rule : _data.rules) {
		Cache cache({
			.identity = id,
			.policy   = _data.id,
			.rule     = rule,
		});

		cache.discard();
	}
}

const AccessPolicy::collections_t AccessPolicy::collections() const {
	std::string qry = R"(
		select
			collection_id
		from "access-policies_collections"
		where policy_id = $1::text;
	)";

	auto res = pg::exec(qry, _data.id);

	collections_t collections;
	for (const auto &r : res) {
		collections.insert(r["collection_id"].as<std::string>());
	}

	return collections;
}

const AccessPolicy::identities_t AccessPolicy::identities(bool expand) const {
	std::string qry = R"(
		select
			identity_id
		from "access-policies_identities"
		where policy_id = $1::text
	)";

	if (expand) {
		qry += R"(
			union
			select
				c.identity_id
			from
				"access-policies_collections" p
				join "collections_identities" c on p.collection_id = c.collection_id
			;
		)";
	} else {
		qry += ';';
	}

	auto res = pg::exec(qry, _data.id);

	identities_t identities;
	for (const auto &r : res) {
		identities.insert(r["identity_id"].as<std::string>());
	}

	return identities;
}

void AccessPolicy::store() const {
	std::string_view qry = R"(
		insert into "access-policies" as t (
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

	auto res = pg::exec(qry, _data.id, _rev, _data.name, _data.rules);
	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}

const Policies AccessPolicy::Cache::check(
	const std::string &identity, const std::string &resource) {
	auto conn  = datastore::redis::conn();
	auto reply = conn.cmd("hgetall %s", key(identity, resource).c_str());

	Policies policies;
	for (int i = 0; i < reply->elements; i += 2) {
		policies.push_back({
			.attrs = reply->element[i + 1]->str,
			.id    = reply->element[i]->str,
		});
	}

	return policies;
}

void AccessPolicy::Cache::discard() const {
	datastore::redis::conn().cmd("del %s", key().c_str());
}

void AccessPolicy::Cache::store() const {
	auto conn = datastore::redis::conn();
	conn.cmd("hset %s %s %s", key().c_str(), policy.c_str(), rule.attrs.c_str());
}

AccessPolicy RetrieveAccessPolicy(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			name,
			rules
		from "access-policies"
		where
			_id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DatastoreAccessPolicyNotFound();
	}

	return AccessPolicy(res[0]);
}

AccessPolicies RetrieveAccessPoliciesByCollection(const std::string &cid) {
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

	auto res = pg::exec(qry, cid);

	AccessPolicies policies;
	for (const auto &r : res) {
		auto policy = AccessPolicy(r);
		policies.push_back(policy);
	}

	return policies;
}

} // namespace datastore
