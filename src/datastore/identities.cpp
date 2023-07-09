#include "identities.h"

#include <utility>

#include <xid/xid.h>

#include "err/errors.h"

namespace datastore {
Identity::Identity(const Identity::Data &data) noexcept : _data(data), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Identity::Identity(Data &&data) noexcept : _data(std::move(data)), _rev(0) {
	if (_data.id.empty()) {
		_data.id = xid::next();
	}
}

Identity::Identity(const pg::row_t &r) :
	_data({
		.attrs = r["attrs"].as<Data::attrs_t>(),
		.id    = r["_id"].as<std::string>(),
		.sub   = r["sub"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

void Identity::discard() const {
	std::string_view qry = R"(
		delete from identities
		where
			_id = $1::text;
	)";

	pg::exec(qry, _data.id);
}

void Identity::store() const {
	std::string_view qry = R"(
		insert into identities as t (
			_id,
			_rev,
			sub,
			attrs
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
				sub,
				attrs
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
		res = pg::exec(qry, _data.id, _rev, _data.sub, _data.attrs);
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

Identities LookupIdentities(const std::string &sub) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			sub,
			attrs
		from identities
		where
			sub = $1::text;
	)";

	auto res = pg::exec(qry, sub);

	Identities identities;
	for (const auto &row : res) {
		identities.push_back(Identity(row));
	}

	return identities;
}

Identity RetrieveIdentity(const std::string &id) {
	std::string_view qry = R"(
		select
			_id,
			_rev,
			sub,
			attrs
		from identities
		where
			_id = $1::text;
	)";

	auto res = pg::exec(qry, id);
	if (res.empty()) {
		throw err::DatastoreIdentityNotFound();
	}

	return Identity(res[0]);
}
} // namespace datastore
