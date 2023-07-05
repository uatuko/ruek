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
			_rev
		) values (
			$1::text,
			$2::integer
		)
		on conflict (_id)
		do nothing
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.id, _rev);
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

void AccessPolicy::discard() const {
	std::string_view qry = R"(
		delete from "access-policies"
		where
			_id = $1::text;
	)";

	pg::exec(qry, _data.id);
}

void AccessPolicy::add_resource(const AccessPolicy::resource_t &resource) {
	redis::conn();
}

void AccessPolicy::add_identity_principal(const std::string principal_id) const {
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
		pg::exec(qry, id(), principal_id);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidCollectionOrMember();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateCollectionMember();
	}
}

void AccessPolicy::add_collection_principal(const std::string principal_id) const {
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
		pg::exec(qry, id(), principal_id);
	} catch (pg::fkey_violation_t &) {
		throw err::DatastoreInvalidCollectionOrMember();
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateCollectionMember();
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
} // namespace datastore
