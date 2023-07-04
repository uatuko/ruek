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
		.rules = r["attrs"].as<Data::rules_t>(),
	}),
	_rev(r["_rev"].as<int>()) {}

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

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.id, _rev, _data.name, _data.rules);
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
} // namespace datastore
