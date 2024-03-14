#include "records.h"

#include <fmt/core.h>

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
		.spaceId      = r["space_id"].as<std::string>(),
	}),
	_rev(r["_rev"].as<int>()) {}

bool Record::discard(
	std::string_view spaceId, const std::string &principalId, const std::string &resourceType,
	const std::string &resourceId) {
	std::string_view qry = R"(
		delete from records
		where
			space_id = $1::text
			and principal_id = $2::text
			and resource_type = $3::text
			and resource_id = $4::text
		;
	)";

	auto res = pg::exec(qry, spaceId, principalId, resourceType, resourceId);
	return (res.affected_rows() == 1);
}

std::optional<Record> Record::lookup(
	std::string_view spaceId, const std::string &principalId, const std::string &resourceType,
	const std::string &resourceId) {
	std::string_view qry = R"(
		select
			space_id,
			principal_id,
			resource_type,
			resource_id,
			attrs,
			_rev
		from records
		where
			space_id = $1::text
			and principal_id = $2::text
			and resource_type = $3::text
			and resource_id = $4::text
		;
	)";

	auto res = pg::exec(qry, spaceId, principalId, resourceType, resourceId);
	if (res.empty()) {
		return std::nullopt;
	}

	return Record(res[0]);
}

void Record::store() {
	std::string_view qry = R"(
		insert into records as t (
			space_id,
			principal_id,
			resource_type,
			resource_id,
			attrs,
			_rev
		) values (
			$1::text,
			$2::text,
			$3::text,
			$4::text,
			$5::jsonb,
			$6::integer
		)
		on conflict (space_id, principal_id, resource_type, resource_id)
		do update
			set (
				attrs,
				_rev
			) = (
				$5::jsonb,
				excluded._rev + 1
			)
			where t._rev = $6::integer
		returning _rev;
	)";

	pg::result_t res;
	try {
		res = pg::exec(
			qry,
			_data.spaceId,
			_data.principalId,
			_data.resourceType,
			_data.resourceId,
			_data.attrs,
			_rev);
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

Records ListRecordsByPrincipal(
	std::string_view spaceId, std::string_view principalId, std::string_view resourceType,
	std::string_view lastId, std::uint16_t count) {
	std::string where =
		"where space_id = $1::text and principal_id = $2::text and resource_type = $3::text";
	if (!lastId.empty()) {
		where += " and resource_id < $4::text";
	}

	const std::string qry = fmt::format(
		R"(
			select
				space_id,
				principal_id,
				resource_type,
				resource_id,
				attrs,
				_rev
			from records
			{}
			order by resource_id desc
			limit {:d}
		)",
		where,
		count);

	db::pg::result_t res;
	if (!lastId.empty()) {
		res = pg::exec(qry, spaceId, principalId, resourceType, lastId);
	} else {
		res = pg::exec(qry, spaceId, principalId, resourceType);
	}

	Records records;
	records.reserve(res.affected_rows());
	for (const auto &r : res) {
		records.emplace_back(r);
	}

	return records;
}

Records ListRecordsByResource(
	std::string_view spaceId, std::string_view resourceType, std::string_view resourceId,
	std::string_view lastId, std::uint16_t count) {
	std::string where =
		"where space_id = $1::text and resource_type = $2::text and resource_id = $3::text";
	if (!lastId.empty()) {
		where += " and principal_id < $4::text";
	}

	const std::string qry = fmt::format(
		R"(
			select
				space_id,
				principal_id,
				resource_type,
				resource_id,
				attrs,
				_rev
			from records
			{}
			order by principal_id desc
			limit {:d}
		)",
		where,
		count);

	db::pg::result_t res;
	if (!lastId.empty()) {
		res = pg::exec(qry, spaceId, resourceType, resourceId, lastId);
	} else {
		res = pg::exec(qry, spaceId, resourceType, resourceId);
	}

	Records records;
	records.reserve(res.affected_rows());
	for (const auto &r : res) {
		records.emplace_back(r);
	}

	return records;
}
} // namespace db
