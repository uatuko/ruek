#include "records.h"

#include "err/errors.h"

namespace db {
Record::Record(const Record::Data &data) noexcept : _data(data), _rev(0) {}

Record::Record(Record::Data &&data) noexcept : _data(std::move(data)), _rev(0) {}

Record::Record(const pg::row_t &r) :
	_data({
		.attrs        = r["attrs"].as<Data::attrs_t>(),
		.principalId  = r["principal_id"].as<std::string>(),
		.resourceId   = r["resource_id"].as<std::string>(),
		.resourceType = r["resource_type"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

bool Record::discard(
	const std::string &principalId, const std::string &resourceType,
	const std::string &resourceId) {
	std::string_view qry = R"(
		delete from records
		where
			principal_id = $1::text
			and resource_type = $2::text
			and resource_id = $3::text
		;
	)";

	auto res = pg::exec(qry, principalId, resourceType, resourceId);
	return (res.affected_rows() == 1);
}

std::optional<Record> Record::lookup(
	const std::string &principalId, const std::string &resourceType,
	const std::string &resourceId) {
	std::string_view qry = R"(
		select
			_rev,
			principal_id,
			resource_type,
			resource_id,
			attrs
		from records
		where
			principal_id = $1::text
			and resource_type = $2::text
			and resource_id = $3::text
		;
	)";

	auto res = pg::exec(qry, principalId, resourceType, resourceId);
	if (res.empty()) {
		return std::nullopt;
	}

	return Record(res[0]);
}

void Record::store() {
	std::string_view qry = R"(
		insert into records as t (
			_rev,
			principal_id,
			resource_type,
			resource_id,
			attrs
		) values (
			$1::integer,
			$2::text,
			$3::text,
			$4::text,
			$5::jsonb
		)
		on conflict (principal_id, resource_type, resource_id)
		do update
			set (
				_rev,
				attrs
			) = (
				excluded._rev + 1,
				$5::jsonb
			)
			where t._rev = $1::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(
			qry, _rev, _data.principalId, _data.resourceType, _data.resourceId, _data.attrs);
	} catch (pqxx::check_violation &) {
		throw err::DbRecordInvalidData();
	} catch (pg::fkey_violation_t &) {
		throw err::DbRecordInvalidPrincipalId();
	}

	if (res.empty()) {
		throw err::DbRevisionMismatch();
	}

	_rev = res.at(0, 0).as<int>();
}
} // namespace db
