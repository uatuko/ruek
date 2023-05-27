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
			sub
		) values (
			$1::text,
			$2::integer,
			$3::text
		)
		on conflict (_id)
		do update
			set (
				_rev,
				sub
			) = (
				excluded._rev + 1,
				$3::text
			)
			where t._rev = $2::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(qry, _data.id, _rev, _data.sub);
	} catch (pg::unique_violation_t &) {
		throw err::DatastoreDuplicateIdentity();
	}

	if (res.empty()) {
		throw err::DatastoreRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}
} // namespace datastore
