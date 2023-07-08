#include "access-policies.h"

#include <utility>

#include <xid/xid.h>

#include "err/errors.h"

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
		.id = r["_id"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

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

void AccessPolicy::discard() const {
	std::string_view qry = R"(
		delete from "access-policies"
		where
			_id = $1::text;
	)";

	pg::exec(qry, _data.id);
}

void AccessPolicy::add(const AccessPolicy::Record &record) const {
	auto conn = datastore::redis::conn();
	conn.cmd("HSET " + record.key() + " " + _data.id + " \"\"");
}

void AccessPolicy::addIdentityPrincipal(const AccessPolicy::principal_t principalId) const {
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
		pg::exec(qry, id(), principalId);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidAccessPolicyOrIdentity();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateAccessPolicyIdentity();
	}
}

void AccessPolicy::addCollectionPrincipal(const AccessPolicy::principal_t principalId) const {
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
		pg::exec(qry, id(), principalId);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidAccessPolicyOrCollection();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateAccessPolicyCollection();
	}
}

AccessPolicy RetrieveAccessPolicy(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev
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

std::set<std::string> RetrieveAccessPolicyIdentities(const std::string &id) {
	std::string_view qry = R"(
		select
			identity_id
		from "access-policies_identities"
		where policy_id = $1::text
		union
		select
			c.identity_id
		from
			"access-policies_collections" p
			join "collections_identities" c on p.collection_id = c.collection_id
		;
	)";

	auto res = pg::exec(qry, id);

	std::set<std::string> results;
	for (const auto &r : res) {
		results.insert(r["identity_id"].as<std::string>());
	}

	return results;
}

std::vector<AccessPolicy> AccessPolicy::Record::check() const {
	auto conn = datastore::redis::conn();

	auto reply = conn.cmd("HGETALL " + key());

	std::vector<AccessPolicy> policies;
	for (int i = 0; i < reply->elements; i += 2) {
		policies.push_back(AccessPolicy({
			.id = reply->element[i]->str,
		}));
	}

	return policies;
}

void AccessPolicy::Record::discard() const {
	datastore::redis::conn().cmd("DEL " + key());
}
} // namespace datastore
