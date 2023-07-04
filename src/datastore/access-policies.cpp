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
		.rules = r["rules"].as<Data::rules_t>(),
		.id    = r["_id"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void AccessPolicy::store() const {
	std::string_view qry = R"(
		insert into "access-policies" as t (
			_id,
			_rev,
			rules
		) values (
			$1::text,
			$2::integer,
			$3::jsonb
		)
		on conflict (_id)
		do update
			set (
				_rev,
				rules
			) = (
				excluded._rev + 1,
				$3::jsonb
			)
			where t._rev = $2::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.id, _rev, _data.rules);
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

AccessPolicy RetrieveAccessPolicy(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
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
} // namespace datastore
