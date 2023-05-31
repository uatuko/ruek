#include "collections.h"

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

void Collection::add(const member_t &mid) const {
	std::string_view qry = R"(
		insert into collection_members (
			collection_id,
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

const Collection::members_t Collection::members() const {
	std::string_view qry = R"(
		select
			identity_id
		from
			collection_members
		where
			collection_id = $1::text;
	)";

	auto res = pg::exec(qry, id());

	members_t members;
	for (const auto &r : res) {
		members.insert(r["identity_id"].as<member_t>());
	}

	return members;
}

void Collection::remove(const member_t &mid) const {
	std::string_view qry = R"(
		delete from collection_members
		where
			collection_id = $1::text and
			identity_id = $2::text;
	)";

	pg::exec(qry, id(), mid);
}
} // namespace datastore