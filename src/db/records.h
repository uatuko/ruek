#pragma once

#include <optional>
#include <string>
#include <vector>

#include "pg.h"

namespace db {
class Record {
public:
	struct Data {
		using attrs_t = std::optional<std::string>;

		attrs_t     attrs;
		std::string principalId;
		std::string resourceId;
		std::string resourceType;
		std::string spaceId;

		bool operator==(const Data &) const noexcept = default;
	};

	Record(const Data &data) noexcept;
	Record(Data &&data) noexcept;

	Record(const pg::row_t &r);

	bool operator==(const Record &) const noexcept = default;

	const Data::attrs_t &attrs() const noexcept { return _data.attrs; }
	void                 attrs(const Data::attrs_t &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(const std::string &attrs) noexcept { _data.attrs = attrs; }
	void                 attrs(Data::attrs_t &&attrs) noexcept { _data.attrs = std::move(attrs); }
	void                 attrs(std::string &&attrs) noexcept { _data.attrs = std::move(attrs); }

	const std::string &principalId() const noexcept { return _data.principalId; }
	const std::string &resourceId() const noexcept { return _data.resourceId; }
	const std::string &resourceType() const noexcept { return _data.resourceType; }
	const std::string &spaceId() const noexcept { return _data.spaceId; }

	const int &rev() const noexcept { return _rev; }

	void store();

	static bool discard(
		std::string_view spaceId, const std::string &principalId, const std::string &resourceType,
		const std::string &resourceId);

	static std::optional<Record> lookup(
		std::string_view spaceId, const std::string &principalId, const std::string &resourceType,
		const std::string &resourceId);

private:
	Data _data;
	int  _rev;
};

using Records = std::vector<Record>;

Records ListRecordsByPrincipal(
	std::string_view spaceId, std::string_view principalId, std::string_view resourceType,
	std::string_view lastId = "", std::uint16_t count = 10);

Records ListRecordsByResource(
	std::string_view spaceId, std::string_view resourceType, std::string_view resourceId,
	std::string_view lastId = "", std::uint16_t count = 10);
} // namespace db
